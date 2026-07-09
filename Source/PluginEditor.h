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

    class AuroraLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        AuroraLookAndFeel();

        void drawRotarySlider (juce::Graphics& g,
                               int x,
                               int y,
                               int width,
                               int height,
                               float sliderPosProportional,
                               float rotaryStartAngle,
                               float rotaryEndAngle,
                               juce::Slider& slider) override;

        void drawComboBox (juce::Graphics& g,
                           int width,
                           int height,
                           bool isButtonDown,
                           int buttonX,
                           int buttonY,
                           int buttonW,
                           int buttonH,
                           juce::ComboBox& box) override;

        juce::Font getComboBoxFont (juce::ComboBox& box) override;

        void drawToggleButton (juce::Graphics& g,
                               juce::ToggleButton& button,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;
    };

    class PresetArrowButton final : public juce::Button
    {
    public:
        explicit PresetArrowButton (const juce::String& arrowText);

        void paintButton (juce::Graphics& g,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    private:
        juce::String arrow;
    };

    enum class ValueFormat
    {
        Percent,
        ModPercent,
        Milliseconds,
        Hz,
        KHz,
        RateHz
    };

    void configureSlider (juce::Slider& slider,
                          juce::Label& nameLabel,
                          juce::Label& valueLabel,
                          const juce::String& labelText,
                          ValueFormat valueFormat);
    void configureSyncControls();
    void configurePresetBrowser();
    void showPreviousPreset();
    void showNextPreset();
    juce::String getCurrentPresetName() const;
    void updateValueLabel (juce::Slider& slider, juce::Label& valueLabel, ValueFormat valueFormat);
    void updateTimeValueLabel();
    void updateMeterLevels();
    void timerCallback() override;

    AuroraLookAndFeel lookAndFeel;
    AuroraD80AudioProcessor& audioProcessor;

    juce::Slider inputSlider;
    juce::Slider timeSlider;
    juce::Slider lowCutSlider;
    juce::Slider highCutSlider;
    juce::Slider depthSlider;
    juce::Slider rateSlider;
    juce::Slider driveSlider;
    juce::Slider vintageSlider;
    juce::Slider feedbackSlider;
    juce::Slider mixSlider;
    juce::Slider outputSlider;

    juce::Label inputLabel;
    juce::Label timeLabel;
    juce::Label lowCutLabel;
    juce::Label highCutLabel;
    juce::Label depthLabel;
    juce::Label rateLabel;
    juce::Label driveLabel;
    juce::Label vintageLabel;
    juce::Label feedbackLabel;
    juce::Label mixLabel;
    juce::Label outputLabel;

    juce::Label inputValueLabel;
    juce::Label timeValueLabel;
    juce::Label lowCutValueLabel;
    juce::Label highCutValueLabel;
    juce::Label depthValueLabel;
    juce::Label rateValueLabel;
    juce::Label driveValueLabel;
    juce::Label vintageValueLabel;
    juce::Label feedbackValueLabel;
    juce::Label mixValueLabel;
    juce::Label outputValueLabel;

    juce::ToggleButton syncButton;
    juce::ComboBox divisionBox;
    PresetArrowButton previousPresetButton { "<" };
    PresetArrowButton nextPresetButton { ">" };
    juce::TextButton presetNameButton;
    std::vector<juce::String> presetNames { "Deep Horizon", "Neon Rain", "Frozen Echoes", "Midnight Bloom", "Ghost Signal", "Infinite Sky" };
    int currentPresetIndex = 0;
    std::array<float, 2> inputMeterLevels { 0.0f, 0.0f };
    std::array<float, 2> outputMeterLevels { 0.0f, 0.0f };

    std::unique_ptr<SliderAttachment> inputAttachment;
    std::unique_ptr<SliderAttachment> timeAttachment;
    std::unique_ptr<SliderAttachment> lowCutAttachment;
    std::unique_ptr<SliderAttachment> highCutAttachment;
    std::unique_ptr<SliderAttachment> depthAttachment;
    std::unique_ptr<SliderAttachment> rateAttachment;
    std::unique_ptr<SliderAttachment> driveAttachment;
    std::unique_ptr<SliderAttachment> vintageAttachment;
    std::unique_ptr<SliderAttachment> feedbackAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> outputAttachment;
    std::unique_ptr<ButtonAttachment> syncAttachment;
    std::unique_ptr<ComboBoxAttachment> divisionAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuroraD80AudioProcessorEditor)
};
