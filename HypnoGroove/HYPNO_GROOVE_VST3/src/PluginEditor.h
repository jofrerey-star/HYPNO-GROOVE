#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>

// Hardware-style look for a *real* JUCE interface, not a bitmap overlay.
// All controls remain standard interactive JUCE components and APVTS attachments.
class HypnoHardwareLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&,
                              bool highlighted, bool down) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle, juce::Slider&) override;
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;
};

class HypnoGrooveAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              private juce::Timer {
public:
    explicit HypnoGrooveAudioProcessorEditor(HypnoGrooveAudioProcessor&);
    ~HypnoGrooveAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    HypnoGrooveAudioProcessor& processor;
    HypnoHardwareLookAndFeel hardwareLook;
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
    juce::TextButton resetPattern { "RESET A/B" };
    juce::Label bpmLabel;
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::array<juce::String, hypno::lanes> displayedNames;
    int displayedPattern = -1;

    void timerCallback() override;
    void refreshGrid();
    void pickSample(int lane);
    void configureKnob(juce::Slider& slider);
    int selectedPattern() const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HypnoGrooveAudioProcessorEditor)
};
