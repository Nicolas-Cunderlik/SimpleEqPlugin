#include "IIRFilter.h"
#include <cmath>

std::array<IIRFilter::Biquad, EQConstants::maxStages>
IIRFilter::designButterworthCutFilter(float cutoff, float sampleRate, int order, FilterType type)
{
    std::array<Biquad, EQConstants::maxStages> filters{}; // No vector for the audio thread

	const bool isOddOrder = (order % 2 == 1);
    const size_t activeStages = (order + 1) / 2; // Ceiling to account for odd orders
    // Future-proofing (useless right now, slopes represent 2nd, 4th, 6th, and 8th order)
    if (isOddOrder)
        filters[0] = makeFirstOrderBiquad(cutoff, sampleRate, type);

	for (size_t i = isOddOrder ? 1 : 0; i < activeStages; ++i)
    {
        float Q = 1.f / (2.f * cosf((2.f * i + 1.f) * juce::float_Pi / (2.f * order)));
        filters[i] = makeBiquad(cutoff, sampleRate, Q, type);
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

void IIRFilter::populateCoefficients(const std::array<Biquad, EQConstants::maxStages>& biquads,
    juce::ReferenceCountedArray<juce::dsp::IIR::Coefficients<float>>& coeffs)
{
	for (size_t i = 0; i < EQConstants::maxStages; ++i)
    {
        int numCoeffs = 6; // This is a JUCE constant
        if (auto* coeff = coeffs.getObjectPointer(i))
        {
			*coeff = std::array<float, 6>{ biquads[i].b0,
                                           biquads[i].b1,
                                           biquads[i].b2,
                                           1.f,
                                           biquads[i].a1,
                                           biquads[i].a2 };
        }
    }
}