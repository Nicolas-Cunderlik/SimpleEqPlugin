#pragma once
#include "JuceHeader.h"


class BandPass
{
private:
	static const float PI_F;

public:
	static struct Biquad
	{
		float b0, b1, b2;
		float a1, a2;
	};

	static enum class FilterType
	{
		Lowpass,
		Highpass
	};

	// These functions are my own implementations of high pass and low pass filters
	static std::vector<Biquad> designButterworthHighpass(float cutoff,
												  		 float sampleRate,
												  		 int order);

	static std::vector<Biquad> designButterworthLowpass(float cutoff,
												 	    float sampleRate,
												 		int order);

	// These are helper functions for making Biquads
	static Biquad makeFirstOrderHighpass(float cutoffFrequency, float sampleRate);

	static Biquad makeFirstOrderLowpass(float cutoffFrequency, float sampleRate);

	static Biquad makeBiquad(float cutoffFrequency, float sampleRate, float Q, FilterType type);

	static juce::ReferenceCountedArray<juce::dsp::IIR::Coefficients<float>>
		convertToCoefficients(const std::vector<Biquad>& biquads);
};