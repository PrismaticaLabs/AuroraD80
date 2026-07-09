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
    auto controlsArea = getLocalBounds().withTrimmedTop (136).withTrimmedBottom (38).reduced (28, 0);
    const auto controlWidth = controlsArea.getWidth() / 9;

    juce::Slider* sliders[] = { &inputSlider, &timeSlider, &lowCutSlider, &highCutSlider, &depthSlider, &rateSlider, &feedbackSlider, &mixSlider, &outputSlider };
    juce::Label* nameLabels[] = { &inputLabel, &timeLabel, &lowCutLabel, &highCutLabel, &depthLabel, &rateLabel, &feedbackLabel, &mixLabel, &outputLabel };
    juce::Label* valueLabels[] = { &inputValueLabel, &timeValueLabel, &lowCutValueLabel, &highCutValueLabel, &depthValueLabel, &rateValueLabel, &feedbackValueLabel, &mixValueLabel, &outputValueLabel };

    for (auto index = 0; index < 9; ++index)
    {
        auto controlBounds = controlsArea.removeFromLeft (controlWidth).reduced (5, 0);
        auto centredBounds = controlBounds.withSizeKeepingCentre (76, controlBounds.getHeight());

        nameLabels[index]->setBounds (centredBounds.removeFromTop (22));
        centredBounds.removeFromTop (5);
        sliders[index]->setBounds (centredBounds.removeFromTop (104));
        centredBounds.removeFromTop (3);
        valueLabels[index]->setBounds (centredBounds.removeFromTop (22));

        if (index == 1)
        {
            centredBounds.removeFromTop (7);
            syncButton.setBounds (centredBounds.removeFromTop (22));
            centredBounds.removeFromTop (5);
            divisionBox.setBounds (centredBounds.removeFromTop (24));
        }
    }
}
