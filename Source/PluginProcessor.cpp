/*

  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================

*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AuroraD80AudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "inputGain", 1 }, "Input", juce::NormalisableRange<float> { 0.0f, 2.0f }, 1.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "delayTimeMs", 1 }, "Time", juce::NormalisableRange<float> { 1.0f, 2000.0f }, 450.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "feedback", 1 }, "Feedback", juce::NormalisableRange<float> { 0.0f, 0.95f }, 0.35f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix", juce::NormalisableRange<float> { 0.0f, 1.0f }, 0.35f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "outputGain", 1 }, "Output", juce::NormalisableRange<float> { 0.0f, 2.0f }, 1.0f));

    return layout;
}

AuroraD80AudioProcessor::AuroraD80AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       parameters (*this, nullptr, "Parameters", createParameterLayout())
#else
     : parameters (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}

AuroraD80AudioProcessor::~AuroraD80AudioProcessor()
{
}

//==============================================================================
const juce::String AuroraD80AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AuroraD80AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AuroraD80AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AuroraD80AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AuroraD80AudioProcessor::getTailLengthSeconds() const
{
    return 2.0;
}

int AuroraD80AudioProcessor::getNumPrograms()
{
    return 1;
}

int AuroraD80AudioProcessor::getCurrentProgram()
{
    return 0;
}

void AuroraD80AudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AuroraD80AudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AuroraD80AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AuroraD80AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    const auto maxDelaySamples = static_cast<int> (std::ceil (2.0 * sampleRate)) + samplesPerBlock + 1;
    delayBuffer.setSize (2, maxDelaySamples);
    delayBuffer.clear();
    delayWritePosition = 0;
}

void AuroraD80AudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AuroraD80AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void AuroraD80AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();

    for (auto channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);

    if (delayBuffer.getNumSamples() == 0)
        return;

    const auto inputGain = parameters.getRawParameterValue ("inputGain")->load();
    const auto delayTimeMs = parameters.getRawParameterValue ("delayTimeMs")->load();
    const auto feedback = parameters.getRawParameterValue ("feedback")->load();
    const auto mix = parameters.getRawParameterValue ("mix")->load();
    const auto outputGain = parameters.getRawParameterValue ("outputGain")->load();

    const auto delayBufferSize = delayBuffer.getNumSamples();
    const auto delaySamples = juce::jlimit (1, delayBufferSize - 1,
                                           static_cast<int> (std::round (delayTimeMs * currentSampleRate / 1000.0)));
    const auto channelsToProcess = juce::jmin (2, totalNumInputChannels, totalNumOutputChannels);

    for (auto sample = 0; sample < numSamples; ++sample)
    {
        const auto readPosition = (delayWritePosition + delayBufferSize - delaySamples) % delayBufferSize;

        for (auto channel = 0; channel < channelsToProcess; ++channel)
        {
            const auto input = buffer.getSample (channel, sample) * inputGain;
            const auto delayed = delayBuffer.getSample (channel, readPosition);
            const auto output = ((input * (1.0f - mix)) + (delayed * mix)) * outputGain;

            delayBuffer.setSample (channel, delayWritePosition, input + (delayed * feedback));
            buffer.setSample (channel, sample, output);
        }

        delayWritePosition = (delayWritePosition + 1) % delayBufferSize;
    }
}

//==============================================================================
bool AuroraD80AudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AuroraD80AudioProcessor::createEditor()
{
    return new AuroraD80AudioProcessorEditor (*this);
}

//==============================================================================
void AuroraD80AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState().createXml())
        copyXmlToBinary (*state, destData);
}

void AuroraD80AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto state = getXmlFromBinary (data, sizeInBytes))
        if (state->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*state));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AuroraD80AudioProcessor();
}
