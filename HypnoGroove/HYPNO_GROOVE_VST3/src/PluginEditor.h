#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class HypnoGrooveAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              private juce::Timer {
public:
    explicit HypnoGrooveAudioProcessorEditor(HypnoGrooveAudioProcessor&);
    ~HypnoGrooveAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    HypnoGrooveAudioProcessor& processor;
    std::array<std::array<std::unique_ptr<juce::TextButton>, 16>, hypno::lanes> steps;
    std::array<std::unique_ptr<juce::TextButton>, hypno::lanes> sampleButtons;
    std::array<std::unique_ptr<juce::TextButton>, hypno::lanes> resetButtons;
    std::array<std::unique_ptr<juce::Slider>, hypno::lanes> volumeSliders;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, hypno::lanes> volumeAttachments;
    juce::ComboBox patternBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> patternAttachment;
    juce::Slider swingSlider, humanSlider, chanceSlider, masterSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> swingAttachment,
        humanAttachment, chanceAttachment, masterAttachment;
    juce::TextButton resetPattern { "RESET PATTERNS" };
    juce::Label bpmLabel;
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::array<juce::String, hypno::lanes> displayedNames;
    int displayedPattern = -1;

    void timerCallback() override;
    void refreshGrid();
    void pickSample(int lane);
    void configureBottomSlider(juce::Slider& slider);
    int selectedPattern() const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HypnoGrooveAudioProcessorEditor)
};
