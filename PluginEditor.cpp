/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), responseCurve(p)
{
    auto customFont = juce::Font("Helvetica", 15.0f, juce::Font::plain);
    getLookAndFeel().setDefaultSansSerifTypefaceName("Helvetica");

    setLookAndFeel(&eqLookAndFeel);

    auto setupTitle = [this](juce::Label& l, juce::String text)
    {
        l.setText(text, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(juce::Font(14.0f, juce::Font::bold));
        addAndMakeVisible(l);
    };

    setupTitle(lowCutTitle, "LOW CUT");
    setupTitle(peakTitle, "PEAK");
    setupTitle(highCutTitle, "HIGH CUT");

    getLookAndFeel().setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::cyan);
    getLookAndFeel().setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::grey.withAlpha(0.3f));
    getLookAndFeel().setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack); // Clean look

    createSlopeButtons(lowCutButtons, "LowCut Slope", 1001);
    createSlopeButtons(highCutButtons,  "HighCut Slope", 1002);

    // Add the unit labels
    lowCutSlopeUnitLabel.setText("dB/oct", juce::dontSendNotification);
    lowCutSlopeUnitLabel.setJustificationType(juce::Justification::left);
    addAndMakeVisible(lowCutSlopeUnitLabel);

    highCutSlopeUnitLabel.setText("dB/oct", juce::dontSendNotification);
    highCutSlopeUnitLabel.setJustificationType(juce::Justification::left);
    addAndMakeVisible(highCutSlopeUnitLabel);

    addAndMakeVisible(responseCurve);

    styleSlider(highcutSlider);
    styleSlider(lowcutSlider);
    styleSlider(peakFilterSlider);
    styleSlider(peakGainSlider);
    peakGainSlider.setTextValueSuffix(" dB"); // Suffix visually differentiates from quality slider
    styleSlider(peakQualitySlider);

    addAndMakeVisible(highcutSlider);
    addAndMakeVisible(lowcutSlider);
    addAndMakeVisible(peakFilterSlider);
    addAndMakeVisible(peakGainSlider);
    addAndMakeVisible(peakQualitySlider);

    createFrequencyLabels();

    // Attach parameters
    highcutAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "HighCut Freq", highcutSlider);

    lowcutAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "LowCut Freq", lowcutSlider);

    peakFilterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "Peak Freq", peakFilterSlider);

    peakGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "Peak Gain", peakGainSlider);

    peakQualityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "Peak Quality", peakQualitySlider);

    setSize(800, 450);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
    setLookAndFeel(nullptr); // Crucial to prevent crashes
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    g.setColour(primaryColor2);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawFittedText("Equalizer Plugin",
                     getLocalBounds().removeFromTop(40),
                     juce::Justification::centred, 1);
}

void SimpleEQAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20);

    auto responseArea = bounds.removeFromTop(200).reduced(20, 0);
    responseCurve.setBounds(responseArea);

    // Frequency labels
    auto labelArea = bounds.removeFromTop(20);
    for (size_t i = 0; i < freqs.size(); ++i)
    {
        auto normX = juce::mapFromLog10(freqs[i], 20.f, 20000.f);
        int x = responseArea.getX() + normX * responseArea.getWidth();
        frequencyLabels[i]->setBounds(x - 20,
                                      labelArea.getY(),
                                      40,
                                      labelArea.getHeight());
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
        lowCutButtonsBox.items.add(juce::FlexItem(*btn).withHeight(18).withWidth(90).withFlex(0, 0, 0));
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

    // It's redundant work, but the peak sliders should be restyled again as vertical
    peakGainSlider.setSliderStyle(juce::Slider::LinearVertical);
    peakQualitySlider.setSliderStyle(juce::Slider::LinearVertical);

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
        highCutButtonsBox.items.add(juce::FlexItem(*btn).withHeight(18).withWidth(90).withFlex(0, 0, 0));
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

void SimpleEQAudioProcessorEditor::styleSlider(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 60, 10);
    // This removes the border around the value readout box
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void SimpleEQAudioProcessorEditor::createFrequencyLabels()
{
    std::array<juce::String, 9> freqs =
    { "20","50","100","200","500","1k","5k","10k","20k" };

    for (auto& f : freqs)
    {
        auto label = std::make_unique<juce::Label>();
        label->setText(f, juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centred);
        label->setBorderSize(juce::BorderSize<int>(0));
        label->setColour(juce::Label::textColourId, juce::Colour::fromRGBA(255, 255, 255, 150));
        addAndMakeVisible(*label);
        frequencyLabels.push_back(std::move(label));
    }
}

void SimpleEQAudioProcessorEditor::createSlopeButtons(std::vector<std::unique_ptr<juce::ToggleButton>>& buttons,
    const juce::String& paramID, int radioGroupId)
{
    std::array<int, 4> values = { 12, 24, 36, 48 };

    if (auto* param = audioProcessor.apvts.getParameter(paramID))
        if (param->getValue() == 0.0f)
            param->setValueNotifyingHost(0.0f);

    for (int i = 0; i < 4; ++i)
    {
        auto btn = std::make_unique<juce::ToggleButton>(juce::String(values[i]));
        btn->setRadioGroupId(radioGroupId);
        addAndMakeVisible(*btn);

        btn->onClick = [this, btnPtr = btn.get(), paramID, val = values[i]]()
            {
                if (auto* param = audioProcessor.apvts.getParameter(paramID))
                {
                    float normalized = (val - 12) / 36.0f;
                    param->beginChangeGesture();
                    param->setValueNotifyingHost(normalized);
                    param->endChangeGesture();
                }

                auto& targetButtons = (paramID == "LowCut Slope" ? lowCutButtons : highCutButtons);
                for (auto& b : targetButtons)
                    b->setToggleState(b.get() == btnPtr, juce::dontSendNotification);
            };

        buttons.push_back(std::move(btn));
    }

    if (auto* param = audioProcessor.apvts.getParameter(paramID))
    {
        float val = param->getValue() * 36.0f + 12;
        for (auto& b : buttons)
            b->setToggleState(b->getButtonText().getIntValue() == (int)val, juce::dontSendNotification);
    }
}