/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p)
    : AudioProcessorEditor (&p)
    , audioProcessor (p)
    , responseCurve(p)
	, spectrumVisualizer(p)
    , highcutAttachment(audioProcessor.apvts, ParamIDs::highCutFreq, highcutSlider)
    , lowcutAttachment(audioProcessor.apvts, ParamIDs::lowCutFreq, lowcutSlider)
    , peakFilterAttachment(audioProcessor.apvts, ParamIDs::peakFreq, peakFilterSlider)
    , peakGainAttachment(audioProcessor.apvts, ParamIDs::peakGain, peakGainSlider)
    , peakQualityAttachment(audioProcessor.apvts, ParamIDs::peakQuality, peakQualitySlider)
{
    getLookAndFeel().setDefaultSansSerifTypefaceName("Helvetica");

    setLookAndFeel(&eqLookAndFeel);

    audioProcessor.apvts.addParameterListener(ParamIDs::lowCutSlope, this);
    audioProcessor.apvts.addParameterListener(ParamIDs::highCutSlope, this);

    setupTitle(lowCutTitle, "LOW CUT");
    setupTitle(peakTitle, "PEAK");
    setupTitle(highCutTitle, "HIGH CUT");

    getLookAndFeel().setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::cyan);
    getLookAndFeel().setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::grey.withAlpha(0.3f));
    getLookAndFeel().setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack); // Clean look

    createSlopeButtons(lowCutButtons, ParamIDs::lowCutSlope, 1001);
    createSlopeButtons(highCutButtons, ParamIDs::highCutSlope, 1002);

    // Add the unit labels
    lowCutSlopeUnitLabel.setText("dB/oct", juce::dontSendNotification);
    lowCutSlopeUnitLabel.setJustificationType(juce::Justification::left);
    addAndMakeVisible(lowCutSlopeUnitLabel);

    highCutSlopeUnitLabel.setText("dB/oct", juce::dontSendNotification);
    highCutSlopeUnitLabel.setJustificationType(juce::Justification::left);
    addAndMakeVisible(highCutSlopeUnitLabel);

    addAndMakeVisible(spectrumVisualizer);
    addAndMakeVisible(responseCurve);

    styleSlider(highcutSlider, juce::Slider::RotaryHorizontalVerticalDrag);
    styleSlider(lowcutSlider, juce::Slider::RotaryHorizontalVerticalDrag);
    styleSlider(peakFilterSlider, juce::Slider::RotaryHorizontalVerticalDrag);
    styleSlider(peakGainSlider, juce::Slider::LinearVertical);
    peakGainSlider.setTextValueSuffix(" dB"); // Suffix visually differentiates from quality slider
    styleSlider(peakQualitySlider, juce::Slider::LinearVertical);

    addAndMakeVisible(highcutSlider);
    addAndMakeVisible(lowcutSlider);
    addAndMakeVisible(peakFilterSlider);
    addAndMakeVisible(peakGainSlider);
    addAndMakeVisible(peakQualitySlider);

    createFrequencyLabels();
    syncSlopeButtons();

    setSize(800, 450);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
    cancelPendingUpdate();
    audioProcessor.apvts.removeParameterListener(ParamIDs::lowCutSlope, this);
    audioProcessor.apvts.removeParameterListener(ParamIDs::highCutSlope, this);
    setLookAndFeel(nullptr); // Crucial to prevent crashes
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void SimpleEQAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20);

    auto responseArea = bounds.removeFromTop(200).reduced(20, 0);
	spectrumVisualizer.setBounds(responseArea);
    responseCurve.setBounds(responseArea);

    // Frequency labels
    auto labelArea = bounds.removeFromTop(20);
    for (size_t i = 0; i < EQConstants::frequenciesHz.size(); ++i)
    {
        auto normX = juce::mapFromLog10(EQConstants::frequenciesHz[i],
                                        EQConstants::minFrequencyHz,
                                        EQConstants::maxFrequencyHz);
        int x = responseArea.getX() + normX * responseArea.getWidth();
        frequencyLabels[i].setBounds(x - 20, labelArea.getY(), 40, labelArea.getHeight());
    }

    //------------------------------------------------
    // Main Flex (3 bands spaced evenly)
    //------------------------------------------------

    juce::FlexBox mainFlex;
    mainFlex.flexDirection = juce::FlexBox::Direction::row;
    mainFlex.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;

    //------------------------------------------------
    // Lowcut Column
    //------------------------------------------------

    juce::FlexBox lowCutColumn;
    lowCutColumn.flexDirection = juce::FlexBox::Direction::column;
    lowCutColumn.alignItems = juce::FlexBox::AlignItems::center;

    // Create a row for the Slider and ComboBox to sit side-by-side
    juce::FlexBox lowCutControlsRow;
    juce::FlexBox lowCutButtonsBox;
    lowCutButtonsBox.flexDirection = juce::FlexBox::Direction::column;
    lowCutButtonsBox.items.add(juce::FlexItem().withHeight(10).withFlex(0, 0, 0));
    for (auto& btn : lowCutButtons)
        lowCutButtonsBox.items.add(juce::FlexItem(btn).withHeight(18).withWidth(90).withFlex(0, 0, 0));
    lowCutButtonsBox.items.add(juce::FlexItem().withHeight(3).withFlex(0, 0, 0));
    lowCutButtonsBox.items.add(juce::FlexItem(lowCutSlopeUnitLabel).withHeight(20).withWidth(90).withFlex(0, 0, 0));

    lowCutControlsRow.flexDirection = juce::FlexBox::Direction::row;
    lowCutControlsRow.alignItems = juce::FlexBox::AlignItems::center;
    lowCutControlsRow.items.add(juce::FlexItem(lowcutSlider).withHeight(100).withWidth(100));
    lowCutControlsRow.items.add(juce::FlexItem(lowCutButtonsBox).withHeight(100).withWidth(100));
    
    lowCutColumn.items.add(juce::FlexItem(lowCutControlsRow).withHeight(100).withWidth(200));
    lowCutColumn.items.add(juce::FlexItem().withHeight(20));
    lowCutColumn.items.add(juce::FlexItem(lowCutTitle).withHeight(20).withWidth(200));

    //------------------------------------------------
    // Peak column
    //------------------------------------------------

    juce::FlexBox peakColumn;
    peakColumn.flexDirection = juce::FlexBox::Direction::column;
    peakColumn.alignItems = juce::FlexBox::AlignItems::center;
    juce::FlexBox peakRow;
    peakRow.flexDirection = juce::FlexBox::Direction::row;
    peakRow.alignItems = juce::FlexBox::AlignItems::center;

    peakRow.items.add(juce::FlexItem(peakFilterSlider).withHeight(100).withWidth(100));
    peakRow.items.add(juce::FlexItem(peakGainSlider).withHeight(100).withWidth(50));
    peakRow.items.add(juce::FlexItem(peakQualitySlider).withHeight(100).withWidth(50));

    peakColumn.items.add(juce::FlexItem(peakRow).withHeight(100).withWidth(200));
    peakColumn.items.add(juce::FlexItem().withHeight(20));
    peakColumn.items.add(juce::FlexItem(peakTitle).withHeight(20).withWidth(200));

    //------------------------------------------------
    // Highcut column
    //------------------------------------------------

    juce::FlexBox highCutColumn;
    highCutColumn.flexDirection = juce::FlexBox::Direction::column;
    highCutColumn.alignItems = juce::FlexBox::AlignItems::center;

    // Create a row for the Slider and ComboBox
    juce::FlexBox highCutControlsRow;
    juce::FlexBox highCutButtonsBox;
    highCutButtonsBox.flexDirection = juce::FlexBox::Direction::column;
    highCutButtonsBox.items.add(juce::FlexItem().withHeight(10).withFlex(0, 0, 0));
    for (auto& btn : highCutButtons)
        highCutButtonsBox.items.add(juce::FlexItem(btn).withHeight(18).withWidth(90).withFlex(0, 0, 0));
    highCutButtonsBox.items.add(juce::FlexItem().withHeight(3).withFlex(0, 0, 0));
    highCutButtonsBox.items.add(juce::FlexItem(highCutSlopeUnitLabel).withHeight(20).withWidth(90).withFlex(0, 0, 0));

    highCutControlsRow.flexDirection = juce::FlexBox::Direction::row;
    highCutControlsRow.alignItems = juce::FlexBox::AlignItems::center;
    highCutControlsRow.items.add(juce::FlexItem(highcutSlider).withHeight(100).withWidth(100));
    highCutControlsRow.items.add(juce::FlexItem(highCutButtonsBox).withHeight(100).withWidth(100));

    highCutColumn.items.add(juce::FlexItem(highCutControlsRow).withHeight(100).withWidth(200));
    highCutColumn.items.add(juce::FlexItem().withHeight(20));
    highCutColumn.items.add(juce::FlexItem(highCutTitle).withHeight(20).withWidth(200));

    //------------------------------------------------
    // Add to main flex
    //------------------------------------------------

    mainFlex.items.add(juce::FlexItem(lowCutColumn).withWidth(330));
    mainFlex.items.add(juce::FlexItem(peakColumn).withWidth(330));
    mainFlex.items.add(juce::FlexItem(highCutColumn).withWidth(330));

    mainFlex.performLayout(bounds.removeFromBottom(150));
}

