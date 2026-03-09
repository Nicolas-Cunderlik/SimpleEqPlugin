/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ResponseCurveComponent.h"
#include "EQLookAndFeel.h"

//==============================================================================
/**
*/
class SimpleEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor&);
    ~SimpleEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    const std::array<float, 9> freqs
    {
        20.f, 50.f, 100.f, 200.f, 500.f, 1000.f, 5000.f, 10000.f, 20000.f
    };

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEQAudioProcessor& audioProcessor;

    // Visual representation of the EQ state
    ResponseCurveComponent responseCurve;

    // Custom GUI styling
    EQLookAndFeel eqLookAndFeel;

    // GUI Components
    juce::Slider highcutSlider;
    juce::Slider lowcutSlider;
    juce::Slider peakFilterSlider;
    juce::Slider peakGainSlider;     // Range: -36 to +36
    juce::Slider peakQualitySlider;

    std::vector<std::unique_ptr<juce::ToggleButton>> lowCutButtons, highCutButtons;
    juce::FlexBox lowCutSlopeFlex, highCutSlopeFlex;
    juce::Label lowCutSlopeUnitLabel, highCutSlopeUnitLabel;

    juce::Label lowCutTitle;
    juce::Label peakTitle;
    juce::Label highCutTitle;

    std::vector<std::unique_ptr<juce::Label>> frequencyLabels;

    // Attachments to link GUI elements with parameters in the processor
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> highcutAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowcutAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakFilterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakQualityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> highcutSlopeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lowcutSlopeAttachment;

    // Define primary colours from your hex codes
    juce::Colour primaryColor1{ juce::Colour::fromString("#09122C") };
    juce::Colour primaryColor2{ juce::Colour::fromString("#872341") };
    juce::Colour primaryColor3{ juce::Colour::fromString("#BE3144") };
    juce::Colour primaryColor4{ juce::Colour::fromString("#E17564") };

    // Styling helpers
    void styleSlider(juce::Slider&);
    void createFrequencyLabels();
    void createSlopeButtons(std::vector<std::unique_ptr<juce::ToggleButton>>& buttons,
        const juce::String& paramID,
        int radioGroupId);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};