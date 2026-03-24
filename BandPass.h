#pragma once
#include "JuceHeader.h"

class BandPass
{
public:
	struct Biquad
	{
		float b0, b1, b2;
		float a1, a2;
	};

	enum class FilterType
	{
		Lowpass,
		Highpass
	};

	// My own cut filter implementations
	static std::vector<Biquad> designButterworthCutFilter(float cutoff, float sampleRate, int order, FilterType type);

	// These are helper functions for making Biquads
	static Biquad makeFirstOrderBiquad(float cutoffFrequency, float sampleRate, FilterType type);
	static Biquad makeBiquad(float cutoffFrequency, float sampleRate, float Q, FilterType type);

	static juce::ReferenceCountedArray<juce::dsp::IIR::Coefficients<float>>
		convertToCoefficients(const std::vector<Biquad>& biquads);
};