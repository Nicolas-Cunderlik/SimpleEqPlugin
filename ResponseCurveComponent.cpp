#include "ResponseCurveComponent.h"

ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor& p)
    : audioProcessor(p)
{
    startTimerHz(24); // Refresh rate
}

ResponseCurveComponent::~ResponseCurveComponent()
{
}

void ResponseCurveComponent::resized()
{
}

void ResponseCurveComponent::timerCallback()
{
    repaint();
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    using namespace juce;

    g.fillAll(Colours::black);

    auto bounds = getLocalBounds();
    auto responseArea = bounds;

    Path responseCurve;

    auto width = responseArea.getWidth();
    auto sampleRate = audioProcessor.getSampleRate();
    auto settings = getChainSettings(audioProcessor.apvts);

    auto peakCoefficients =
        dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                                                      settings.peakFreq,
                                                      settings.peakQuality,
                                                      Decibels::decibelsToGain(settings.peakGainInDecibels));
    auto lowCutCoefficients = dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, settings.lowCutFreq);
    auto highCutCoefficients = dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, settings.highCutFreq);

    std::vector<double> mags;
    mags.resize(width);

    // Grid lines for visualizing the frequency range
    std::array<float, 9> freqs
    {
        20, 50, 100, 200, 500, 1000, 5000, 10000, 20000
    };
    g.setColour(Colours::grey.withAlpha(0.5f));

    for (auto f : freqs)
    {
        auto normX = mapFromLog10(f, 20.f, 20000.f);
        float x = responseArea.getX() + normX * responseArea.getWidth();

        g.drawLine(x,
                   (float)responseArea.getY(),
                   x,
                   (float)responseArea.getBottom() + 10.0f,
                   1.0f);
    }
    
    // Magnitude representing the filter states
    for (int i = 0; i < width; i++)
    {
        double mag = 1.0;
        double normX = (double)i / (double)width;
        double freq = mapToLog10(normX, 20.0, 20000.0);

        // 1. Peak is always the same
        mag *= peakCoefficients->getMagnitudeForFrequency(freq, sampleRate);

        int lcOrder = settings.lowCutSlope;
        auto lcMag = lowCutCoefficients->getMagnitudeForFrequency(freq, sampleRate);
        mag *= std::pow(lcMag, lcOrder + 1); // Raise to power of slope 

        int hcOrder = settings.highCutSlope;
        auto hcMag = highCutCoefficients->getMagnitudeForFrequency(freq, sampleRate);
        mag *= std::pow(hcMag, hcOrder + 1); // Raise to power of slope

        mags[i] = Decibels::gainToDecibels(mag);
    }

    responseCurve.startNewSubPath(responseArea.getX(),
        jmap(mags[0],
             -24.0,
             24.0,
             (double)responseArea.getBottom(),
             (double)responseArea.getY()));

    for (size_t i = 1; i < mags.size(); ++i)
    {
        auto y = jmap(mags[i],
                      -24.0,
                      24.0,
                      (double)responseArea.getBottom(),
                      (double)responseArea.getY());

        responseCurve.lineTo(responseArea.getX() + i, y);
    }

    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));

    // This gradient fade out makes the graph more appealing without a lot of styling
    auto fadeHeight = getHeight() * 0.25f;
    juce::ColourGradient fade(juce::Colours::transparentBlack,
                              0,
                              getHeight() - fadeHeight,
                              juce::Colours::black.withAlpha(1.0f),
                              0,
                              getHeight(),
                              false
    );
    g.setGradientFill(fade);
    g.fillRect(getLocalBounds().withTop(getHeight() - fadeHeight));
}