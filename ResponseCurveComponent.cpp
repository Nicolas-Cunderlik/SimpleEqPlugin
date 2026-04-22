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
    if (elapsedMs > noParameterChangeThresholdMs)
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
    startTimerHz(timerSpeedHz);
    populateCoefficients();
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
	const float sampleRate = audioProcessor.getSampleRate();
	if (sampleRate <= 0.0f)
        return;

    if (!peakCoefficients || !lowCutCoefficients.size() || !highCutCoefficients.size())
		populateCoefficients();

    const auto bounds = getLocalBounds();
    const auto width = bounds.getWidth();
    const auto height = bounds.getHeight();

    juce::Path responseCurve;

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
    std::vector<float> mags((size_t)width);
    for (size_t i = 0; i < mags.size(); ++i)
    {
        const float normX = (float)i / (float)width;
        const float freq =
            juce::mapToLog10(normX,
 							 (float)EQConstants::minFrequencyHz,
							 (float)EQConstants::maxFrequencyHz);
		float mag = getMagnitudeForFrequency(freq);
    }

    responseCurve.startNewSubPath(
        (float)bounds.getX(),
        juce::jmap(mags[0],
                   -24.0f, 24.0f,
                   (float)bounds.getBottom(),
                   (float)bounds.getY()));

    for (size_t i = 1; i < mags.size(); ++i)
    {
        const float y = juce::jmap(mags[i],
								   -24.0f, 24.0f,
								   (float)bounds.getBottom(),
								   (float)bounds.getY());

        responseCurve.lineTo(bounds.getX() + (float)i, y);
    }

    g.setColour(juce::Colours::cyan);
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

float ResponseCurveComponent::getMagnitudeForFrequency(float freq) const
{
    const float sampleRate = audioProcessor.getSampleRate();
    if (sampleRate <= 0.0f)
        return 1.0f;

    float mag = 1.0f;
    mag *= peakCoefficients->getMagnitudeForFrequency(freq, sampleRate);
    for (auto* c : lowCutCoefficients)
        mag *= c->getMagnitudeForFrequency(freq, sampleRate);
    for (auto* c : highCutCoefficients)
        mag *= c->getMagnitudeForFrequency(freq, sampleRate);
    return mag;
}

// All this just to show a fucking curve because otherwise there's a data race with the audio thread
void ResponseCurveComponent::populateCoefficients()
{
	const auto& settings = getChainSettings(audioProcessor.apvts);
	const float sampleRate = audioProcessor.getSampleRate();
	if (sampleRate <= 0.0f)
        return;

	peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate,
        settings.peakFreq,
		settings.peakQuality,
		juce::Decibels::decibelsToGain(settings.peakGainInDecibels));

	SimpleEQAudioProcessor::initializeCoefficients(lowCutCoefficients);
	SimpleEQAudioProcessor::initializeCoefficients(highCutCoefficients);
    auto lowCutBiquads = IIRFilter::designButterworthCutFilter(
        settings.lowCutFreq,
		sampleRate,
		EQConstants::slopeEnumToOrder(settings.lowCutSlope),
		IIRFilter::FilterType::Highpass);
    auto highCutBiquads = IIRFilter::designButterworthCutFilter(
        settings.highCutFreq,
		sampleRate,
		EQConstants::slopeEnumToOrder(settings.highCutSlope),
		IIRFilter::FilterType::Lowpass);
	IIRFilter::populateCoefficients(lowCutBiquads, lowCutCoefficients);
	IIRFilter::populateCoefficients(highCutBiquads, highCutCoefficients);
}