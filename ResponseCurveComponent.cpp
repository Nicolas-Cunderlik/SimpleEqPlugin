#include "ResponseCurveComponent.h"
#include "ParamIDs.h"

ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor& p)
    : audioProcessor(p)
{
    audioProcessor.apvts.addParameterListener(ParamIDs::lowCutFreq, this);
    audioProcessor.apvts.addParameterListener(ParamIDs::highCutFreq, this);
    audioProcessor.apvts.addParameterListener(ParamIDs::peakFreq, this);
    audioProcessor.apvts.addParameterListener(ParamIDs::peakGain, this);
    audioProcessor.apvts.addParameterListener(ParamIDs::peakQuality, this);
    audioProcessor.apvts.addParameterListener(ParamIDs::lowCutSlope, this);
    audioProcessor.apvts.addParameterListener(ParamIDs::highCutSlope, this);

	lastParameterChangeMs = juce::Time::getMillisecondCounter();
    repaint();
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    stopTimer();

    audioProcessor.apvts.removeParameterListener(ParamIDs::lowCutFreq, this);
    audioProcessor.apvts.removeParameterListener(ParamIDs::highCutFreq, this);
    audioProcessor.apvts.removeParameterListener(ParamIDs::peakFreq, this);
    audioProcessor.apvts.removeParameterListener(ParamIDs::peakGain, this);
    audioProcessor.apvts.removeParameterListener(ParamIDs::peakQuality, this);
    audioProcessor.apvts.removeParameterListener(ParamIDs::lowCutSlope, this);
    audioProcessor.apvts.removeParameterListener(ParamIDs::highCutSlope, this);
}

void ResponseCurveComponent::resized()
{
}

void ResponseCurveComponent::timerCallback()
{
    repaint();

    auto elapsedMs = juce::Time::getMillisecondCounter() - lastParameterChangeMs;
    if (elapsedMs > 1000)
        stopTimer();
}

void ResponseCurveComponent::parameterChanged(const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused(parameterID, newValue);
	lastParameterChangeMs = juce::Time::getMillisecondCounter();
    triggerAsyncUpdate();
}

void ResponseCurveComponent::handleAsyncUpdate()
{
    startTimerHz(30);
    repaint();
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    const auto bounds = getLocalBounds();
    const auto width = bounds.getWidth();
    const auto height = bounds.getHeight();

    juce::Path responseCurve;

    const auto sampleRate = (float)audioProcessor.getSampleRate();
    if (width <= 0 || sampleRate <= 0.0f)
        return;

    const auto settings = getChainSettings(audioProcessor.apvts);

    const auto peakGain = juce::Decibels::decibelsToGain(settings.peakGainInDecibels);

    auto peakCoefficients =
        juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sampleRate,
            settings.peakFreq,
            settings.peakQuality,
            peakGain);

    auto lowCutBiquads =
        BandPass::designButterworthCutFilter(
            settings.lowCutFreq,
            sampleRate,
            EQConstants::slopeToOrder(settings.lowCutSlope),
            BandPass::FilterType::Highpass);

    auto lowCutCoefficients = BandPass::convertToCoefficients(lowCutBiquads);

    auto highCutBiquads =
        BandPass::designButterworthCutFilter(
            settings.highCutFreq,
            sampleRate,
            EQConstants::slopeToOrder(settings.highCutSlope),
            BandPass::FilterType::Lowpass);

    auto highCutCoefficients = BandPass::convertToCoefficients(highCutBiquads);

    std::vector<float> mags((size_t)width);

    // Grid lines
    g.setColour(juce::Colours::grey.withAlpha(0.5f));

    for (auto f : EQConstants::frequenciesHz)
    {
        const float normX =
            juce::mapFromLog10(f,
                EQConstants::minFrequencyHz,
                EQConstants::maxFrequencyHz);

        const float x = bounds.getX() + normX * (float)width;

        g.drawLine(x,
            (float)bounds.getY(),
            x,
            (float)bounds.getBottom() + 10.0f,
            1.0f);
    }

    // Magnitude calculation
    for (size_t i = 0; i < mags.size(); ++i)
    {
        const float normX = (float)i / (float)width;
        const float freq =
            juce::mapToLog10(normX,
                (float)EQConstants::minFrequencyHz,
                (float)EQConstants::maxFrequencyHz);

        float mag = 1.0f;

        mag *= peakCoefficients->getMagnitudeForFrequency(freq, sampleRate);

        for (auto* c : lowCutCoefficients)
            mag *= c->getMagnitudeForFrequency(freq, sampleRate);

        for (auto* c : highCutCoefficients)
            mag *= c->getMagnitudeForFrequency(freq, sampleRate);

        mags[i] = juce::Decibels::gainToDecibels(mag);
    }

    responseCurve.startNewSubPath(
        (float)bounds.getX(),
        juce::jmap(mags[0],
            -24.0f, 24.0f,
            (float)bounds.getBottom(),
            (float)bounds.getY()));

    for (size_t i = 1; i < mags.size(); ++i)
    {
        const float y =
            juce::jmap(mags[i],
                -24.0f, 24.0f,
                (float)bounds.getBottom(),
                (float)bounds.getY());

        responseCurve.lineTo(bounds.getX() + (float)i, y);
    }

    g.setColour(juce::Colours::white);
    g.strokePath(responseCurve, juce::PathStrokeType(2.0f));

    const float fadeHeight = height * 0.25f;

    juce::ColourGradient fade(
        juce::Colours::transparentBlack,
        0.0f, height - fadeHeight,
        juce::Colours::black.withAlpha(1.0f),
        0.0f, height,
        false);

    g.setGradientFill(fade);
    g.fillRect(bounds.withTop((int)(height - fadeHeight)));
}