#pragma once
#include <JuceHeader.h>
#include <atomic>
#include "PluginProcessor.h"
#include "EQConstants.h"

// This class represents the spectrum visualizer component of the GUI.
// It uses a timer to update the visualizer at a fixed rate,
// reading from a FIFO buffer that the audio processor fills with the latest filtered audio data.
// -------------------------------------------------------------------------------------------------
// The visualizer performs a STFT on the audio data, with an expected 50% overlap between
// consecutive transforms, but the overlap is not guaranteed if some buffers are dropped or if any
// buffer is not completely filled at any respective point in the process. In fact, the overlap
// is extremely unlikely because this runs at just 60Hz.
class SpectrumVisualizerComponent : public juce::Component
    , private juce::Timer
{
public:
    SpectrumVisualizerComponent(SimpleEQAudioProcessor&);
    ~SpectrumVisualizerComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    const uint8_t timerSpeedHz = 60;
    void timerCallback() override;

    bool computeSTFT() noexcept;
	bool readFromFifo(juce::AudioBuffer<float>& destBuffer) noexcept;
    juce::AudioBuffer<float> STFTBuffer{ 1, EQConstants::STFTSize * 2};
	juce::dsp::FFT fft;
	juce::dsp::WindowingFunction<float> window;

    SimpleEQAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumVisualizerComponent)
};
