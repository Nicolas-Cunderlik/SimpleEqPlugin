#include "BandPass.h"
#include <cmath>  // For sinf, cosf, tanf

// Define static constant
const float BandPass::PI_F = juce::MathConstants<float>::pi;

// Highpass design
std::vector<BandPass::Biquad> BandPass::designButterworthHighpass(float cutoff,
    float sampleRate,
    int order)
{
    std::vector<Biquad> filters;

    if (order % 2 == 1)  // Odd order -> first-order stage
        filters.push_back(makeFirstOrderHighpass(cutoff, sampleRate));

    int numStages = order / 2;
    for (int i = 0; i < numStages; ++i)
    {
        float Q = 1.f / (2.f * cosf((2.f * i + 1.f) * PI_F / (2.f * order)));
        filters.push_back(makeBiquad(cutoff, sampleRate, Q, FilterType::Highpass));
    }

    return filters;
}

// Lowpass design
std::vector<BandPass::Biquad> BandPass::designButterworthLowpass(float cutoff,
    float sampleRate,
    int order)
{
    std::vector<Biquad> filters;

    if (order % 2 == 1)
        filters.push_back(makeFirstOrderLowpass(cutoff, sampleRate));

    int numStages = order / 2;
    for (int i = 0; i < numStages; ++i)
    {
        float Q = 1.f / (2.f * cosf((2.f * i + 1.f) * PI_F / (2.f * order)));
        filters.push_back(makeBiquad(cutoff, sampleRate, Q, FilterType::Lowpass));
    }

    return filters;
}

// First-order highpass
BandPass::Biquad BandPass::makeFirstOrderHighpass(float cutoffFrequency, float sampleRate)
{
    const float K = tanf(PI_F * cutoffFrequency / sampleRate); // Pre-warped
    const float NORM = 1.f / (1.f + K);

    Biquad bq;
    bq.b0 = 1.f * NORM;
    bq.b1 = -1.f * NORM;
    bq.b2 = 0.f;
    bq.a1 = (K - 1.f) * NORM;
    bq.a2 = 0.f;

    return bq;
}

// First-order lowpass
BandPass::Biquad BandPass::makeFirstOrderLowpass(float cutoffFrequency, float sampleRate)
{
    const float K = tanf(PI_F * cutoffFrequency / sampleRate); // Pre-warped
    const float NORM = 1.f / (1.f + K);

    Biquad bq;
    bq.b0 = K * NORM;
    bq.b1 = K * NORM;
    bq.b2 = 0.f;
    bq.a1 = (K - 1.f) * NORM;
    bq.a2 = 0.f;

    return bq;
}

// Second-order bandpass
BandPass::Biquad BandPass::makeBiquad(float cutoffFrequency, float sampleRate, float Q, FilterType type)
{
    const float W0 = 2.f * PI_F * cutoffFrequency / sampleRate;
    const float COS_W0 = cosf(W0);
    const float SIN_W0 = sinf(W0);
    const float ALPHA = SIN_W0 / (2.f * Q);

    float b0, b1, b2;

    switch (type)
    {
    case FilterType::Highpass:
        b0 = (1.f + COS_W0) / 2.f;
        b1 = -(1.f + COS_W0);
        b2 = b0;
        break;

    case FilterType::Lowpass:
        b0 = (1.f - COS_W0) / 2.f;
        b1 = 1.f - COS_W0;
        b2 = b0;
        break;
    }

    float a0 = 1.f + ALPHA;
    float a1 = -2.f * COS_W0;
    float a2 = 1.f - ALPHA;

    Biquad bq;
    bq.b0 = b0 / a0;
    bq.b1 = b1 / a0;
    bq.b2 = b2 / a0;
    bq.a1 = a1 / a0;
    bq.a2 = a2 / a0;

    return bq;
}

juce::ReferenceCountedArray<juce::dsp::IIR::Coefficients<float>>
BandPass::convertToCoefficients(const std::vector<Biquad>& biquads)
{
    juce::ReferenceCountedArray<juce::dsp::IIR::Coefficients<float>> coeffs;

    for (auto& bq : biquads)
    {
        coeffs.add(new juce::dsp::IIR::Coefficients<float>(
            bq.b0, bq.b1, bq.b2,
            1.f, // JUCE expects a0 = 1
            bq.a1,
            bq.a2
        ));
    }

    return coeffs;
}