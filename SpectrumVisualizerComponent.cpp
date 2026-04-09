#include "SpectrumVisualizerComponent.h"

SpectrumVisualizerComponent::SpectrumVisualizerComponent(SimpleEQAudioProcessor& p)
    : audioProcessor(p)
	, fft(EQConstants::STFTOrder)
	, window(EQConstants::STFTSize, juce::dsp::WindowingFunction<float>::hann)
{
    startTimerHz(timerSpeedHz);
}

SpectrumVisualizerComponent::~SpectrumVisualizerComponent()
{
    stopTimer();
}

void SpectrumVisualizerComponent::resized()
{
}

void SpectrumVisualizerComponent::timerCallback()
{
    repaint();
}

void SpectrumVisualizerComponent::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds();
    const auto width = bounds.getWidth();

    const auto sampleRate = audioProcessor.getSampleRate();
    if (width <= 0 || sampleRate <= 0.0)
        return;

    if (!computeSTFT())
        return;

    juce::Path responseCurve;

    const auto numBins = EQConstants::STFTSize / 2;
    const float binWidth = (float)(sampleRate) / (float)(EQConstants::STFTSize);

    const float leftX = (float)bounds.getX();
    const float leftY = juce::jmap(-80.0f,
        -80.0f, 0.0f,
        (float)bounds.getBottom(),
        (float)bounds.getY());

    responseCurve.startNewSubPath(leftX, leftY);

    for (size_t i = 1; i < numBins; ++i)
    {
        const float freq = (float)(i) * binWidth;

        const float normX = juce::mapFromLog10(freq,
            EQConstants::minFrequencyHz,
            EQConstants::maxFrequencyHz);

        const float x = (float)(bounds.getX()) + normX * (float)(bounds.getWidth());

		// Tilt 3db per octave, normalized at 1kHz, to make the spectrum more visually balanced
		const float tilt = 3.0f * std::log2(std::max(freq, 1.0f) / 1000.0f);
        const float mag = STFTBuffer.getSample(0, (int)(i)) + tilt;
        const float y = juce::jmap(mag,
            -80.0f, 0.0f,
            (float)(bounds.getBottom()),
            (float)(bounds.getY()));

		responseCurve.lineTo(x, y);
    }

    g.setColour(juce::Colours::cyan);
    g.strokePath(responseCurve, juce::PathStrokeType(2.0f));
}

// Calculates an array of magnitudes for the frequency bins of the STFT
bool SpectrumVisualizerComponent::computeSTFT() noexcept
{
    if (!readFromFifo(STFTBuffer))
        return false;

    window.multiplyWithWindowingTable(STFTBuffer.getWritePointer(0), EQConstants::STFTSize);
    fft.performFrequencyOnlyForwardTransform(STFTBuffer.getWritePointer(0));

    for (size_t i = 0; i < EQConstants::STFTSize; ++i)
    {
        float mag = STFTBuffer.getSample(0, i);
        mag /= EQConstants::STFTSize;
        mag = juce::Decibels::gainToDecibels(mag);
        mag = std::max(mag, -100.0f);
        mag = std::min(mag, 0.0f);
        STFTBuffer.setSample(0, i, mag);
    }

    return true;
}

bool SpectrumVisualizerComponent::readFromFifo(juce::AudioBuffer<float>& destBuffer) noexcept
{
    auto& fifo = audioProcessor.getFilteredBufferFifo();
    auto& buffer = audioProcessor.getFilteredMonoBuffer();

    if (fifo.getNumReady() < EQConstants::STFTSize)
        return false;

    auto readHandle = fifo.read(destBuffer.getNumSamples());

    for (int i = 0; i < destBuffer.getNumSamples(); ++i)
    {
        if (i < readHandle.blockSize1)
            destBuffer.setSample(0, i, buffer.getSample(0, readHandle.startIndex1 + i));

        else if (i < readHandle.blockSize1 + readHandle.blockSize2)
            destBuffer.setSample(0, i, buffer.getSample(0, readHandle.startIndex2 + (i - readHandle.blockSize1)));
        
        else
            destBuffer.setSample(0, i, 0.0f);
    }

    return true;
}
