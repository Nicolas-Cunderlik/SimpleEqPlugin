/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "EQConstants.h"

//==============================================================================
SimpleEQAudioProcessor::SimpleEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    apvts.addParameterListener(ParamIDs::lowCutFreq, this);
    apvts.addParameterListener(ParamIDs::highCutFreq, this);
    apvts.addParameterListener(ParamIDs::peakFreq, this);
    apvts.addParameterListener(ParamIDs::peakGain, this);
    apvts.addParameterListener(ParamIDs::peakQuality, this);
    apvts.addParameterListener(ParamIDs::lowCutSlope, this);
    apvts.addParameterListener(ParamIDs::highCutSlope, this);
}

SimpleEQAudioProcessor::~SimpleEQAudioProcessor()
{
    apvts.removeParameterListener(ParamIDs::lowCutFreq, this);
    apvts.removeParameterListener(ParamIDs::highCutFreq, this);
    apvts.removeParameterListener(ParamIDs::peakFreq, this);
    apvts.removeParameterListener(ParamIDs::peakGain, this);
    apvts.removeParameterListener(ParamIDs::peakQuality, this);
    apvts.removeParameterListener(ParamIDs::lowCutSlope, this);
    apvts.removeParameterListener(ParamIDs::highCutSlope, this);
}

//==============================================================================
const juce::String SimpleEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleEQAudioProcessor::getProgramName (int index)
{
    return {};
}

juce::AbstractFifo& SimpleEQAudioProcessor::getFilteredBufferFifo() noexcept
{
    return filteredBufferFifo;
}

const juce::AudioBuffer<float>& SimpleEQAudioProcessor::getFilteredMonoBuffer() const noexcept
{
    return filteredMonoBuffer;
}

void SimpleEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    peakCoefficients = new juce::dsp::IIR::Coefficients<float>();
    initializeCoefficients(lowCutCoefficients);
    initializeCoefficients(highCutCoefficients);

    updateFilters();
    filtersNeedUpdate.store(false);

    filteredBufferFifo.setTotalSize(samplesPerBlock * EQConstants::SpectrumAnalyzerBufferSizeMultiplier);
    filteredBufferFifo.reset();
    filteredMonoBuffer.setSize(1, samplesPerBlock * EQConstants::SpectrumAnalyzerBufferSizeMultiplier, false, false, true);
}

void SimpleEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SimpleEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    if (filtersNeedUpdate.exchange(false))
        updateFilters();

    juce::dsp::AudioBlock<float> block(buffer);

    auto leftBlock = block.getSingleChannelBlock(0);
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    leftChain.process(leftContext);

    if (block.getNumChannels() > 1)
    {
        auto rightBlock = block.getSingleChannelBlock(1);
        juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
        rightChain.process(rightContext);
    }

    // The chosen FIFO policy is to skip new
    if (filteredBufferFifo.getFreeSpace() >= buffer.getNumSamples())
    {
        pushFilteredMonoAudioToFifo(block);
    }
}
//
//void SimpleEQAudioProcessor::pushFilteredMonoAudioToFifo(const juce::dsp::AudioBlock<float>& block)
//{
//    const auto numChannels = block.getNumChannels();
//    auto writeHandle = filteredBufferFifo.write(block.getNumSamples());
//
//	for (size_t i = 0; i < writeHandle.blockSize1 + writeHandle.blockSize2; ++i)
//    {
//        float sample = 0.0f;
//        auto index = (i < writeHandle.blockSize1)
//                     ? writeHandle.startIndex1 + i
//                     : writeHandle.startIndex2 + (i - writeHandle.blockSize1);
//		for (size_t channel = 0; channel < numChannels; ++channel)
//            sample += block.getChannelPointer(channel)[index];
//
//        filteredMonoBuffer.setSample(0, index, filteredMonoBuffer.getSample(0, index) / numChannels);
//    }
//}

void SimpleEQAudioProcessor::pushFilteredMonoAudioToFifo(const juce::dsp::AudioBlock<float>& block)
{
    const auto numChannels = block.getNumChannels();
    auto writeHandle = filteredBufferFifo.write(block.getNumSamples());

    for (size_t i = 0; i < writeHandle.blockSize1 + writeHandle.blockSize2; ++i)
    {
        float sample = 0.0f;
        for (size_t channel = 0; channel < numChannels; ++channel)
            sample += block.getChannelPointer(channel)[i];

        const int fifoIndex = (i < (size_t)writeHandle.blockSize1)
            ? writeHandle.startIndex1 + (int)i
            : writeHandle.startIndex2 + (int)i - writeHandle.blockSize1;

        filteredMonoBuffer.setSample(0, fifoIndex, sample / numChannels);
    }
}

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor()
{
    return new SimpleEQAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void SimpleEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));

    filtersNeedUpdate.store(true);
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue(ParamIDs::lowCutFreq)->load();
    settings.highCutFreq = apvts.getRawParameterValue(ParamIDs::highCutFreq)->load();
    settings.peakFreq = apvts.getRawParameterValue(ParamIDs::peakFreq)->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue(ParamIDs::peakGain)->load();
    settings.peakQuality = apvts.getRawParameterValue(ParamIDs::peakQuality)->load();
    settings.lowCutSlope = static_cast<EQConstants::Slope>(apvts.getRawParameterValue(ParamIDs::lowCutSlope)->load());
    settings.highCutSlope = static_cast<EQConstants::Slope>(apvts.getRawParameterValue(ParamIDs::highCutSlope)->load());

    return settings;
}

void SimpleEQAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused(parameterID, newValue);
    filtersNeedUpdate.store(true);
}

void SimpleEQAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings)
{
    auto newCoeffs = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter(getSampleRate(),
        chainSettings.peakFreq,
        chainSettings.peakQuality,
        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));

	*peakCoefficients = newCoeffs;

    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
}

void SimpleEQAudioProcessor::updateCoefficients(Coefficients &old, const Coefficients &replacements) 
{
    *old = *replacements;
}

void SimpleEQAudioProcessor::updateLowCutFilters(const ChainSettings& chainSettings) 
{
    auto biquads = IIRFilter::designButterworthCutFilter(chainSettings.lowCutFreq,
                                                       getSampleRate(),
                                                       EQConstants::slopeEnumToOrder(chainSettings.lowCutSlope),
                                                       IIRFilter::FilterType::Highpass);
    IIRFilter::populateCoefficients(biquads, lowCutCoefficients);

    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();

    updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
}

void SimpleEQAudioProcessor::updateHighCutFilters(const ChainSettings& chainSettings)
{
    auto biquads = IIRFilter::designButterworthCutFilter(chainSettings.highCutFreq,
                                                      getSampleRate(),
                                                      EQConstants::slopeEnumToOrder(chainSettings.highCutSlope),
                                                      IIRFilter::FilterType::Lowpass);
    IIRFilter::populateCoefficients(biquads, highCutCoefficients);

    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();

    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
}

void SimpleEQAudioProcessor::updateFilters()
{
    auto chainSettings = getChainSettings(apvts);

    updateLowCutFilters(chainSettings);
    updatePeakFilter(chainSettings);
    updateHighCutFilters(chainSettings);
}

template<typename ChainType, typename CoefficientType>
static void SimpleEQAudioProcessor::updateCutFilter(ChainType& chain,
    const CoefficientType& coefficients,
    const EQConstants::Slope& slope)
{
    chain.template setBypassed<0>(true);
    chain.template setBypassed<1>(true);
    chain.template setBypassed<2>(true);
    chain.template setBypassed<3>(true);

    switch (slope)
    {
    case EQConstants::Slope::Slope_48:
        update<3>(chain, coefficients);
        [[fallthrough]];
    case EQConstants::Slope::Slope_36:
        update<2>(chain, coefficients);
        [[fallthrough]];
    case EQConstants::Slope::Slope_24:
        update<1>(chain, coefficients);
        [[fallthrough]];
    case EQConstants::Slope::Slope_12:
        update<0>(chain, coefficients);
        break;
    default:
        break;
    }
}

template<int Index, typename ChainType, typename CoefficientType>
static void SimpleEQAudioProcessor::update(ChainType& chain, const CoefficientType& coefficients)
{
    updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
    chain.template setBypassed<Index>(false);
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::lowCutFreq,
        ParamIDs::lowCutFreq,
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f),
        20.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::highCutFreq,
        ParamIDs::highCutFreq,
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f),
        20000.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::peakFreq,
        ParamIDs::peakFreq,
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f),
        750.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::peakGain,
        ParamIDs::peakGain,
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.5f, 1.0f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::peakQuality,
        ParamIDs::peakQuality,
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.05f, 1.0f),
        1.0f));

    juce::StringArray stringArray;
    for (auto slope : EQConstants::slopeDbPerOct)
        stringArray.add(juce::String(slope) + " dB/Oct");

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::lowCutSlope,
        ParamIDs::lowCutSlope,
        stringArray,
        0));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::highCutSlope,
        ParamIDs::highCutSlope,
        stringArray,
        0));

    return layout;
}

void SimpleEQAudioProcessor::initializeCoefficients(
    juce::ReferenceCountedArray<juce::dsp::IIR::Coefficients<float>>& coefficients)
{
    if (coefficients.size() == EQConstants::maxStages)
        return;

    coefficients.clear();

	for (size_t i = 0; i < EQConstants::maxStages; ++i)
        coefficients.add(new juce::dsp::IIR::Coefficients<float>());
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEQAudioProcessor();
}
