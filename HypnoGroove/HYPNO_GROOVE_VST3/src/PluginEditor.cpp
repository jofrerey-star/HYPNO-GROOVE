#include "PluginEditor.h"
#include <cmath>

namespace {
const juce::Colour background { 0xff151b20 };
const juce::Colour panel { 0xff242e35 };
const juce::Colour stepOff { 0xff334048 };
const juce::Colour textMain { 0xfff1f4ee };
const juce::Colour textMuted { 0xff9aaca9 };
const std::array<juce::Colour, hypno::lanes> laneColors {
    juce::Colour { 0xffe4a082 }, juce::Colour { 0xffca96d8 },
    juce::Colour { 0xff95caa6 }, juce::Colour { 0xffd9bb76 },
    juce::Colour { 0xff80b3d6 }, juce::Colour { 0xffd19d97 }
};
}

HypnoGrooveAudioProcessorEditor::HypnoGrooveAudioProcessorEditor(HypnoGrooveAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p) {
    setSize(1090, 575);

    patternBox.addItem("PATTERN A", 1);
    patternBox.addItem("PATTERN B", 2);
    patternBox.setColour(juce::ComboBox::backgroundColourId, panel);
    patternBox.setColour(juce::ComboBox::textColourId, textMain);
    addAndMakeVisible(patternBox);
    patternAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.parameters, "pattern", patternBox);

    bpmLabel.setText("124 BPM", juce::dontSendNotification);
    bpmLabel.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
    bpmLabel.setColour(juce::Label::textColourId, juce::Colour { 0xff8ed5b3 });
    bpmLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(bpmLabel);

    for (int lane = 0; lane < hypno::lanes; ++lane) {
        for (int step = 0; step < 16; ++step) {
            auto button = std::make_unique<juce::TextButton>();
            button->setButtonText("");
            button->setColour(juce::TextButton::buttonColourId, stepOff);
            button->setColour(juce::TextButton::buttonOnColourId, laneColors[size_t(lane)]);
            button->setTooltip("Click to toggle step " + juce::String(step + 1));
            button->onClick = [this, lane, step] {
                processor.toggleStep(selectedPattern(), lane, step);
                refreshGrid();
            };
            addAndMakeVisible(*button);
            steps[size_t(lane)][size_t(step)] = std::move(button);
        }

        auto sampleButton = std::make_unique<juce::TextButton>("LOAD");
        sampleButton->setTooltip("Load a WAV or AIFF sample");
        sampleButton->setColour(juce::TextButton::buttonColourId, panel);
        sampleButton->setColour(juce::TextButton::textColourOffId, textMain);
        sampleButton->onClick = [this, lane] { pickSample(lane); };
        addAndMakeVisible(*sampleButton);
        sampleButtons[size_t(lane)] = std::move(sampleButton);

        auto factoryButton = std::make_unique<juce::TextButton>("↺");
        factoryButton->setTooltip("Return to built-in sound");
        factoryButton->setColour(juce::TextButton::buttonColourId, panel);
        factoryButton->onClick = [this, lane] {
            processor.useFactorySample(lane);
            refreshGrid();
        };
        addAndMakeVisible(*factoryButton);
        resetButtons[size_t(lane)] = std::move(factoryButton);

        auto slider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal,
                                                     juce::Slider::NoTextBox);
        slider->setColour(juce::Slider::trackColourId, laneColors[size_t(lane)]);
        slider->setColour(juce::Slider::thumbColourId, textMain);
        slider->setTooltip("Level for " + juce::String(hypno::laneNames[size_t(lane)]));
        addAndMakeVisible(*slider);
        volumeAttachments[size_t(lane)] =
            std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                processor.parameters, "level" + juce::String(lane), *slider);
        volumeSliders[size_t(lane)] = std::move(slider);
    }

    configureBottomSlider(swingSlider);
    configureBottomSlider(humanSlider);
    configureBottomSlider(chanceSlider);
    configureBottomSlider(masterSlider);
    swingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "swing", swingSlider);
    humanAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "human", humanSlider);
    chanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "chance", chanceSlider);
    masterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "master", masterSlider);

    resetPattern.setColour(juce::TextButton::buttonColourId, panel);
    resetPattern.setColour(juce::TextButton::textColourOffId, textMain);
    resetPattern.onClick = [this] { processor.resetFactoryPatterns(); refreshGrid(); };
    addAndMakeVisible(resetPattern);
    refreshGrid();
    startTimerHz(8);
}

HypnoGrooveAudioProcessorEditor::~HypnoGrooveAudioProcessorEditor() {
    stopTimer();
    fileChooser.reset();
}

void HypnoGrooveAudioProcessorEditor::configureBottomSlider(juce::Slider& slider) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 19);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour { 0xff87c6a5 });
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, panel);
    slider.setColour(juce::Slider::textBoxTextColourId, textMain);
    slider.setColour(juce::Slider::textBoxOutlineColourId, background);
    addAndMakeVisible(slider);
}

int HypnoGrooveAudioProcessorEditor::selectedPattern() const {
    return juce::jlimit(0, 1, patternBox.getSelectedId() - 1);
}

