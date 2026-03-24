#include "IIRFilter.h"
#include <cmath>

std::vector<IIRFilter::Biquad> IIRFilter::designButterworthCutFilter(float cutoff, float sampleRate, int order, FilterType type)
{
    std::vector<Biquad> filters;

    // Future-proofing (useless right now, slopes represent 2nd, 4th, 6th, and 8th order)
    if (order % 2 == 1)
        filters.push_back(makeFirstOrderBiquad(cutoff, sampleRate, type));

    size_t numStages = order / 2;

    for (size_t i = 0; i < numStages; ++i)
    {
        float Q = 1.f / (2.f * cosf((2.f * i + 1.f) * juce::float_Pi / (2.f * order)));
        filters.push_back(makeBiquad(cutoff, sampleRate, Q, type));
    }

    return filters;
}

IIRFilter::Biquad IIRFilter::makeFirstOrderBiquad(float cutoffFrequency, float sampleRate, FilterType type)
{
    float k = tanf(juce::float_Pi * cutoffFrequency / sampleRate); // Pre-warped
    float norm = 1.f / (1.f + k);

    Biquad bq;

    bq.b0 = type == FilterType::Lowpass ? k * norm : 1.f * norm;
    bq.b1 = type == FilterType::Lowpass ? k * norm : -1.f * norm;

    bq.b2 = 0.f;
    bq.a1 = (k - 1.f) * norm;
    bq.a2 = 0.f;

    return bq;
}

IIRFilter::Biquad IIRFilter::makeBiquad(float cutoffFrequency, float sampleRate, float Q, FilterType type)
{
    float w0 = 2.f * juce::float_Pi * cutoffFrequency / sampleRate;
    float cos_w0 = cosf(w0);
    float sin_w0 = sinf(w0);
    float alpha = sin_w0 / (2.f * Q);

    float b0, b1, b2;

    switch (type)
    {
    case FilterType::Highpass:
        b0 = (1.f + cos_w0) / 2.f;
        b1 = -(1.f + cos_w0);
        b2 = b0;
        break;

    case FilterType::Lowpass:
        b0 = (1.f - cos_w0) / 2.f;
        b1 = 1.f - cos_w0;
        b2 = b0;
        break;
    }

    float a0 = 1.f + alpha;
    float a1 = -2.f * cos_w0;
    float a2 = 1.f - alpha;

    Biquad bq;

    bq.b0 = b0 / a0;
    bq.b1 = b1 / a0;
    bq.b2 = b2 / a0;
    bq.a1 = a1 / a0;
    bq.a2 = a2 / a0;

    return bq;
}

// Typically JUCE filters are implemented using predefined classes, this converts biquads -> JUCE IIR Biquad Coefficients
juce::ReferenceCountedArray<juce::dsp::IIR::Coefficients<float>>
IIRFilter::convertToCoefficients(const std::vector<Biquad>& biquads)
{
    juce::ReferenceCountedArray<juce::dsp::IIR::Coefficients<float>> coeffs;

    for (const auto& bq : biquads)
    {
        coeffs.add(new juce::dsp::IIR::Coefficients<float>(
            bq.b0, bq.b1, bq.b2,
            1.f, // JUCE expects a0 = 1
            bq.a1,
            bq.a2));
    }

    return coeffs;
}