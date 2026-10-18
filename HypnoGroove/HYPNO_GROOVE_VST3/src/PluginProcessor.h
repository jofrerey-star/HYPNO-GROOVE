#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>
#include "GrooveCore.h"

class HypnoGrooveAudioProcessor final : public juce::AudioProcessor {
public:
    HypnoGrooveAudioProcessor();
    ~HypnoGrooveAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Factory Groove"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    std::atomic<double> displayedBpm { 124.0 };

    uint16_t getLaneMask(int pattern, int lane) const;
    void toggleStep(int pattern, int lane, int step);
    void resetFactoryPatterns();
    bool loadSample(int lane, const juce::File& file);
    void useFactorySample(int lane);
    juce::String sampleName(int lane) const;

    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameterLayout();

private:
    struct SampleData {
        std::vector<float> audio;
        double sourceSampleRate = 44100.0;
    };
    struct Voice {
        std::shared_ptr<const SampleData> sample;
        double position = 0.0;
        double increment = 1.0;
        float gain = 0.0f;
        int lane = 0;
    };
    struct Event {
        int offset = 0;
        int lane = 0;
        float velocity = 1.0f;
    };
    std::array<std::array<std::atomic<uint16_t>, hypno::lanes>, 2> patterns;
    std::array<std::shared_ptr<const SampleData>, hypno::lanes> factorySamples;
    std::array<std::shared_ptr<const SampleData>, hypno::lanes> userSamples;
    std::array<juce::String, hypno::lanes> userSamplePaths;
    mutable juce::CriticalSection pathsLock;
    juce::AudioFormatManager formatManager;
    std::array<Voice, 48> voices;
    double currentSampleRate = 44100.0;
    double expectedNextPpq = -1.0;

    void startVoice(int lane, float velocity);
    void clearVoices();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HypnoGrooveAudioProcessor)
};
