/*

  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================

*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr double maxDelayTimeMs = 2000.0;
    constexpr double maxModulationDepthMs = 20.0;

    double getDivisionMultiplier (int divisionIndex)
    {
        switch (divisionIndex)
        {
            case 0:  return 1.0 / 16.0; // 1/64
            case 1:  return 1.0 / 8.0;  // 1/32
            case 2:  return 1.0 / 4.0;  // 1/16
            case 3:  return 1.0 / 2.0;  // 1/8
            case 4:  return 1.0;        // 1/4
            case 5:  return 2.0;        // 1/2
            case 6:  return 4.0;        // 1/1
            case 7:  return 3.0 / 32.0; // 1/64D
            case 8:  return 3.0 / 16.0; // 1/32D
            case 9:  return 3.0 / 8.0;  // 1/16D
            case 10: return 3.0 / 4.0;  // 1/8D
            case 11: return 3.0 / 2.0;  // 1/4D
            case 12: return 3.0;        // 1/2D
            case 13: return 6.0;        // 1/1D
            case 14: return 1.0 / 24.0; // 1/64T
            case 15: return 1.0 / 12.0; // 1/32T
            case 16: return 1.0 / 6.0;  // 1/16T
            case 17: return 1.0 / 3.0;  // 1/8T
            case 18: return 2.0 / 3.0;  // 1/4T
            case 19: return 4.0 / 3.0;  // 1/2T
            case 20: return 8.0 / 3.0;  // 1/1T
            default: return 1.0;
        }
    }

    float readDelaySampleLinear (const juce::AudioBuffer<float>& delayBuffer,
                                 int channel,
                                 int writePosition,
                                 double delaySamples)
    {
        const auto bufferSize = delayBuffer.getNumSamples();
        auto readPosition = static_cast<double> (writePosition) - delaySamples;

        while (readPosition < 0.0)
            readPosition += static_cast<double> (bufferSize);

        const auto index0 = static_cast<int> (readPosition) % bufferSize;
        const auto index1 = (index0 + 1) % bufferSize;
        const auto fraction = static_cast<float> (readPosition - std::floor (readPosition));

        return delayBuffer.getSample (channel, index0)
             + ((delayBuffer.getSample (channel, index1) - delayBuffer.getSample (channel, index0)) * fraction);
    }

    float nextNoiseSample (uint32_t& state)
    {
        state = (state * 1664525u) + 1013904223u;
        return (static_cast<float> ((state >> 8) & 0x00ffffff) / 8388607.5f) - 1.0f;
    }

    float calculateRmsLevel (const juce::AudioBuffer<float>& buffer, int channel)
    {
        if (channel >= buffer.getNumChannels() || buffer.getNumSamples() <= 0)
            return 0.0f;

        auto sum = 0.0f;
        const auto* samples = buffer.getReadPointer (channel);

        for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
            sum += samples[sample] * samples[sample];

        return std::sqrt (sum / static_cast<float> (buffer.getNumSamples()));
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AuroraD80AudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "inputGain", 1 }, "Input Trim", juce::NormalisableRange<float> { -12.0f, 12.0f }, 0.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "delayTimeMs", 1 }, "Time", juce::NormalisableRange<float> { 1.0f, 2000.0f }, 578.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "feedback", 1 }, "Feedback", juce::NormalisableRange<float> { 0.0f, 0.95f }, 0.35f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix", juce::NormalisableRange<float> { 0.0f, 1.0f }, 0.35f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "outputGain", 1 }, "Output Trim", juce::NormalisableRange<float> { -12.0f, 12.0f }, -1.5f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "syncEnabled", 1 }, "Sync", true));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "syncDivision", 1 }, "Division",
        juce::StringArray { "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1/1",
                            "1/64D", "1/32D", "1/16D", "1/8D", "1/4D", "1/2D", "1/1D",
                            "1/64T", "1/32T", "1/16T", "1/8T", "1/4T", "1/2T", "1/1T" }, 4));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lowCutHz", 1 }, "Low Cut", juce::NormalisableRange<float> { 20.0f, 2000.0f }, 120.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "highCutHz", 1 }, "High Cut", juce::NormalisableRange<float> { 1000.0f, 20000.0f }, 19000.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "modulationDepth", 1 }, "Depth", juce::NormalisableRange<float> { 0.0f, 100.0f }, 15.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "modulationRate", 1 }, "Rate", juce::NormalisableRange<float> { 0.1f, 10.0f }, 0.35f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "drive", 1 }, "Drive", juce::NormalisableRange<float> { 0.0f, 100.0f }, 10.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "vintage", 1 }, "Evolve", juce::NormalisableRange<float> { 0.0f, 100.0f }, 52.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "width", 1 }, "Width", juce::NormalisableRange<float> { 0.0f, 200.0f }, 131.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "drift", 1 }, "Drift", juce::NormalisableRange<float> { 0.0f, 100.0f }, 12.0f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "modShape", 1 }, "Shape", juce::StringArray { "Sine", "Triangle", "Square", "Saw", "Random" }, 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "bloom", 1 }, "Bloom", juce::NormalisableRange<float> { 0.0f, 100.0f }, 25.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "pingPong", 1 }, "Ping Pong", true));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "mode", 1 }, "Mode", juce::StringArray { "Digital", "Tape", "Analog" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "quality", 1 }, "Quality", juce::StringArray { "Lo Fi", "Standard", "Hi Fi" }, 1));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "freeze", 1 }, "Freeze", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "oversampling", 1 }, "Oversampling", juce::StringArray { "Off", "2x", "4x", "8x" }, 1));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "bypass", 1 }, "Bypass", false));

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
    for (auto channel = 0; channel < 2; ++channel)
    {
        inputRmsLevels[(size_t) channel].store (0.0f);
        outputRmsLevels[(size_t) channel].store (0.0f);
    }
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

    for (auto channel = 0; channel < 2; ++channel)
    {
        inputRmsLevels[(size_t) channel].store (0.0f);
        outputRmsLevels[(size_t) channel].store (0.0f);
    }

    const auto maxDelaySamples = static_cast<int> (std::ceil (2.0 * sampleRate)) + samplesPerBlock + 1;
    delayBuffer.setSize (2, maxDelaySamples);
    delayBuffer.clear();
    delayWritePosition = 0;
    lfoPhase = 0.0;
    vintageSoftenState = { 0.0f, 0.0f };
    noiseState = 0x12345678;

    smoothedModulationDepth.reset (sampleRate, 0.05);
    smoothedModulationRate.reset (sampleRate, 0.05);
    smoothedDrive.reset (sampleRate, 0.05);
    smoothedVintage.reset (sampleRate, 0.05);
    smoothedModulationDepth.setCurrentAndTargetValue (15.0f);
    smoothedModulationRate.setCurrentAndTargetValue (0.35f);
    smoothedDrive.setCurrentAndTargetValue (0.0f);
    smoothedVintage.setCurrentAndTargetValue (0.0f);

    const auto lowCutCoefficients = juce::IIRCoefficients::makeHighPass (currentSampleRate, 120.0);
    const auto highCutCoefficients = juce::IIRCoefficients::makeLowPass (currentSampleRate, 12000.0);

    for (auto channel = 0; channel < 2; ++channel)
    {
        lowCutFilters[channel].reset();
        highCutFilters[channel].reset();
        lowCutFilters[channel].setCoefficients (lowCutCoefficients);
        highCutFilters[channel].setCoefficients (highCutCoefficients);
    }
}

void AuroraD80AudioProcessor::releaseResources()
{
    for (auto channel = 0; channel < 2; ++channel)
    {
        inputRmsLevels[(size_t) channel].store (0.0f);
        outputRmsLevels[(size_t) channel].store (0.0f);
    }
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

    for (auto channel = 0; channel < 2; ++channel)
        inputRmsLevels[static_cast<size_t> (channel)].store (channel < totalNumInputChannels ? calculateRmsLevel (buffer, channel) : 0.0f);

    for (auto channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);

    if (delayBuffer.getNumSamples() == 0)
    {
        for (auto channel = 0; channel < 2; ++channel)
            outputRmsLevels[static_cast<size_t> (channel)].store (channel < totalNumOutputChannels ? calculateRmsLevel (buffer, channel) : 0.0f);

        return;
    }

    const auto inputGain = juce::Decibels::decibelsToGain (parameters.getRawParameterValue ("inputGain")->load());
    auto delayTimeMs = parameters.getRawParameterValue ("delayTimeMs")->load();
    const auto feedback = parameters.getRawParameterValue ("feedback")->load();
    const auto mix = parameters.getRawParameterValue ("mix")->load();
    const auto outputGain = juce::Decibels::decibelsToGain (parameters.getRawParameterValue ("outputGain")->load());
    const auto syncEnabled = parameters.getRawParameterValue ("syncEnabled")->load() > 0.5f;
    const auto syncDivision = static_cast<int> (std::round (parameters.getRawParameterValue ("syncDivision")->load()));
    const auto lowCutHz = parameters.getRawParameterValue ("lowCutHz")->load();
    const auto highCutHz = parameters.getRawParameterValue ("highCutHz")->load();
    const auto modulationDepth = parameters.getRawParameterValue ("modulationDepth")->load();
    const auto modulationRate = parameters.getRawParameterValue ("modulationRate")->load();
    const auto drive = parameters.getRawParameterValue ("drive")->load();
    const auto vintage = parameters.getRawParameterValue ("vintage")->load();
    const auto width = parameters.getRawParameterValue ("width")->load() / 100.0f;
    const auto drift = parameters.getRawParameterValue ("drift")->load() / 100.0f;
    const auto modShape = static_cast<int> (std::round (parameters.getRawParameterValue ("modShape")->load()));
    const auto bloom = parameters.getRawParameterValue ("bloom")->load() / 100.0f;
    const auto pingPong = parameters.getRawParameterValue ("pingPong")->load() > 0.5f;
    const auto mode = static_cast<int> (std::round (parameters.getRawParameterValue ("mode")->load()));
    const auto freeze = parameters.getRawParameterValue ("freeze")->load() > 0.5f;
    const auto bypass = parameters.getRawParameterValue ("bypass")->load() > 0.5f;

    if (syncEnabled)
    {
        auto bpm = 120.0;

        if (auto* playHead = getPlayHead())
            if (auto position = playHead->getPosition())
                if (auto hostBpm = position->getBpm(); hostBpm.hasValue() && *hostBpm > 0.0)
                    bpm = *hostBpm;

        const auto quarterNoteMs = 60000.0 / bpm;
        delayTimeMs = static_cast<float> (juce::jlimit (1.0, maxDelayTimeMs, quarterNoteMs * getDivisionMultiplier (syncDivision)));
    }

    const auto nyquist = currentSampleRate * 0.5;
    const auto safeLowCutHz = juce::jlimit (20.0, juce::jmax (20.0, nyquist - 1.0), static_cast<double> (lowCutHz));
    const auto safeHighCutHz = juce::jlimit (1000.0, juce::jmax (1000.0, nyquist - 1.0), static_cast<double> (highCutHz));
    const auto lowCutCoefficients = juce::IIRCoefficients::makeHighPass (currentSampleRate, safeLowCutHz);
    const auto highCutCoefficients = juce::IIRCoefficients::makeLowPass (currentSampleRate, safeHighCutHz);

    for (auto channel = 0; channel < 2; ++channel)
    {
        lowCutFilters[channel].setCoefficients (lowCutCoefficients);
        highCutFilters[channel].setCoefficients (highCutCoefficients);
    }

    smoothedModulationDepth.setTargetValue (modulationDepth);
    smoothedModulationRate.setTargetValue (modulationRate);
    smoothedDrive.setTargetValue (drive);
    smoothedVintage.setTargetValue (vintage);

    const auto delayBufferSize = delayBuffer.getNumSamples();
    const auto baseDelaySamples = juce::jlimit (1, delayBufferSize - 1,
                                               static_cast<int> (std::round (delayTimeMs * currentSampleRate / 1000.0)));
    const auto maxModulationSamples = maxModulationDepthMs * currentSampleRate / 1000.0;
    const auto channelsToProcess = juce::jmin (2, totalNumInputChannels, totalNumOutputChannels);

    for (auto sample = 0; sample < numSamples; ++sample)
    {
        const auto depthPercent = smoothedModulationDepth.getNextValue();
        const auto rateHz = smoothedModulationRate.getNextValue();
        const auto driveAmount = smoothedDrive.getNextValue() / 100.0f;
        const auto vintageAmount = smoothedVintage.getNextValue() / 100.0f;
        const auto modulationIsActive = depthPercent > 0.0001f;
        auto effectiveDelaySamples = static_cast<double> (baseDelaySamples);
        auto readPosition = (delayWritePosition + delayBufferSize - baseDelaySamples) % delayBufferSize;

        if (modulationIsActive)
        {
            float lfoValue = std::sin (lfoPhase);
            if (modShape == 1)
                lfoValue = static_cast<float> ((2.0 / juce::MathConstants<double>::pi) * std::asin (std::sin (lfoPhase)));
            else if (modShape == 2)
                lfoValue = std::sin (lfoPhase) >= 0.0 ? 1.0f : -1.0f;
            else if (modShape == 3)
                lfoValue = static_cast<float> ((lfoPhase / juce::MathConstants<double>::pi) - 1.0);
            else if (modShape == 4)
                lfoValue = nextNoiseSample (noiseState) * 0.65f;
            lfoValue += nextNoiseSample (noiseState) * drift * 0.12f;
            const auto modulationSamples = (static_cast<double> (depthPercent) / 100.0) * maxModulationSamples * lfoValue;
            effectiveDelaySamples = juce::jlimit (1.0, static_cast<double> (delayBufferSize - 2), effectiveDelaySamples + modulationSamples);
        }

        float dry[2] { 0.0f, 0.0f };
        float wet[2] { 0.0f, 0.0f };
        for (auto channel = 0; channel < channelsToProcess; ++channel)
        {
            dry[channel] = buffer.getSample (channel, sample) * inputGain;
            const auto delayed = modulationIsActive ? readDelaySampleLinear (delayBuffer, channel, delayWritePosition, effectiveDelaySamples)
                                                    : delayBuffer.getSample (channel, readPosition);
            auto wetSignal = highCutFilters[channel].processSingleSampleRaw (lowCutFilters[channel].processSingleSampleRaw (delayed));

            const auto characterDrive = juce::jlimit (0.0f, 1.0f, driveAmount + (mode == 1 ? 0.10f : mode == 2 ? 0.18f : 0.0f));
            if (characterDrive > 0.0001f)
            {
                const auto driveGain = 1.0f + characterDrive * (mode == 2 ? 7.0f : 5.0f);
                const auto saturated = std::tanh (wetSignal * driveGain) / std::tanh (driveGain);
                wetSignal += (saturated - wetSignal) * characterDrive;
            }

            if (vintageAmount > 0.0001f || mode == 1)
            {
                const auto amount = juce::jlimit (0.0f, 1.0f, vintageAmount + (mode == 1 ? 0.12f : 0.0f));
                const auto softeningCoefficient = juce::jmap (amount, 0.95f, 0.28f);
                vintageSoftenState[channel] += softeningCoefficient * (wetSignal - vintageSoftenState[channel]);
                const auto bitLevels = juce::jmap (amount, 65536.0f, 2048.0f);
                auto degraded = std::round (vintageSoftenState[channel] * bitLevels) / bitLevels;
                degraded += nextNoiseSample (noiseState) * amount * 0.00008f;
                wetSignal += (degraded - wetSignal) * amount;
            }
            else
                vintageSoftenState[channel] = wetSignal;

            wet[channel] = wetSignal;
        }

        if (channelsToProcess == 2)
        {
            const auto mid = 0.5f * (wet[0] + wet[1]);
            const auto side = 0.5f * (wet[0] - wet[1]) * width;
            wet[0] = mid + side; wet[1] = mid - side;
        }

        for (auto channel = 0; channel < channelsToProcess; ++channel)
        {
            const auto other = channelsToProcess == 2 ? 1 - channel : channel;
            const auto feedbackSource = pingPong ? wet[other] : wet[channel];
            const auto bloomFeed = channelsToProcess == 2 ? (wet[channel] * (1.0f - bloom * 0.25f) + wet[other] * bloom * 0.25f) : wet[channel];
            const auto writeInput = freeze ? 0.0f : dry[channel];
            const auto fb = freeze ? 0.9995f : feedback;
            delayBuffer.setSample (channel, delayWritePosition, writeInput + ((feedbackSource * (1.0f - bloom * 0.35f) + bloomFeed * bloom * 0.35f) * fb));
            const auto processed = ((dry[channel] * (1.0f - mix)) + (wet[channel] * mix)) * outputGain;
            buffer.setSample (channel, sample, bypass ? dry[channel] : processed);
        }

        lfoPhase += juce::MathConstants<double>::twoPi * static_cast<double> (rateHz) / currentSampleRate;
        if (lfoPhase >= juce::MathConstants<double>::twoPi) lfoPhase -= juce::MathConstants<double>::twoPi;
        delayWritePosition = (delayWritePosition + 1) % delayBufferSize;
    }

    for (auto channel = 0; channel < 2; ++channel)
        outputRmsLevels[static_cast<size_t> (channel)].store (channel < totalNumOutputChannels ? calculateRmsLevel (buffer, channel) : 0.0f);
}

//==============================================================================
float AuroraD80AudioProcessor::getInputRmsLevel (int channel) const
{
    return inputRmsLevels[static_cast<size_t> (juce::jlimit (0, 1, channel))].load();
}

float AuroraD80AudioProcessor::getOutputRmsLevel (int channel) const
{
    return outputRmsLevels[static_cast<size_t> (juce::jlimit (0, 1, channel))].load();
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
