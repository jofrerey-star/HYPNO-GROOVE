#include "PluginEditor.h"
#include <cmath>

namespace {
constexpr int uiWidth = 1220;
constexpr int uiHeight = 735;
constexpr int firstStepX = 251;
constexpr int stepPitch = 46;
constexpr int firstRowY = 221;
constexpr int rowPitch = 46;
const juce::Colour metal { 0xffbac3c5 };
const juce::Colour metalLight { 0xffdbe0de };
const juce::Colour ink { 0xff2e3b42 };
const juce::Colour dark { 0xff2b363e };
const juce::Colour padOff { 0xff43515a };
const juce::Colour lcdBackground { 0xff273a35 };
const juce::Colour lcdText { 0xffffa992 };
const std::array<juce::Colour, hypno::lanes> laneColours {
    juce::Colour { 0xffe6a07d }, juce::Colour { 0xffcb9bc8 },
    juce::Colour { 0xff92bba2 }, juce::Colour { 0xffd7ba7d },
    juce::Colour { 0xff87b0ce }, juce::Colour { 0xffd4a8a1 }
};
const std::array<const char*, 6> laneShort { "KICK", "CLAP", "CLOSED HAT", "OPEN HAT", "SHAKER", "RIM" };
}

void HypnoHardwareLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                     const juce::Colour& background,
                                                     bool highlighted, bool down) {
    const bool isPad = bool(button.getProperties().getWithDefault("stepPad", false));
    auto r = button.getLocalBounds().toFloat().reduced(isPad ? 1.5f : 1.0f);
    const float radius = isPad ? 5.5f : 6.0f;
    g.setColour(juce::Colours::black.withAlpha(0.25f));
    g.fillRoundedRectangle(r.translated(0.8f, 2.0f), radius);
    const auto fill = background.brighter(highlighted ? 0.10f : 0.0f).darker(down ? 0.14f : 0.0f);
    g.setColour(fill);
    g.fillRoundedRectangle(r, radius);
    g.setColour(juce::Colours::white.withAlpha(isPad ? 0.20f : 0.35f));
    g.drawRoundedRectangle(r.reduced(.6f), radius, 1.0f);
    if (isPad && button.getToggleState()) {
        g.setColour(juce::Colours::white.withAlpha(.26f));
        g.fillRoundedRectangle(r.removeFromTop(4.0f).reduced(3.0f, 0.0f), 1.5f);
    }
}

void HypnoHardwareLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width,
                                                int height, float sliderPos, float, float,
                                                const juce::Slider::SliderStyle style,
                                                juce::Slider&) {
    if (style != juce::Slider::LinearVertical) return;
    const float cx = float(x + width) * .5f;
    const float top = float(y) + 7.0f, bottom = float(y + height) - 7.0f;
    g.setColour(juce::Colour { 0xff505b61 });
    g.fillRoundedRectangle(cx - 3, top, 6, bottom - top, 3);
    for (int i = 0; i <= 8; ++i) {
        const float yy = top + (bottom-top)*float(i)/8.f;
        g.setColour(juce::Colour { 0xff677479 });
        g.drawLine(cx - 15, yy, cx - 9, yy, .8f);
        g.drawLine(cx + 9, yy, cx + 15, yy, .8f);
    }
    const auto thumb = juce::Rectangle<float>(cx - 20, sliderPos - 9, 40, 18);
    g.setColour(juce::Colours::black.withAlpha(.25f));
    g.fillRoundedRectangle(thumb.translated(0.8f, 2), 3);
    g.setGradientFill(juce::ColourGradient(juce::Colour { 0xfff1f2ef }, thumb.getX(), thumb.getY(),
                                            juce::Colour { 0xff8d999d }, thumb.getX(), thumb.getBottom(), false));
    g.fillRoundedRectangle(thumb, 3);
    g.setColour(ink);
    g.drawLine(cx - 14, sliderPos, cx + 14, sliderPos, 1.4f);
}

void HypnoHardwareLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width,
                                                int height, float p, float start,
                                                float end, juce::Slider&) {
    const float cx = float(x) + float(width)*.5f;
    const float cy = float(y) + float(height)*.5f;
    const float r = juce::jmin(float(width),float(height)) * .36f;
    g.setColour(juce::Colours::black.withAlpha(.20f));
    g.fillEllipse(cx-r+2, cy-r+4, r*2, r*2);
    juce::Path arc;
    arc.addCentredArc(cx, cy, r+4, r+4, 0.f, start, end, true);
    g.setColour(juce::Colour { 0xff89989c });
    g.strokePath(arc, juce::PathStrokeType(4.f, juce::PathStrokeType::curved,
                                         juce::PathStrokeType::rounded));
    juce::Path progress;
    progress.addCentredArc(cx, cy, r+4, r+4, 0.f, start, start+(end-start)*p, true);
    g.setColour(juce::Colour { 0xffd66d5f });
    g.strokePath(progress, juce::PathStrokeType(4.f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    g.setGradientFill(juce::ColourGradient(metalLight, cx-r, cy-r, juce::Colour { 0xff77878e }, cx+r,cy+r,false));
    g.fillEllipse(cx-r, cy-r, r*2, r*2);
    g.setColour(ink.withAlpha(.75f));
    g.drawEllipse(cx-r, cy-r, r*2, r*2, 1.2f);
    const float angle = start+(end-start)*p;
    g.setColour(ink);
    g.drawLine(cx+std::sin(angle)*(r*.25f), cy-std::cos(angle)*(r*.25f),
               cx+std::sin(angle)*(r*.76f), cy-std::cos(angle)*(r*.76f), 3.0f);
}

HypnoGrooveAudioProcessorEditor::HypnoGrooveAudioProcessorEditor(HypnoGrooveAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p) {
    // CRASH FIX: All child controls MUST exist before setSize() invokes resized().
    setLookAndFeel(&hardwareLook);

    patternBox.addItem("PATTERN A", 1);
    patternBox.addItem("PATTERN B", 2);
    patternBox.setColour(juce::ComboBox::backgroundColourId, dark);
    patternBox.setColour(juce::ComboBox::textColourId, metalLight);
    patternBox.setColour(juce::ComboBox::outlineColourId, ink);
    addAndMakeVisible(patternBox);
    patternAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.parameters, "pattern", patternBox);

    bpmLabel.setText("124.0", juce::dontSendNotification);
    bpmLabel.setFont(juce::Font(juce::FontOptions(35.0f, juce::Font::bold)));
    bpmLabel.setColour(juce::Label::textColourId, lcdText);
    bpmLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(bpmLabel);

    for (int lane = 0; lane < hypno::lanes; ++lane) {
        for (int step = 0; step < 16; ++step) {
            auto pad = std::make_unique<juce::TextButton>();
            pad->getProperties().set("stepPad", true);
            pad->setButtonText("");
            pad->setColour(juce::TextButton::buttonColourId, padOff);
            pad->setTooltip("Toggle " + juce::String(hypno::laneNames[size_t(lane)])
                            + " at step " + juce::String(step+1));
            pad->onClick = [this,lane,step] {
                processor.toggleStep(selectedPattern(), lane, step);
                refreshGrid();
            };
            addAndMakeVisible(*pad);
            steps[size_t(lane)][size_t(step)] = std::move(pad);
        }
        auto load = std::make_unique<juce::TextButton>("LOAD");
        load->setColour(juce::TextButton::buttonColourId, dark);
        load->setColour(juce::TextButton::textColourOffId, metalLight);
        load->onClick = [this,lane] { pickSample(lane); };
        addAndMakeVisible(*load);
        sampleButtons[size_t(lane)] = std::move(load);

        auto factory = std::make_unique<juce::TextButton>("RST");
        factory->setTooltip("Use factory synthesized hit");
        factory->setColour(juce::TextButton::buttonColourId, juce::Colour { 0xff879398 });
        factory->setColour(juce::TextButton::textColourOffId, ink);
        factory->onClick = [this,lane] { processor.useFactorySample(lane); refreshGrid(); };
        addAndMakeVisible(*factory);
        resetButtons[size_t(lane)] = std::move(factory);

        auto fader = std::make_unique<juce::Slider>(juce::Slider::LinearVertical,
                                                    juce::Slider::NoTextBox);
        fader->setTooltip("Volume: " + juce::String(hypno::laneNames[size_t(lane)]));
        addAndMakeVisible(*fader);
        volumeAttachments[size_t(lane)] =
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                processor.parameters, "level" + juce::String(lane), *fader);
        volumeSliders[size_t(lane)] = std::move(fader);
    }

    configureKnob(swingSlider);
    configureKnob(humanSlider);
    configureKnob(chanceSlider);
    configureKnob(masterSlider);
    swingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "swing", swingSlider);
    humanAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "human", humanSlider);
    chanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "chance", chanceSlider);
    masterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "master", masterSlider);

    resetPattern.setColour(juce::TextButton::buttonColourId, juce::Colour { 0xffa24d4a });
    resetPattern.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    resetPattern.onClick = [this] { processor.resetFactoryPatterns(); refreshGrid(); };
    addAndMakeVisible(resetPattern);

    // IMPORTANT: defer callbacks until after the 96 pads and all other controls exist.
    // The APVTS attachment may initialize the ComboBox synchronously.
    patternBox.onChange = [this] { refreshGrid(); repaint(); };

    // These lines must remain AFTER every addAndMakeVisible and all pad allocation.
    setSize(uiWidth, uiHeight);
    refreshGrid();
    startTimerHz(8);
}

