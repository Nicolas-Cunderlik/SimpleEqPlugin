#include "EQLookAndFeel.h"

void EQLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
    const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider)
{
    auto outline = slider.findColour(juce::Slider::rotarySliderOutlineColourId);
    auto fill = slider.findColour(juce::Slider::rotarySliderFillColourId);

    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = 6.0f; // Thickness of the dial
    auto arcRadius = radius - lineW * 0.5f;

    juce::Path backgroundArc;
    backgroundArc.addCentredArc(bounds.getCentreX(),
                                bounds.getCentreY(),
                                arcRadius,
                                arcRadius,
                                0.0f,
                                rotaryStartAngle,
                                rotaryEndAngle,
                                true);
    g.setColour(outline);
    g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius,
            0.0f, rotaryStartAngle, toAngle, true);
        g.setColour(fill);
        g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

void EQLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float minSliderPos, float maxSliderPos,
    const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    auto trackWidth = 6.0f;
    auto bounds = juce::Rectangle<float>(x, y, width, height).reduced(width * 0.25f, 0.0f);
    auto trackX = bounds.getCentreX() - (trackWidth * 0.5f);
    juce::Path trackPath;
    trackPath.addRoundedRectangle(trackX, bounds.getY(), trackWidth, bounds.getHeight(), trackWidth * 0.5f);
    g.setColour(slider.findColour(juce::Slider::rotarySliderOutlineColourId));
    g.fillPath(trackPath);

    if (slider.isEnabled())
    {
        juce::Graphics::ScopedSaveState state(g);
        // This ensures NOTHING is drawn outside the track's rounded corners
        g.reduceClipRegion(trackPath);
        g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
        float fillHeight = bounds.getBottom() - sliderPos;
        g.fillRect(trackX, sliderPos, trackWidth, fillHeight);
    }
}

void EQLookAndFeel::drawTickBox(juce::Graphics& g, juce::Component& btn,
    float x, float y, float w, float h,
    bool ticked, bool isEnabled,
    bool isMouseOver, bool isButtonDown)
{
    auto bounds = juce::Rectangle<float>(x, y, w, h).reduced(2.0f);
    g.setColour(btn.findColour(juce::ToggleButton::tickDisabledColourId).withAlpha(0.1f));
    g.fillRoundedRectangle(bounds, 3.0f);

    if (ticked)
    {
        g.setColour(juce::Colours::cyan);
        g.drawRoundedRectangle(bounds, 3.0f, 2.0f);
        g.fillRoundedRectangle(bounds.reduced(3.0f), 2.0f);
    }
}