#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

juce::AudioProcessorValueTreeState::ParameterLayout HypnoGrooveAudioProcessor::makeParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "pattern", 1 }, "Pattern", juce::StringArray { "A", "B" }, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "swing", 1 }, "Swing", juce::NormalisableRange<float> { 0.f, 100.f, 1.f }, 18.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "human", 1 }, "Humanize", juce::NormalisableRange<float> { 0.f, 100.f, 1.f }, 12.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "chance", 1 }, "Hit probability", juce::NormalisableRange<float> { 0.f, 100.f, 1.f }, 100.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "master", 1 }, "Master volume", juce::NormalisableRange<float> { 0.f, 1.5f, .01f }, .75f));
    const std::array<float, hypno::lanes> defaults { .90f, .74f, .53f, .55f, .38f, .42f };
    for (int i = 0; i < hypno::lanes; ++i)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { "level" + juce::String(i), 1 },
            juce::String(hypno::laneNames[size_t(i)]) + " level",
            juce::NormalisableRange<float> { 0.f, 1.f, .01f }, defaults[size_t(i)]));
    return layout;
}

HypnoGrooveAudioProcessor::HypnoGrooveAudioProcessor()
  : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    parameters(*this, nullptr, "HYPNO_GROOVE_PARAMETERS", makeParameterLayout()) {
    formatManager.registerBasicFormats();
    resetFactoryPatterns();
}

bool HypnoGrooveAudioProcessor::isBusesLayoutSupported(const BusesLayout& layout) const {
    return layout.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layout.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void HypnoGrooveAudioProcessor::prepareToPlay(double sampleRate, int) {
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    for (int lane = 0; lane < hypno::lanes; ++lane) {
        auto result = std::make_shared<SampleData>();
        result->audio = hypno::makeFactoryHit(lane, currentSampleRate);
        result->sourceSampleRate = currentSampleRate;
        factorySamples[size_t(lane)] = std::move(result);
    }
    clearVoices();
    expectedNextPpq = -1.0;
}

void HypnoGrooveAudioProcessor::releaseResources() {
    clearVoices();
    expectedNextPpq = -1.0;
}

void HypnoGrooveAudioProcessor::clearVoices() {
    for (auto& voice : voices) voice.sample.reset();
}

uint16_t HypnoGrooveAudioProcessor::getLaneMask(int pattern, int lane) const {
    if (pattern < 0 || pattern >= 2 || lane < 0 || lane >= hypno::lanes) return 0;
    return patterns[size_t(pattern)][size_t(lane)].load(std::memory_order_relaxed);
}

void HypnoGrooveAudioProcessor::toggleStep(int pattern, int lane, int step) {
    if (pattern < 0 || pattern >= 2 || lane < 0 || lane >= hypno::lanes || step < 0 || step >= 16) return;
    patterns[size_t(pattern)][size_t(lane)].fetch_xor(uint16_t(1u << step), std::memory_order_relaxed);
}

void HypnoGrooveAudioProcessor::resetFactoryPatterns() {
    const auto a = hypno::patternA(), b = hypno::patternB();
    for (int lane = 0; lane < hypno::lanes; ++lane) {
        patterns[0][size_t(lane)].store(a[size_t(lane)], std::memory_order_relaxed);
        patterns[1][size_t(lane)].store(b[size_t(lane)], std::memory_order_relaxed);
    }
}

bool HypnoGrooveAudioProcessor::loadSample(int lane, const juce::File& file) {
    if (lane < 0 || lane >= hypno::lanes || !file.existsAsFile()) return false;
    auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(file));
    if (reader == nullptr || reader->sampleRate <= 0 || reader->lengthInSamples < 1) return false;
    const auto count = int(std::min<int64_t>(reader->lengthInSamples, int64_t(reader->sampleRate * 8.0)));
    juce::AudioBuffer<float> buffer(2, count);
    buffer.clear();
    if (!reader->read(&buffer, 0, 0, count, true, true)) return false;
    auto sample = std::make_shared<SampleData>();
    sample->sourceSampleRate = reader->sampleRate;
    sample->audio.resize(size_t(count));
    const bool stereo = reader->numChannels >= 2;
    for (int i = 0; i < count; ++i)
        sample->audio[size_t(i)] = stereo ?
            (buffer.getSample(0, i) + buffer.getSample(1, i)) * 0.5f : buffer.getSample(0, i);
    std::atomic_store(&userSamples[size_t(lane)], std::shared_ptr<const SampleData>(sample));
    {
        const juce::ScopedLock lock(pathsLock);
        userSamplePaths[size_t(lane)] = file.getFullPathName();
    }
    return true;
}