HypnoGrooveAudioProcessorEditor::~HypnoGrooveAudioProcessorEditor() {
    stopTimer();
    fileChooser.reset();
    setLookAndFeel(nullptr); // prevent a dangling child LookAndFeel reference
}

void HypnoGrooveAudioProcessorEditor::configureKnob(juce::Slider& s) {
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 78, 18);
    s.setColour(juce::Slider::textBoxTextColourId, ink);
    s.setColour(juce::Slider::textBoxOutlineColourId, metal);
    s.setColour(juce::Slider::textBoxBackgroundColourId, metal);
    addAndMakeVisible(s);
}

int HypnoGrooveAudioProcessorEditor::selectedPattern() const {
    return juce::jlimit(0,1,patternBox.getSelectedId()-1);
}

void HypnoGrooveAudioProcessorEditor::refreshGrid() {
    const int p = selectedPattern();
    displayedPattern = p;
    for (int lane = 0; lane < hypno::lanes; ++lane) {
        const auto mask = processor.getLaneMask(p,lane);
        for (int step = 0; step < 16; ++step) {
            auto& pad = *steps[size_t(lane)][size_t(step)];
            const bool lit = hypno::isActive(mask,step);
            pad.setToggleState(lit,juce::dontSendNotification);
            const auto fill = lit ? laneColours[size_t(lane)]
                                  : (step%4==0 ? juce::Colour { 0xff526068 } : padOff);
            pad.setColour(juce::TextButton::buttonColourId,fill);
        }
        const auto name = processor.sampleName(lane);
        if (displayedNames[size_t(lane)] != name) {
            displayedNames[size_t(lane)] = name;
            sampleButtons[size_t(lane)]->setTooltip("Current sound: " + name + ". Choose WAV/AIFF.");
        }
    }
}

void HypnoGrooveAudioProcessorEditor::timerCallback() {
    bpmLabel.setText(juce::String(processor.displayedBpm.load(std::memory_order_relaxed),1),
                     juce::dontSendNotification);
    if (displayedPattern != selectedPattern()) repaint();
    refreshGrid();
}

