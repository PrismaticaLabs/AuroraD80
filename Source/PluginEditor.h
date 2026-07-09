/*

  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================

*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class AuroraD80AudioProcessorEditor  : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    AuroraD80AudioProcessorEditor (AuroraD80AudioProcessor&);
    ~AuroraD80AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void configureSlider (juce::Slider& slider,
                          juce::Label& nameLabel,
                          juce::Label& valueLabel,
                          const juce::String& labelText,
                          bool usesMilliseconds);
    void configureSyncControls();
    void updateValueLabel (juce::Slider& slider, juce::Label& valueLabel, bool usesMilliseconds);
    void updateTimeValueLabel();
    void timerCallback() override;

    AuroraD80AudioProcessor& audioProcessor;

    juce::Slider inputSlider;
    juce::Slider timeSlider;
    juce::Slider feedbackSlider;
    juce::Slider mixSlider;
    juce::Slider outputSlider;

    juce::Label inputLabel;
    juce::Label timeLabel;
    juce::Label feedbackLabel;
    juce::Label mixLabel;
    juce::Label outputLabel;

    juce::Label inputValueLabel;
    juce::Label timeValueLabel;
    juce::Label feedbackValueLabel;
    juce::Label mixValueLabel;
    juce::Label outputValueLabel;

    juce::ToggleButton syncButton;
    juce::ComboBox divisionBox;

    std::unique_ptr<SliderAttachment> inputAttachment;
    std::unique_ptr<SliderAttachment> timeAttachment;
    std::unique_ptr<SliderAttachment> feedbackAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> outputAttachment;
    std::unique_ptr<ButtonAttachment> syncAttachment;
    std::unique_ptr<ComboBoxAttachment> divisionAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuroraD80AudioProcessorEditor)
};