void SimpleEQAudioProcessorEditor::setupTitle(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(14.0f, juce::Font::bold));
    addAndMakeVisible(label);
}

void SimpleEQAudioProcessorEditor::styleSlider(juce::Slider& slider, juce::Slider::SliderStyle style)
{
    slider.setSliderStyle(style);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 60, 10);
    // This removes the border around the value readout box
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void SimpleEQAudioProcessorEditor::createFrequencyLabels()
{
    for (size_t i = 0; i < EQConstants::frequencyLabels.size(); ++i)
    {
        auto& label = frequencyLabels[i];
        label.setText(EQConstants::frequencyLabels[i], juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setBorderSize(juce::BorderSize<int>(0));
        label.setColour(juce::Label::textColourId, juce::Colour::fromRGBA(255, 255, 255, 150));
        addAndMakeVisible(label);
    }
}

// The slope buttons need manual handling since they don't use attachments like the sliders.
// Attachments do not make sense because the buttons represent multiple discrete values for a single parameter, not a continous range.
// Could this be done better? Probably with a custom component but I'm not doing all that shit
void SimpleEQAudioProcessorEditor::createSlopeButtons(std::array<juce::ToggleButton, EQConstants::slopeDbPerOct.size()>& buttons,
                                                     const juce::String& paramID,
                                                     int radioGroupId)
{
    for (size_t i = 0; i < buttons.size(); ++i)
    {
        auto& btn = buttons[i];
        btn.setButtonText(juce::String(EQConstants::slopeDbPerOct[i]));
        btn.setRadioGroupId(radioGroupId);
        addAndMakeVisible(btn);

        btn.onClick = [this, paramID, index = (int)i]
        {
            slopeButtonClicked(paramID, index);
        };
    }
}

void SimpleEQAudioProcessorEditor::slopeButtonClicked(const juce::String& paramID, int index)
{
    if (auto* param = audioProcessor.apvts.getParameter(paramID))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost(param->convertTo0to1((float)index));
        param->endChangeGesture();
    }

    syncSlopeButtons();
}

void SimpleEQAudioProcessorEditor::syncSlopeButtons()
{
    auto lowIndex = (int)audioProcessor.apvts.getRawParameterValue(ParamIDs::lowCutSlope)->load();
    auto highIndex = (int)audioProcessor.apvts.getRawParameterValue(ParamIDs::highCutSlope)->load();

    for (size_t i = 0; i < lowCutButtons.size(); ++i)
        lowCutButtons[i].setToggleState((int)i == lowIndex, juce::dontSendNotification);
    for (size_t i = 0; i < highCutButtons.size(); ++i)
        highCutButtons[i].setToggleState((int)i == highIndex, juce::dontSendNotification);
}

void SimpleEQAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused(newValue);

    if (parameterID == ParamIDs::lowCutSlope || parameterID == ParamIDs::highCutSlope)
        triggerAsyncUpdate();
}

void SimpleEQAudioProcessorEditor::handleAsyncUpdate()
{
    syncSlopeButtons();
}