void HypnoGrooveAudioProcessorEditor::pickSample(int lane) {
    fileChooser = std::make_unique<juce::FileChooser>(
        "Choose drum sample for " + juce::String(hypno::laneNames[size_t(lane)]),
        juce::File{}, "*.wav;*.aif;*.aiff");
    auto safe = juce::Component::SafePointer<HypnoGrooveAudioProcessorEditor>(this);
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode |
                             juce::FileBrowserComponent::canSelectFiles,
        [safe,lane](const juce::FileChooser& chooser) {
            if (safe == nullptr) return;
            const auto file = chooser.getResult();
            if (file.existsAsFile() && !safe->processor.loadSample(lane,file))
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::WarningIcon,"Couldn't load sample",
                    "Please select a supported WAV or AIFF file.");
            safe->refreshGrid();
        });
}

void HypnoGrooveAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(metal);
    auto full = getLocalBounds().toFloat();
    g.setGradientFill(juce::ColourGradient(metalLight,0,0,metal,0,float(getHeight()),false));
    g.fillRoundedRectangle(full.reduced(7),16.f);
    g.setColour(ink.withAlpha(.48f));
    g.drawRoundedRectangle(full.reduced(7),16.f,2.f);
    // Corner screws, for a hardware-panel feel
    for (auto p : {juce::Point<float>(25,25),juce::Point<float>(1195,25),
                   juce::Point<float>(25,710),juce::Point<float>(1195,710)}) {
        g.setColour(juce::Colour { 0xff677479 });
        g.fillEllipse(p.x-7,p.y-7,14,14);
        g.setColour(metalLight);
        g.drawLine(p.x-4,p.y+2,p.x+4,p.y-2,1.2f);
    }
    g.setColour(ink);
    g.setFont(juce::Font(juce::FontOptions(38.f,juce::Font::bold)));
    g.drawText("HYPNO GROOVE",43,26,455,56,juce::Justification::centredLeft);
    g.setFont(juce::Font(juce::FontOptions(12.f,juce::Font::bold)));
    g.drawText("HARDWARE EDITION   /   RHYTHM ENGINE   /   V1.1",46,86,460,22,
               juce::Justification::centredLeft);
    g.setColour(juce::Colour { 0xff737e82 });
    g.drawLine(40.f,141.f,1180.f,141.f,2.f);
    // Backlit display
    g.setColour(juce::Colour { 0xff65746d });
    g.fillRoundedRectangle(767.f,22.f,414.f,104.f,8.f);
    g.setColour(lcdBackground);
    g.fillRoundedRectangle(776.f,31.f,396.f,86.f,4.f);
    g.setColour(lcdText);
    g.setFont(juce::Font(juce::FontOptions(13.f,juce::Font::bold)));
    g.drawText("HOST TEMPO",800,37,145,19,juce::Justification::centredLeft);
    g.drawText("PAT " + juce::String(selectedPattern()==0?"A":"B"),1033,38,106,17,
               juce::Justification::centredRight);
    g.setFont(juce::Font(juce::FontOptions(12.f)));
    g.drawText("SYNC  /  FL STUDIO",791,91,194,19,juce::Justification::centredLeft);
    g.drawText("BPM",1112,81,47,19,juce::Justification::centredRight);
    g.setColour(ink);
    g.setFont(juce::Font(juce::FontOptions(12.f,juce::Font::bold)));
    g.drawText("ACTIVE PATTERN",514,48,192,20,juce::Justification::centredLeft);
    // Sequencer plate
    g.setColour(juce::Colour { 0xff899699 });
    g.fillRoundedRectangle(35.f,166.f,1150.f,326.f,12.f);
    g.setColour(juce::Colour { 0xffd0d7d6 });
    g.fillRoundedRectangle(40.f,169.f,1140.f,318.f,9.f);
    g.setColour(ink);
    g.setFont(juce::Font(juce::FontOptions(15.f,juce::Font::bold)));
    g.drawText("16-STEP SEQUENCER",55,180,245,20,juce::Justification::centredLeft);
    g.setFont(juce::Font(juce::FontOptions(10.f,juce::Font::bold)));
    g.drawText("SAMPLE      RESET",1003,180,172,20,juce::Justification::centredLeft);
    for(int step=0;step<16;++step) {
        g.setColour(step%4==0 ? ink : juce::Colour { 0xff75848a });
        g.drawText(juce::String(step+1),firstStepX+step*stepPitch,181,39,20,
                   juce::Justification::centred);
    }
    for(int lane=0;lane<hypno::lanes;++lane) {
        const int y=firstRowY+lane*rowPitch;
        if(lane%2==0) {
            g.setColour(juce::Colours::white.withAlpha(.18f));
            g.fillRoundedRectangle(52.f,float(y-4),1113.f,39.f,4.f);
        }
        g.setColour(laneColours[size_t(lane)]);
        g.fillRoundedRectangle(54.f,float(y+5),7.f,20.f,2.f);
        g.setColour(ink);
        g.setFont(juce::Font(juce::FontOptions(12.f,juce::Font::bold)));
        g.drawText(laneShort[size_t(lane)],72,y+2,148,30,juce::Justification::centredLeft);
    }
    // Mixer strip and control panel
    g.setColour(juce::Colour { 0xff899699 });
    g.fillRoundedRectangle(35.f,509.f,538.f,184.f,10.f);
    g.fillRoundedRectangle(585.f,509.f,600.f,184.f,10.f);
    g.setColour(juce::Colour { 0xffd1d8d6 });
    g.fillRoundedRectangle(40.f,513.f,528.f,176.f,8.f);
    g.fillRoundedRectangle(590.f,513.f,590.f,176.f,8.f);
    g.setColour(ink);
    g.setFont(juce::Font(juce::FontOptions(13.f,juce::Font::bold)));
    g.drawText("DRUM MIXER",54,522,270,22,juce::Justification::centredLeft);
    g.drawText("GROOVE CONTROLS",615,522,320,22,juce::Justification::centredLeft);
    const std::array<const char*,6> faderLabels { "KICK", "CLAP", "C.HAT", "O.HAT", "SHAKE", "RIM" };
    for(int i=0;i<hypno::lanes;++i) {
        const int x=65+i*82;
        g.setColour(ink);
        g.setFont(juce::Font(juce::FontOptions(10.f,juce::Font::bold)));
        g.drawText(faderLabels[size_t(i)],x-12,657,65,18,juce::Justification::centred);
        g.setColour(laneColours[size_t(i)]);
        g.fillRoundedRectangle(float(x+13),679.f,22.f,4.f,2.f);
    }
    const std::array<const char*,4> knobLabels { "SWING", "HUMANIZE", "PROBABILITY", "MASTER" };
    for(int i=0;i<4;++i) {
        g.setColour(ink);
        g.setFont(juce::Font(juce::FontOptions(10.f,juce::Font::bold)));
        g.drawText(knobLabels[size_t(i)],628+i*139,649,118,18,juce::Justification::centred);
    }
    g.setColour(ink.withAlpha(.85f));
    g.setFont(juce::Font(juce::FontOptions(11.f)));
    g.drawText("A/B patterns   â€¢   WAV/AIFF loading   â€¢   FL Studio host sync   â€¢   Six individual levels",
               52,698,1030,16,juce::Justification::centredLeft);
}

void HypnoGrooveAudioProcessorEditor::resized() {
    // Only invoked after all steps, sliders and buttons have been created.
    patternBox.setBounds(514,76,178,34);
    bpmLabel.setBounds(895,55,209,47);
    for(int lane=0;lane<hypno::lanes;++lane) {
        const int y=firstRowY+lane*rowPitch;
        for(int step=0;step<16;++step)
            steps[size_t(lane)][size_t(step)]->setBounds(firstStepX+step*stepPitch,y,39,31);
        sampleButtons[size_t(lane)]->setBounds(1010,y,92,31);
        resetButtons[size_t(lane)]->setBounds(1110,y,56,31);
        volumeSliders[size_t(lane)]->setBounds(64+lane*82,553,48,98);
    }
    swingSlider.setBounds(626,552,118,101);
    humanSlider.setBounds(765,552,118,101);
    chanceSlider.setBounds(904,552,118,101);
    masterSlider.setBounds(1043,552,118,101);
    resetPattern.setBounds(1012,690,153,24);
}
