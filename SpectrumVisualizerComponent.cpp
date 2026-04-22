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
    if (!computeSTFT())
        return;

	auto sampleRate = audioProcessor.getSampleRate();
	if (sampleRate <= 0.0)
        return;

    repaint();
}

void SpectrumVisualizerComponent::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds();
    const auto width = bounds.getWidth();

    juce::Path responseCurve;

    const auto numBins = EQConstants::STFTSize / 2;
    const float binWidth = audioProcessor.getSampleRate() / (float)(EQConstants::STFTSize);

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

		// Tilt 3.5dB per octave, normalized at 1kHz, to make the spectrum more visually balanced
		const float tilt = 3.5f * std::log2(std::max(freq, 1.0f) / 1000.0f);
        const float mag = STFTBuffer.getSample(0, (int)(i)) + tilt;
        const float y = juce::jmap(mag,
            -80.0f, 0.0f,
            (float)(bounds.getBottom()),
            (float)(bounds.getY()));

		responseCurve.lineTo(x, y);
    }

    g.setColour(juce::Colours::white.withAlpha(0.75f));
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
        mag = juce::Decibels::gainToDecibels(mag / EQConstants::STFTSize);
        mag = std::clamp(mag, -100.0f, 0.0f);
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

    for (size_t i = 0; i < destBuffer.getNumSamples(); ++i)
    {
        if (i < readHandle.blockSize1)
            destBuffer.setSample(0, i, buffer.getSample(0, readHandle.startIndex1 + i));

        else if (i < readHandle.blockSize1 + readHandle.blockSize2) // Yes redundant but more readable
            destBuffer.setSample(0, i, buffer.getSample(0, readHandle.startIndex2 + (i - readHandle.blockSize1)));
    }

    return true;
}
