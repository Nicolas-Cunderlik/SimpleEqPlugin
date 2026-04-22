#pragma once
#include "JuceHeader.h"
#include "EQConstants.h"

class IIRFilter
{
public:
	struct Biquad
	{
		float b0 { 1.f },
			  b1 { 0.f },
			  b2 { 0.f },
			  a1 { 0.f },
			  a2 { 0.f };
	};

	enum class FilterType
	{
		Lowpass,
		Highpass
	};

	// My own cut filter implementations
	static std::array<Biquad, EQConstants::numSlopes + 1>
		designButterworthCutFilter(float cutoff, float sampleRate, int order, FilterType type);

	// These are helper functions for making Biquads
	static Biquad makeFirstOrderBiquad(float cutoffFrequency, float sampleRate, FilterType type);
	static Biquad makeBiquad(float cutoffFrequency, float sampleRate, float Q, FilterType type);

	// Puts the coefficients from the biquads into the format needed for the juce filters
	static void populateCoefficients(const std::array<Biquad, EQConstants::maxStages>& biquads,
		juce::ReferenceCountedArray<juce::dsp::IIR::Coefficients<float>>& coeffs);
};