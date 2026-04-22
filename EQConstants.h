#pragma once

#include <array>

//------------------------------------------------
// Constants for the filters and response curve
//------------------------------------------------
namespace EQConstants
{
enum class Slope {
    Slope_12, Slope_24, Slope_36, Slope_48
};

constexpr float minFrequencyHz = 20.0f;
constexpr float maxFrequencyHz = 20000.0f;

inline constexpr std::array<float, 9> frequenciesHz
{
    20.0f, 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 5000.0f, 10000.0f, 20000.0f
};

inline constexpr std::array<const char*, 9> frequencyLabels
{
    "20", "50", "100", "200", "500", "1k", "5k", "10k", "20k"
};

inline constexpr std::array<int, 4> slopeDbPerOct
{
    12, 24, 36, 48
};

constexpr int numSlopes = slopeDbPerOct.size();
constexpr int maxStages = numSlopes + 1; // To account for any first-order filters (e.g. 18dB/oct)

// Takes a Slope enum value and converts it to a filter order (2, 4, 6, or 8)
inline constexpr int slopeEnumToOrder(Slope slope)
{
    return 2 * ((int)slope + 1);
}

//------------------------------------------------
// Constants for the spectrum analyzer
//------------------------------------------------
constexpr size_t STFTOrder = 10;
constexpr size_t STFTSize = 1 << STFTOrder; 
// This is how big the buffer and fifo are relative to host-provided buffer size
constexpr size_t SpectrumAnalyzerBufferSizeMultiplier = 4;
}
