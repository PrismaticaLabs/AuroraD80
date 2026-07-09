/*

  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================

*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class AuroraD80AudioProcessor  : public juce::AudioProcessor
{
public:
    using APVTS = juce::AudioProcessorValueTreeState;

    //==============================================================================
    AuroraD80AudioProcessor();
    ~AuroraD80AudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    APVTS parameters;

private:
    //==============================================================================
    static APVTS::ParameterLayout createParameterLayout();

    juce::AudioBuffer<float> delayBuffer;
    std::array<juce::IIRFilter, 2> lowCutFilters;
    std::array<juce::IIRFilter, 2> highCutFilters;
    std::array<float, 2> vintageSoftenState { 0.0f, 0.0f };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedModulationDepth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedModulationRate;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDrive;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedVintage;
    int delayWritePosition = 0;
    double currentSampleRate = 44100.0;
    double lfoPhase = 0.0;
    uint32_t noiseState = 0x12345678;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuroraD80AudioProcessor)
};
