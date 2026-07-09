/*

  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================

*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr auto backgroundTop = 0xff191716;
    constexpr auto backgroundBottom = 0xff0d0c0c;
    constexpr auto headerTop = 0xff242120;
    constexpr auto headerBottom = 0xff151313;
    constexpr auto panelFill = 0xff1f1d1c;
    constexpr auto panelBorder = 0xff3a3330;
    constexpr auto panelHighlight = 0x22ffffff;
    constexpr auto orange = 0xffff6a1a;
    constexpr auto softOrange = 0xffff8a3d;
    constexpr auto lightGrey = 0xffc8c0ba;
    constexpr auto valueWhite = 0xfffff1e8;

    struct RackLayout
    {
        juce::Rectangle<int> header;
        juce::Rectangle<int> display;
        juce::Rectangle<int> input;
        juce::Rectangle<int> delay;
        juce::Rectangle<int> tone;
        juce::Rectangle<int> modulation;
        juce::Rectangle<int> character;
        juce::Rectangle<int> output;
    };

    RackLayout getRackLayout (juce::Rectangle<int> bounds)
    {
        RackLayout layout;
        layout.header = bounds.removeFromTop (60);
        layout.display = { 250, 74, 300, 48 };
        layout.input = { 18, 138, 110, 124 };
        layout.delay = { 138, 138, 300, 124 };
        layout.tone = { 448, 138, 190, 124 };
        layout.output = { 648, 138, 134, 124 };
        layout.modulation = { 168, 278, 220, 124 };
        layout.character = { 412, 278, 220, 124 };
        return layout;
    }

    void drawPanel (juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title)
    {
        const auto panel = bounds.toFloat();

        g.setColour (juce::Colour (panelFill));
        g.fillRoundedRectangle (panel, 7.0f);

        g.setColour (juce::Colour (panelHighlight));
        g.drawRoundedRectangle (panel.reduced (1.0f), 7.0f, 1.0f);

        g.setColour (juce::Colour (panelBorder));
        g.drawRoundedRectangle (panel, 7.0f, 1.0f);

        g.setColour (juce::Colour (softOrange));
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (title, bounds.withHeight (22).reduced (10, 0), juce::Justification::centredLeft);

        g.setColour (juce::Colour (0x25ff6a1a));
        g.drawLine (static_cast<float> (bounds.getX() + 10),
                    static_cast<float> (bounds.getY() + 24),
                    static_cast<float> (bounds.getRight() - 10),
                    static_cast<float> (bounds.getY() + 24),
                    1.0f);
    }

    juce::Rectangle<int> panelContent (juce::Rectangle<int> panel)
    {
        return panel.reduced (10).withTrimmedTop (20);
    }

    juce::Rectangle<int> layoutKnob (juce::Rectangle<int> area,
                                     int index,
                                     int count,
                                     juce::Slider& slider,
                                     juce::Label& nameLabel,
                                     juce::Label& valueLabel,
                                     int maxWidth = 78)
    {
        const auto controlWidth = area.getWidth() / count;
        auto controlBounds = area.withTrimmedLeft (controlWidth * index)
                               .withWidth (controlWidth)
                               .reduced (5, 0);
        auto centredBounds = controlBounds.withSizeKeepingCentre (juce::jmin (maxWidth, controlBounds.getWidth()), controlBounds.getHeight());

        nameLabel.setBounds (centredBounds.removeFromTop (16));
        centredBounds.removeFromTop (1);
        slider.setBounds (centredBounds.removeFromTop (62));
        centredBounds.removeFromTop (1);
        valueLabel.setBounds (centredBounds.removeFromTop (18));

        return centredBounds;
    }

    void layoutSingleKnobPanel (juce::Rectangle<int> panel,
                                juce::Slider& slider,
                                juce::Label& nameLabel,
                                juce::Label& valueLabel)
    {
        layoutKnob (panelContent (panel), 0, 1, slider, nameLabel, valueLabel, 82);
    }
}

//==============================================================================
AuroraD80AudioProcessorEditor::AuroraD80AudioProcessorEditor (AuroraD80AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    configureSlider (inputSlider, inputLabel, inputValueLabel, "Input", ValueFormat::Percent);
    configureSlider (timeSlider, timeLabel, timeValueLabel, "Time", ValueFormat::Milliseconds);
    configureSlider (lowCutSlider, lowCutLabel, lowCutValueLabel, "Low Cut", ValueFormat::Hz);
    configureSlider (highCutSlider, highCutLabel, highCutValueLabel, "High Cut", ValueFormat::KHz);
    configureSlider (depthSlider, depthLabel, depthValueLabel, "Depth", ValueFormat::ModPercent);
    configureSlider (rateSlider, rateLabel, rateValueLabel, "Rate", ValueFormat::RateHz);
    configureSlider (driveSlider, driveLabel, driveValueLabel, "Drive", ValueFormat::ModPercent);
    configureSlider (vintageSlider, vintageLabel, vintageValueLabel, "Vintage", ValueFormat::ModPercent);
    configureSlider (feedbackSlider, feedbackLabel, feedbackValueLabel, "Feedback", ValueFormat::Percent);
    configureSlider (mixSlider, mixLabel, mixValueLabel, "Mix", ValueFormat::Percent);
    configureSlider (outputSlider, outputLabel, outputValueLabel, "Output", ValueFormat::Percent);
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
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (orange));
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff403733));
    slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffffb36f));
    slider.onValueChange = [this, &slider, &valueLabel, valueFormat]
    {
        updateValueLabel (slider, valueLabel, valueFormat);
    };
    addAndMakeVisible (slider);

    nameLabel.setText (labelText, juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setColour (juce::Label::textColourId, juce::Colour (lightGrey));
    nameLabel.setFont (juce::FontOptions (11.0f));
    addAndMakeVisible (nameLabel);

    valueLabel.setJustificationType (juce::Justification::centred);
    valueLabel.setColour (juce::Label::textColourId, juce::Colour (valueWhite));
    valueLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    addAndMakeVisible (valueLabel);
}

void AuroraD80AudioProcessorEditor::configureSyncControls()
{
    syncButton.setButtonText ("SYNC");
    syncButton.setColour (juce::ToggleButton::textColourId, juce::Colour (lightGrey));
    syncButton.setColour (juce::ToggleButton::tickColourId, juce::Colour (orange));
    syncButton.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff5d3728));
    syncButton.onClick = [this]
    {
        updateTimeValueLabel();
    };
    addAndMakeVisible (syncButton);

    divisionBox.addItemList (juce::StringArray { "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1 Bar" }, 1);
    divisionBox.setSelectedId (5, juce::dontSendNotification);
    divisionBox.setJustificationType (juce::Justification::centred);
    divisionBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff171514));
    divisionBox.setColour (juce::ComboBox::textColourId, juce::Colour (valueWhite));
    divisionBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff4a3f39));
    divisionBox.setColour (juce::ComboBox::arrowColourId, juce::Colour (softOrange));
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
    const auto bounds = getLocalBounds();
    const auto layout = getRackLayout (bounds);

    g.setGradientFill (juce::ColourGradient (juce::Colour (backgroundTop), 0.0f, 0.0f,
                                             juce::Colour (backgroundBottom), 0.0f, static_cast<float> (getHeight()), false));
    g.fillRect (bounds);

    g.setGradientFill (juce::ColourGradient (juce::Colour (headerTop), 0.0f, 0.0f,
                                             juce::Colour (headerBottom), 0.0f, static_cast<float> (layout.header.getBottom()), false));
    g.fillRect (layout.header);

    g.setColour (juce::Colour (orange));
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("AURORA D80", layout.header.withTrimmedBottom (18), juce::Justification::centred);

    g.setColour (juce::Colour (lightGrey));
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("Cinematic Digital Delay", layout.header.withTrimmedTop (36), juce::Justification::centred);

    g.setColour (juce::Colour (0xff3c3430));
    g.drawHorizontalLine (layout.header.getBottom(), 0.0f, static_cast<float> (getWidth()));

    const auto display = layout.display.toFloat();
    g.setColour (juce::Colour (0xff0c0f0f));
    g.fillRoundedRectangle (display, 5.0f);
    g.setColour (juce::Colour (0xff403833));
    g.drawRoundedRectangle (display, 5.0f, 1.0f);
    g.setColour (juce::Colour (0x18ff6a1a));
    g.drawRoundedRectangle (display.reduced (3.0f), 3.0f, 1.0f);
    g.setColour (juce::Colour (0xff766d67));
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("DISPLAY", layout.display, juce::Justification::centred);

    drawPanel (g, layout.input, "INPUT");
    drawPanel (g, layout.delay, "DELAY");
    drawPanel (g, layout.tone, "TONE");
    drawPanel (g, layout.modulation, "MODULATION");
    drawPanel (g, layout.character, "CHARACTER");
    drawPanel (g, layout.output, "OUTPUT");
}

void AuroraD80AudioProcessorEditor::resized()
{
    const auto layout = getRackLayout (getLocalBounds());

    layoutSingleKnobPanel (layout.input, inputSlider, inputLabel, inputValueLabel);
    layoutSingleKnobPanel (layout.output, outputSlider, outputLabel, outputValueLabel);

    auto delayArea = panelContent (layout.delay);
    auto timeArea = delayArea.removeFromLeft (70).reduced (2, 0);
    layoutKnob (timeArea, 0, 1, timeSlider, timeLabel, timeValueLabel, 64);

    auto syncArea = delayArea.removeFromLeft (78).reduced (4, 0);
    syncArea.removeFromTop (12);
    syncButton.setBounds (syncArea.removeFromTop (22));
    syncArea.removeFromTop (8);
    divisionBox.setBounds (syncArea.removeFromTop (24));

    layoutKnob (delayArea, 0, 2, feedbackSlider, feedbackLabel, feedbackValueLabel, 64);
    layoutKnob (delayArea, 1, 2, mixSlider, mixLabel, mixValueLabel, 64);

    layoutKnob (panelContent (layout.tone), 0, 2, lowCutSlider, lowCutLabel, lowCutValueLabel, 72);
    layoutKnob (panelContent (layout.tone), 1, 2, highCutSlider, highCutLabel, highCutValueLabel, 72);

    layoutKnob (panelContent (layout.modulation), 0, 2, depthSlider, depthLabel, depthValueLabel, 74);
    layoutKnob (panelContent (layout.modulation), 1, 2, rateSlider, rateLabel, rateValueLabel, 74);

    layoutKnob (panelContent (layout.character), 0, 2, driveSlider, driveLabel, driveValueLabel, 74);
    layoutKnob (panelContent (layout.character), 1, 2, vintageSlider, vintageLabel, vintageValueLabel, 74);
}
