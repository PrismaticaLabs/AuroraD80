/*

  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================

*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AuroraD80AudioProcessorEditor::AuroraD80AudioProcessorEditor (AuroraD80AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    configureSlider (inputSlider, inputLabel, inputValueLabel, "INPUT", ValueFormat::Percent);
    configureSlider (timeSlider, timeLabel, timeValueLabel, "TIME", ValueFormat::Milliseconds);
    configureSlider (lowCutSlider, lowCutLabel, lowCutValueLabel, "LOW CUT", ValueFormat::Hz);
    configureSlider (highCutSlider, highCutLabel, highCutValueLabel, "HIGH CUT", ValueFormat::KHz);
    configureSlider (depthSlider, depthLabel, depthValueLabel, "DEPTH", ValueFormat::ModPercent);
    configureSlider (rateSlider, rateLabel, rateValueLabel, "RATE", ValueFormat::RateHz);
    configureSlider (driveSlider, driveLabel, driveValueLabel, "DRIVE", ValueFormat::ModPercent);
    configureSlider (vintageSlider, vintageLabel, vintageValueLabel, "VINTAGE", ValueFormat::ModPercent);
    configureSlider (feedbackSlider, feedbackLabel, feedbackValueLabel, "FEEDBACK", ValueFormat::Percent);
    configureSlider (mixSlider, mixLabel, mixValueLabel, "MIX", ValueFormat::Percent);
    configureSlider (outputSlider, outputLabel, outputValueLabel, "OUTPUT", ValueFormat::Percent);
    configureSyncControls();

    timeSlider.onValueChange = [this]
    {
        updateTimeValueLabel();
    };

    inputAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "inputGain", inputSlider);
    timeAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "delayTimeMs", timeSlider);
    lowCutAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "lowCutHz", lowCutSlider);
    highCutAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "highCutHz", highCutSlider);
    depthAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "modulationDepth", depthSlider);
    rateAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "modulationRate", rateSlider);
    driveAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "drive", driveSlider);
    vintageAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "vintage", vintageSlider);
    feedbackAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "feedback", feedbackSlider);
    mixAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "mix", mixSlider);
    outputAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "outputGain", outputSlider);
    syncAttachment = std::make_unique<ButtonAttachment> (audioProcessor.parameters, "syncEnabled", syncButton);
    divisionAttachment = std::make_unique<ComboBoxAttachment> (audioProcessor.parameters, "syncDivision", divisionBox);

    updateValueLabel (inputSlider, inputValueLabel, ValueFormat::Percent);
    updateTimeValueLabel();
    updateValueLabel (lowCutSlider, lowCutValueLabel, ValueFormat::Hz);
    updateValueLabel (highCutSlider, highCutValueLabel, ValueFormat::KHz);
    updateValueLabel (depthSlider, depthValueLabel, ValueFormat::ModPercent);
    updateValueLabel (rateSlider, rateValueLabel, ValueFormat::RateHz);
    updateValueLabel (driveSlider, driveValueLabel, ValueFormat::ModPercent);
    updateValueLabel (vintageSlider, vintageValueLabel, ValueFormat::ModPercent);
    updateValueLabel (feedbackSlider, feedbackValueLabel, ValueFormat::Percent);
    updateValueLabel (mixSlider, mixValueLabel, ValueFormat::Percent);
    updateValueLabel (outputSlider, outputValueLabel, ValueFormat::Percent);

    startTimerHz (15);
    setSize (800, 420);
}

AuroraD80AudioProcessorEditor::~AuroraD80AudioProcessorEditor()
{
}

void AuroraD80AudioProcessorEditor::configureSlider (juce::Slider& slider,
                                                     juce::Label& nameLabel,
                                                     juce::Label& valueLabel,
                                                     const juce::String& labelText,
                                                     ValueFormat valueFormat)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffff6a1a));
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff3a2520));
    slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffffb36f));
    slider.onValueChange = [this, &slider, &valueLabel, valueFormat]
    {
        updateValueLabel (slider, valueLabel, valueFormat);
    };
    addAndMakeVisible (slider);

    nameLabel.setText (labelText, juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setColour (juce::Label::textColourId, juce::Colour (0xffff8a3d));
    nameLabel.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    addAndMakeVisible (nameLabel);

    valueLabel.setJustificationType (juce::Justification::centred);
    valueLabel.setColour (juce::Label::textColourId, juce::Colour (0xffffdcc6));
    valueLabel.setFont (juce::FontOptions (13.0f));
    addAndMakeVisible (valueLabel);
}

void AuroraD80AudioProcessorEditor::configureSyncControls()
{
    syncButton.setButtonText ("SYNC");
    syncButton.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffffdcc6));
    syncButton.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xffff6a1a));
    syncButton.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff5d3728));
    syncButton.onClick = [this]
    {
        updateTimeValueLabel();
    };
    addAndMakeVisible (syncButton);

    divisionBox.addItemList (juce::StringArray { "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1 Bar" }, 1);
    divisionBox.setSelectedId (5, juce::dontSendNotification);
    divisionBox.setJustificationType (juce::Justification::centred);
    divisionBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff17110f));
    divisionBox.setColour (juce::ComboBox::textColourId, juce::Colour (0xffffdcc6));
    divisionBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3f241b));
    divisionBox.setColour (juce::ComboBox::arrowColourId, juce::Colour (0xffff8a3d));
    divisionBox.onChange = [this]
    {
        updateTimeValueLabel();
    };
    addAndMakeVisible (divisionBox);
}

void AuroraD80AudioProcessorEditor::updateValueLabel (juce::Slider& slider,
                                                      juce::Label& valueLabel,
                                                      ValueFormat valueFormat)
{
    switch (valueFormat)
    {
        case ValueFormat::Percent:
            valueLabel.setText (juce::String (juce::roundToInt (slider.getValue() * 100.0)) + "%", juce::dontSendNotification);
            break;

        case ValueFormat::ModPercent:
            valueLabel.setText (juce::String (juce::roundToInt (slider.getValue())) + " %", juce::dontSendNotification);
            break;

        case ValueFormat::Milliseconds:
            valueLabel.setText (juce::String (juce::roundToInt (slider.getValue())) + " ms", juce::dontSendNotification);
            break;

        case ValueFormat::Hz:
            valueLabel.setText (juce::String (juce::roundToInt (slider.getValue())) + " Hz", juce::dontSendNotification);
            break;

        case ValueFormat::KHz:
            valueLabel.setText (juce::String (slider.getValue() / 1000.0, 1) + " kHz", juce::dontSendNotification);
            break;

        case ValueFormat::RateHz:
            valueLabel.setText (juce::String (slider.getValue(), 2) + " Hz", juce::dontSendNotification);
            break;
    }
}

void AuroraD80AudioProcessorEditor::updateTimeValueLabel()
{
    const auto syncIsEnabled = syncButton.getToggleState();
    divisionBox.setEnabled (syncIsEnabled);

    if (syncIsEnabled)
    {
        auto divisionText = divisionBox.getText();

        if (divisionText.isEmpty())
            divisionText = "1/4";

        timeValueLabel.setText (divisionText, juce::dontSendNotification);
        return;
    }

    updateValueLabel (timeSlider, timeValueLabel, ValueFormat::Milliseconds);
}

void AuroraD80AudioProcessorEditor::timerCallback()
{
    updateTimeValueLabel();
}

//==============================================================================
void AuroraD80AudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.fillAll (juce::Colour (0xff100d0d));

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a1110), 0.0f, 0.0f,
                                             juce::Colour (0xff0b0a0a), 0.0f, bounds.getHeight(), false));
    g.fillRect (bounds);

    g.setColour (juce::Colour (0x44ff5a1f));
    g.drawRoundedRectangle (bounds.reduced (20.0f), 10.0f, 1.5f);

    g.setColour (juce::Colour (0xffff6a1a));
    g.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    g.drawText ("AURORA D80", 0, 38, getWidth(), 38, juce::Justification::centred);

    g.setColour (juce::Colour (0xffffb36f));
    g.setFont (juce::FontOptions (16.0f));
    g.drawText ("Cinematic Digital Delay", 0, 78, getWidth(), 28, juce::Justification::centred);
}

void AuroraD80AudioProcessorEditor::resized()
{
    auto controlsArea = getLocalBounds().withTrimmedTop (122).withTrimmedBottom (24).reduced (42, 0);
    auto topRow = controlsArea.removeFromTop (134);
    controlsArea.removeFromTop (4);
    auto bottomRow = controlsArea.removeFromTop (134);

    auto layoutControl = [] (juce::Rectangle<int> row,
                             int index,
                             int count,
                             juce::Slider& slider,
                             juce::Label& nameLabel,
                             juce::Label& valueLabel) -> juce::Rectangle<int>
    {
        const auto controlWidth = row.getWidth() / count;
        auto controlBounds = row.withTrimmedLeft (controlWidth * index)
                                .withWidth (controlWidth)
                                .reduced (10, 0);
        auto centredBounds = controlBounds.withSizeKeepingCentre (82, controlBounds.getHeight());

        nameLabel.setBounds (centredBounds.removeFromTop (20));
        centredBounds.removeFromTop (2);
        slider.setBounds (centredBounds.removeFromTop (80));
        centredBounds.removeFromTop (2);
        valueLabel.setBounds (centredBounds.removeFromTop (20));

        return centredBounds;
    };

    layoutControl (topRow, 0, 6, inputSlider, inputLabel, inputValueLabel);
    auto timeRemainder = layoutControl (topRow, 1, 6, timeSlider, timeLabel, timeValueLabel);
    layoutControl (topRow, 2, 6, lowCutSlider, lowCutLabel, lowCutValueLabel);
    layoutControl (topRow, 3, 6, highCutSlider, highCutLabel, highCutValueLabel);
    layoutControl (topRow, 4, 6, depthSlider, depthLabel, depthValueLabel);
    layoutControl (topRow, 5, 6, rateSlider, rateLabel, rateValueLabel);

    timeRemainder.removeFromTop (3);
    syncButton.setBounds (timeRemainder.removeFromTop (20));
    timeRemainder.removeFromTop (3);
    divisionBox.setBounds (timeRemainder.removeFromTop (23));

    layoutControl (bottomRow, 0, 5, driveSlider, driveLabel, driveValueLabel);
    layoutControl (bottomRow, 1, 5, vintageSlider, vintageLabel, vintageValueLabel);
    layoutControl (bottomRow, 2, 5, feedbackSlider, feedbackLabel, feedbackValueLabel);
    layoutControl (bottomRow, 3, 5, mixSlider, mixLabel, mixValueLabel);
    layoutControl (bottomRow, 4, 5, outputSlider, outputLabel, outputValueLabel);
}