void HypnoGrooveAudioProcessorEditor::refreshGrid() {
    const int pattern = selectedPattern();
    displayedPattern = pattern;
    for (int lane = 0; lane < hypno::lanes; ++lane) {
        const auto mask = processor.getLaneMask(pattern, lane);
        for (int step = 0; step < 16; ++step) {
            auto& button = *steps[size_t(lane)][size_t(step)];
            const auto active = hypno::isActive(mask, step);
            const auto base = (step % 4 == 0) ? juce::Colour { 0xff3c4a50 } : stepOff;
            const auto desired = active ? laneColors[size_t(lane)] : base;
            if (button.findColour(juce::TextButton::buttonColourId) != desired)
                button.setColour(juce::TextButton::buttonColourId, desired);
        }
        const auto name = processor.sampleName(lane);
        if (displayedNames[size_t(lane)] != name) {
            displayedNames[size_t(lane)] = name;
            sampleButtons[size_t(lane)]->setTooltip("Current: " + name + ". Click to load WAV/AIFF.");
        }
    }
}

void HypnoGrooveAudioProcessorEditor::timerCallback() {
    bpmLabel.setText(juce::String(processor.displayedBpm.load(), 1) + " BPM",
                     juce::dontSendNotification);
    refreshGrid();
}

void HypnoGrooveAudioProcessorEditor::pickSample(int lane) {
    fileChooser = std::make_unique<juce::FileChooser>(
        "Choose drum sample for " + juce::String(hypno::laneNames[size_t(lane)]),
        juce::File{}, "*.wav;*.aif;*.aiff");
    auto safe = juce::Component::SafePointer<HypnoGrooveAudioProcessorEditor>(this);
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode |
                                 juce::FileBrowserComponent::canSelectFiles,
        [safe, lane](const juce::FileChooser& chooser) {
            if (safe == nullptr) return;
            const auto file = chooser.getResult();
            if (file.existsAsFile() && !safe->processor.loadSample(lane, file))
                juce::AlertWindow::showMessageBoxAsync(
                    juce::MessageBoxIconType::WarningIcon, "Couldn't load sample",
                    "Please choose an uncompressed WAV or AIFF audio file.");
            safe->refreshGrid();
        });
}

void HypnoGrooveAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(background);
    g.setColour(textMain);
    g.setFont(juce::Font(juce::FontOptions(30.f, juce::Font::bold)));
    g.drawText("HYPNO GROOVE", 24, 15, 380, 40, juce::Justification::centredLeft);
    g.setColour(textMuted);
    g.setFont(juce::Font(juce::FontOptions(12.f)));
    g.drawText("HOUSE RHYTHM ENGINE  /  SIX CHANNEL DRUM MACHINE", 27, 54, 500, 22,
               juce::Justification::centredLeft);
    g.setColour(juce::Colour { 0xff35453e });
    g.drawLine(20.f, 94.f, float(getWidth() - 20), 94.f, 1.5f);

    for (int step = 0; step < 16; ++step) {
        g.setColour(step % 4 == 0 ? textMain : textMuted);
        g.setFont(juce::Font(juce::FontOptions(11.f, step % 4 == 0 ? juce::Font::bold : 0)));
        g.drawText(juce::String(step + 1), 177 + step * 45, 103, 38, 20,
                   juce::Justification::centred);
    }
    for (int lane = 0; lane < hypno::lanes; ++lane) {
        const int y = 132 + lane * 47;
        g.setColour(laneColors[size_t(lane)]);
        g.fillRoundedRectangle(22.f, float(y + 5), 4.f, 22.f, 2.f);
        g.setColour(textMain);
        g.setFont(juce::Font(juce::FontOptions(12.f, juce::Font::bold)));
        g.drawText(hypno::laneNames[size_t(lane)], 33, y + 2, 140, 31,
                   juce::Justification::centredLeft);
    }
    g.setColour(juce::Colour { 0xff35453e });
    g.drawLine(20.f, 428.f, float(getWidth() - 20), 428.f, 1.5f);
    const std::array<const char*, 4> titles { "SWING", "HUMANIZE", "PROBABILITY", "MASTER" };
    for (int i = 0; i < 4; ++i) {
        g.setColour(textMuted);
        g.setFont(juce::Font(juce::FontOptions(12.f, juce::Font::bold)));
        g.drawText(titles[size_t(i)], 165 + i * 174, 438, 148, 18,
                   juce::Justification::centred);
    }
    g.setColour(textMuted);
    g.setFont(juce::Font(juce::FontOptions(10.f)));
    g.drawText("Follows FL Studio transport / tempo  •  Load custom samples or use built-in sounds",
               26, 547, 960, 18, juce::Justification::centredLeft);
}

void HypnoGrooveAudioProcessorEditor::resized() {
    patternBox.setBounds(780, 30, 144, 32);
    bpmLabel.setBounds(929, 31, 133, 31);
    for (int lane = 0; lane < hypno::lanes; ++lane) {
        const int y = 132 + lane * 47;
        volumeSliders[size_t(lane)]->setBounds(96, y + 28, 76, 15);
        for (int step = 0; step < 16; ++step)
            steps[size_t(lane)][size_t(step)]->setBounds(178 + step * 45, y, 39, 31);
        sampleButtons[size_t(lane)]->setBounds(904, y, 78, 31);
        resetButtons[size_t(lane)]->setBounds(991, y, 39, 31);
    }
    swingSlider.setBounds(165, 457, 148, 83);
    humanSlider.setBounds(339, 457, 148, 83);
    chanceSlider.setBounds(513, 457, 148, 83);
    masterSlider.setBounds(687, 457, 148, 83);
    resetPattern.setBounds(870, 482, 163, 33);
}
