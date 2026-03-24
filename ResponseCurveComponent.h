#pragma once
#include <JuceHeader.h>
#include <atomic>
#include "PluginProcessor.h"
#include "EQConstants.h"

class ResponseCurveComponent : public juce::Component,
    private juce::Timer,
    private juce::AudioProcessorValueTreeState::Listener,
    private juce::AsyncUpdater
{
public:
    ResponseCurveComponent(SimpleEQAudioProcessor&);
    ~ResponseCurveComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;

    SimpleEQAudioProcessor& audioProcessor;
    uint32_t lastParameterChangeMs = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResponseCurveComponent)
};
