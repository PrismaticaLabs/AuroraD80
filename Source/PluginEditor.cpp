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
    configureSlider (inputSlider, inputLabel, inputValueLabel, "INPUT", false);
    configureSlider (timeSlider, timeLabel, timeValueLabel, "TIME", true);
    configureSlider (feedbackSlider, feedbackLabel, feedbackValueLabel, "FEEDBACK", false);
    configureSlider (mixSlider, mixLabel, mixValueLabel, "MIX", false);
    configureSlider (outputSlider, outputLabel, outputValueLabel, "OUTPUT", false);
    configureSyncControls();

    timeSlider.onValueChange = [this]
    {
        updateTimeValueLabel();
    };

    inputAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "inputGain", inputSlider);
    timeAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "delayTimeMs", timeSlider);
    feedbackAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "feedback", feedbackSlider);
    mixAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "mix", mixSlider);
    outputAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "outputGain", outputSlider);
    syncAttachment = std::make_unique<ButtonAttachment> (audioProcessor.parameters, "syncEnabled", syncButton);
    divisionAttachment = std::make_unique<ComboBoxAttachment> (audioProcessor.parameters, "syncDivision", divisionBox);

    updateValueLabel (inputSlider, inputValueLabel, false);
    updateTimeValueLabel();
    updateValueLabel (feedbackSlider, feedbackValueLabel, false);
    updateValueLabel (mixSlider, mixValueLabel, false);
    updateValueLabel (outputSlider, outputValueLabel, false);

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
                                                     bool usesMilliseconds)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffff6a1a));
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff3a2520));
    slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffffb36f));
    slider.onValueChange = [this, &slider, &valueLabel, usesMilliseconds]
    {
        updateValueLabel (slider, valueLabel, usesMilliseconds);
    };
    addAndMakeVisible (slider);

    nameLabel.setText (labelText, juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setColour (juce::Label::textColourId, juce::Colour (0xffff8a3d));
    nameLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    addAndMakeVisible (nameLabel);

    valueLabel.setJustificationType (juce::Justification::centred);
    valueLabel.setColour (juce::Label::textColourId, juce::Colour (0xffffdcc6));
    valueLabel.setFont (juce::FontOptions (15.0f));
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
                                                      bool usesMilliseconds)
{
    if (usesMilliseconds)
    {
        valueLabel.setText (juce::String (juce::roundToInt (slider.getValue())) + " ms", juce::dontSendNotification);
        return;
    }

    valueLabel.setText (juce::String (juce::roundToInt (slider.getValue() * 100.0)) + "%", juce::dontSendNotification);
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

    updateValueLabel (timeSlider, timeValueLabel, true);
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
    auto controlsArea = getLocalBounds().withTrimmedTop (136).withTrimmedBottom (38).reduced (62, 0);
    const auto controlWidth = controlsArea.getWidth() / 5;

    juce::Slider* sliders[] = { &inputSlider, &timeSlider, &feedbackSlider, &mixSlider, &outputSlider };
    juce::Label* nameLabels[] = { &inputLabel, &timeLabel, &feedbackLabel, &mixLabel, &outputLabel };
    juce::Label* valueLabels[] = { &inputValueLabel, &timeValueLabel, &feedbackValueLabel, &mixValueLabel, &outputValueLabel };

    for (auto index = 0; index < 5; ++index)
    {
        auto controlBounds = controlsArea.removeFromLeft (controlWidth).reduced (18, 0);
        auto centredBounds = controlBounds.withSizeKeepingCentre (118, controlBounds.getHeight());

        nameLabels[index]->setBounds (centredBounds.removeFromTop (24));
        centredBounds.removeFromTop (6);
        sliders[index]->setBounds (centredBounds.removeFromTop (128));
        centredBounds.removeFromTop (4);
        valueLabels[index]->setBounds (centredBounds.removeFromTop (24));

        if (index == 1)
        {
            centredBounds.removeFromTop (8);
            syncButton.setBounds (centredBounds.removeFromTop (22));
            centredBounds.removeFromTop (6);
            divisionBox.setBounds (centredBounds.removeFromTop (24));
        }
    }
}