void HypnoGrooveAudioProcessor::useFactorySample(int lane) {
    if (lane < 0 || lane >= hypno::lanes) return;
    std::atomic_store(&userSamples[size_t(lane)], std::shared_ptr<const SampleData>());
    const juce::ScopedLock lock(pathsLock);
    userSamplePaths[size_t(lane)].clear();
}

juce::String HypnoGrooveAudioProcessor::sampleName(int lane) const {
    if (lane < 0 || lane >= hypno::lanes) return {};
    const juce::ScopedLock lock(pathsLock);
    return userSamplePaths[size_t(lane)].isEmpty() ? "Factory" :
        juce::File(userSamplePaths[size_t(lane)]).getFileNameWithoutExtension();
}

void HypnoGrooveAudioProcessor::startVoice(int lane, float velocity) {
    if (lane < 0 || lane >= hypno::lanes) return;
    auto source = std::atomic_load(&userSamples[size_t(lane)]);
    if (!source) source = factorySamples[size_t(lane)];
    if (!source || source->audio.empty()) return;
    Voice* choice = nullptr;
    for (auto& voice : voices) {
        if (!voice.sample) { choice = &voice; break; }
    }
    if (!choice) choice = &voices[0]; // Rare voice steal; bounded realtime allocation.
    choice->sample = std::move(source);
    choice->position = 0.0;
    choice->increment = choice->sample->sourceSampleRate / currentSampleRate;
    choice->gain = velocity;
    choice->lane = lane;
}

void HypnoGrooveAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    if (buffer.getNumSamples() == 0) return;
    const auto* head = getPlayHead();
    if (head == nullptr) {
        clearVoices();
        expectedNextPpq = -1.0;
        return;
    }
    const auto pos = head->getPosition();
    if (!pos || !pos->getIsPlaying() || !pos->getPpqPosition().hasValue()) {
        clearVoices();
        expectedNextPpq = -1.0;
        return;
    }

    const double bpm = juce::jlimit(20.0, 320.0, pos->getBpm().orFallback(124.0));
    displayedBpm.store(bpm, std::memory_order_relaxed);
    const double startPpq = *pos->getPpqPosition();
    const double ppqPerSample = bpm / (60.0 * currentSampleRate);
    const double endPpq = startPpq + ppqPerSample * buffer.getNumSamples();
    if (expectedNextPpq >= 0.0 && std::abs(startPpq - expectedNextPpq) > .025)
        clearVoices(); // Position jumped, e.g. user seeks or loop restarts.
    expectedNextPpq = endPpq;

    const int pattern = std::clamp(int(std::round(parameters.getRawParameterValue("pattern")->load())), 0, 1);
    const float swing = parameters.getRawParameterValue("swing")->load();
    const float human = parameters.getRawParameterValue("human")->load();
    const float chance = parameters.getRawParameterValue("chance")->load();
    const float master = parameters.getRawParameterValue("master")->load();
    std::array<float, hypno::lanes> levels{};
    std::array<uint16_t, hypno::lanes> masks{};
    for (int lane = 0; lane < hypno::lanes; ++lane) {
        levels[size_t(lane)] = parameters.getRawParameterValue("level" + juce::String(lane))->load();
        masks[size_t(lane)] = getLaneMask(pattern, lane);
    }

    std::array<Event, 1024> events{};
    int eventCount = 0;
    const int64_t first = int64_t(std::floor(startPpq * 4.0)) - 2;
    const int64_t last = int64_t(std::ceil(endPpq * 4.0)) + 2;
    for (int64_t absStep = first; absStep <= last; ++absStep) {
        const double beat = hypno::stepPpq(absStep, swing);
        if (beat < startPpq - 1e-9 || beat >= endPpq - 1e-9) continue;
        const int step = int((absStep % 16 + 16) % 16);
        int offset = int(std::round((beat - startPpq) / ppqPerSample));
        offset = std::clamp(offset, 0, buffer.getNumSamples() - 1);
        for (int lane = 0; lane < hypno::lanes; ++lane) {
            if (!hypno::isActive(masks[size_t(lane)], step)) continue;
            const auto seed = uint32_t(absStep) * 36529u + uint32_t(lane) * 90149u + 12977u;
            if (hypno::unitRandom(seed) * 100.0f >= chance) continue;
            if (eventCount < int(events.size()))
                events[size_t(eventCount++)] = { offset, lane, hypno::stepVelocity(absStep, lane, human) };
        }
    }
    std::sort(events.begin(), events.begin() + eventCount,
        [](const Event& a, const Event& b) { return a.offset < b.offset; });

    int nextEvent = 0;
    const int channels = buffer.getNumChannels();
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        while (nextEvent < eventCount && events[size_t(nextEvent)].offset <= i) {
            const auto& e = events[size_t(nextEvent++)];
            startVoice(e.lane, e.velocity);
        }
        float output = 0.0f;
        for (auto& voice : voices) {
            if (!voice.sample) continue;
            const auto& wav = voice.sample->audio;
            const auto index = size_t(voice.position);
            if (index + 1 >= wav.size()) { voice.sample.reset(); continue; }
            const float fraction = float(voice.position - double(index));
            const float value = wav[index] + fraction * (wav[index + 1] - wav[index]);
            output += value * voice.gain * levels[size_t(voice.lane)];
            voice.position += voice.increment;
        }
        const float final = std::tanh(output * master * .65f); // Protect against clipping.
        for (int ch = 0; ch < channels; ++ch) buffer.setSample(ch, i, final);
    }
}

void HypnoGrooveAudioProcessor::getStateInformation(juce::MemoryBlock& data) {
    auto state = parameters.copyState();
    for (int p = 0; p < 2; ++p)
        for (int lane = 0; lane < hypno::lanes; ++lane)
            state.setProperty("steps_" + juce::String(p) + "_" + juce::String(lane),
                              int(getLaneMask(p, lane)), nullptr);
    {
        const juce::ScopedLock lock(pathsLock);
        for (int lane = 0; lane < hypno::lanes; ++lane)
            state.setProperty("sample_" + juce::String(lane), userSamplePaths[size_t(lane)], nullptr);
    }
    juce::MemoryOutputStream stream(data, false);
    state.writeToStream(stream);
}

void HypnoGrooveAudioProcessor::setStateInformation(const void* data, int size) {
    auto state = juce::ValueTree::readFromData(data, size_t(size));
    if (!state.isValid() || !state.hasType(parameters.state.getType())) return;
    for (int p = 0; p < 2; ++p)
        for (int lane = 0; lane < hypno::lanes; ++lane) {
            const auto key = "steps_" + juce::String(p) + "_" + juce::String(lane);
            if (state.hasProperty(key))
                patterns[size_t(p)][size_t(lane)].store(
                    uint16_t(int(state.getProperty(key))), std::memory_order_relaxed);
        }
    parameters.replaceState(state);
    for (int lane = 0; lane < hypno::lanes; ++lane) {
        const auto path = state.getProperty("sample_" + juce::String(lane)).toString();
        if (path.isNotEmpty() && juce::File(path).existsAsFile())
            loadSample(lane, juce::File(path));
        else useFactorySample(lane);
    }
}

juce::AudioProcessorEditor* HypnoGrooveAudioProcessor::createEditor() {
    return new HypnoGrooveAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new HypnoGrooveAudioProcessor();
}
