#pragma once

#include <JuceHeader.h>

class EQLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPos,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPos,
                          float minSliderPos,
                          float maxSliderPos,
                          juce::Slider::SliderStyle style,
                          juce::Slider& slider) override;

    void drawTickBox(juce::Graphics& g,
                     juce::Component& btn,
                     float x,
                     float y,
                     float w,
                     float h,
                     bool ticked,
                     bool isEnabled,
                     bool isMouseOver,
                     bool isButtonDown) override;
};
