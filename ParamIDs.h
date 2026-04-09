#pragma once

namespace ParamIDs
{
// JUCE doesn't support std::string_view to juce::StringRef conversion
inline constexpr auto lowCutFreq   = "LowCut Freq";
inline constexpr auto highCutFreq  = "HighCut Freq";
inline constexpr auto peakFreq     = "Peak Freq";
inline constexpr auto peakGain     = "Peak Gain";
inline constexpr auto peakQuality  = "Peak Quality";
inline constexpr auto lowCutSlope  = "LowCut Slope";
inline constexpr auto highCutSlope = "HighCut Slope";
}

