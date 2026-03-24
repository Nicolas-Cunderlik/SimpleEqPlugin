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
#include "EQConstants.h"
#include "ParamIDs.h"

//==============================================================================
/**
*/
class SimpleEQAudioProcessorEditor  : public juce::AudioProcessorEditor
                                   , private juce::AudioProcessorValueTreeState::Listener
                                   , private juce::AsyncUpdater
{
public:
    SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor&);
    ~SimpleEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;

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

    std::array<juce::ToggleButton, EQConstants::slopeDbPerOct.size()> lowCutButtons;
    std::array<juce::ToggleButton, EQConstants::slopeDbPerOct.size()> highCutButtons;
    juce::Label lowCutSlopeUnitLabel, highCutSlopeUnitLabel;

    juce::Label lowCutTitle;
    juce::Label peakTitle;
    juce::Label highCutTitle;

    std::array<juce::Label, EQConstants::frequenciesHz.size()> frequencyLabels;

    // Attachments to link GUI elements with parameters in the processor
    juce::AudioProcessorValueTreeState::SliderAttachment highcutAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment lowcutAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment peakFilterAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment peakGainAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment peakQualityAttachment;

    // Styling helpers
    void setupTitle(juce::Label&, const juce::String&);
    void styleSlider(juce::Slider&, juce::Slider::SliderStyle);
    void createFrequencyLabels();

    void createSlopeButtons(std::array<juce::ToggleButton, EQConstants::slopeDbPerOct.size()>& buttons,
                            const juce::String& paramID,
                            int radioGroupId);
    void slopeButtonClicked(const juce::String& paramID, int index);
    void syncSlopeButtons();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};
