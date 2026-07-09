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
        juce::Rectangle<int> presetBrowser;
        juce::Rectangle<int> display;
        juce::Rectangle<int> inputMeter;
        juce::Rectangle<int> outputMeter;
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
        layout.presetBrowser = { 250, 64, 300, 20 };
        layout.display = { 250, 88, 300, 48 };
        layout.inputMeter = { 1, 138, 26, 264 };
        layout.outputMeter = { 773, 138, 26, 264 };
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

        nameLabel.setBounds (centredBounds.removeFromTop (15));
        centredBounds.removeFromTop (1);
        slider.setBounds (centredBounds.removeFromTop (54));
        centredBounds.removeFromTop (3);
        valueLabel.setBounds (centredBounds.removeFromTop (24));

        return centredBounds;
    }

    void layoutSingleKnobPanel (juce::Rectangle<int> panel,
                                juce::Slider& slider,
                                juce::Label& nameLabel,
                                juce::Label& valueLabel)
    {
        layoutKnob (panelContent (panel), 0, 1, slider, nameLabel, valueLabel, 82);
    }

    void drawStereoMeter (juce::Graphics& g,
                          juce::Rectangle<int> bounds,
                          const juce::String& title,
                          const std::array<float, 2>& levels)
    {
        constexpr auto numSegments = 20;
        const auto meter = bounds.toFloat();

        g.setColour (juce::Colour (0xff030303).withAlpha (0.65f));
        g.fillRoundedRectangle (meter.translated (0.0f, 2.0f), 5.0f);

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2825), meter.getCentreX(), meter.getY(),
                                                 juce::Colour (0xff070606), meter.getCentreX(), meter.getBottom(), false));
        g.fillRoundedRectangle (meter, 5.0f);

        g.setColour (juce::Colour (0xff090807));
        g.fillRoundedRectangle (meter.reduced (2.0f), 3.5f);

        g.setColour (juce::Colour (0xff4a4039));
        g.drawRoundedRectangle (meter, 5.0f, 1.0f);
        g.setColour (juce::Colour (0x20ff6a1a));
        g.drawRoundedRectangle (meter.reduced (1.5f), 4.0f, 0.8f);

        auto content = bounds.reduced (3, 5);
        auto titleArea = content.removeFromTop (35);
        auto labelArea = content.removeFromBottom (13);
        auto segmentArea = content.reduced (0, 2);
        auto scaleArea = segmentArea.removeFromLeft (9);
        segmentArea.removeFromLeft (1);

        const auto segmentGap = 2;
        const auto segmentHeight = juce::jmax (5, (segmentArea.getHeight() - ((numSegments - 1) * segmentGap)) / numSegments);
        const auto columnGap = 2;
        const auto columnWidth = juce::jmax (5, (segmentArea.getWidth() - columnGap) / 2);

        g.setColour (juce::Colour (softOrange));
        g.setFont (juce::FontOptions (6.4f, juce::Font::bold));

        for (auto letter = 0; letter < title.length(); ++letter)
            g.drawText (title.substring (letter, letter + 1),
                        titleArea.removeFromTop (6),
                        juce::Justification::centred,
                        false);

        g.setFont (juce::FontOptions (5.7f));
        g.setColour (juce::Colour (0xffa9a19b));

        const int scaleValues[] = { 0, -6, -12, -18, -24, -36 };
        for (auto value : scaleValues)
        {
            const auto normalised = juce::jlimit (0.0f, 1.0f, (static_cast<float> (value) + 36.0f) / 36.0f);
            const auto y = segmentArea.getY() + juce::roundToInt ((1.0f - normalised) * static_cast<float> (segmentArea.getHeight() - 8));
            g.drawText (juce::String (value), scaleArea.withY (y).withHeight (8), juce::Justification::centredRight, false);
        }

        for (auto channel = 0; channel < 2; ++channel)
        {
            const auto level = juce::jmax (0.0f, levels[static_cast<size_t> (channel)]);
            const auto levelDb = juce::Decibels::gainToDecibels (level, -60.0f);
            const auto normalisedLevel = juce::jlimit (0.0f, 1.0f, (levelDb + 36.0f) / 36.0f);
            const auto litSegments = juce::roundToInt (normalisedLevel * static_cast<float> (numSegments));
            const auto x = segmentArea.getX() + (channel * (columnWidth + columnGap));

            for (auto segment = 0; segment < numSegments; ++segment)
            {
                const auto y = segmentArea.getBottom() - ((segment + 1) * segmentHeight) - (segment * segmentGap);
                const auto led = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                        static_cast<float> (columnWidth), static_cast<float> (segmentHeight));
                const auto isLit = segment < litSegments;
                const auto colour = segment >= 17 ? juce::Colour (0xffff3327)
                                  : segment >= 13 ? juce::Colour (0xffffc63d)
                                                  : juce::Colour (0xff27d75f);

                if (isLit)
                {
                    g.setColour (colour.withAlpha (0.28f));
                    g.fillRoundedRectangle (led.expanded (1.4f, 0.8f), 2.0f);
                }

                g.setColour (isLit ? colour : juce::Colour (0xff151211));
                g.fillRoundedRectangle (led, 1.2f);

                g.setColour (isLit ? juce::Colour (0x40ffffff) : juce::Colour (0x10ffffff));
                g.drawHorizontalLine (y, static_cast<float> (x + 1), static_cast<float> (x + columnWidth - 1));
            }
        }

        auto ledLabelArea = labelArea.withTrimmedLeft (10);
        g.setColour (juce::Colour (valueWhite));
        g.setFont (juce::FontOptions (7.0f, juce::Font::bold));
        g.drawText ("L", ledLabelArea.withWidth (columnWidth), juce::Justification::centred, false);
        g.drawText ("R", ledLabelArea.withTrimmedLeft (columnWidth + columnGap), juce::Justification::centred, false);
    }
}

AuroraD80AudioProcessorEditor::AuroraLookAndFeel::AuroraLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff151312));
    setColour (juce::PopupMenu::textColourId, juce::Colour (valueWhite));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff3a2118));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xffffdcc6));
}

void AuroraD80AudioProcessorEditor::AuroraLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                                                         int x,
                                                                         int y,
                                                                         int width,
                                                                         int height,
                                                                         float sliderPosProportional,
                                                                         float rotaryStartAngle,
                                                                         float rotaryEndAngle,
                                                                         juce::Slider& slider)
{
    juce::ignoreUnused (slider);

    const auto bounds = juce::Rectangle<float> (static_cast<float> (x),
                                                static_cast<float> (y),
                                                static_cast<float> (width),
                                                static_cast<float> (height)).reduced (4.0f);
    const auto diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
    const auto knob = bounds.withSizeKeepingCentre (diameter, diameter).reduced (3.0f);
    const auto radius = knob.getWidth() * 0.5f;
    const auto centre = knob.getCentre();
    const auto angle = rotaryStartAngle + (sliderPosProportional * (rotaryEndAngle - rotaryStartAngle));

    g.setColour (juce::Colour (0xff080707).withAlpha (0.65f));
    g.fillEllipse (knob.translated (0.0f, 2.0f));

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff34302d), centre.x, knob.getY(),
                                             juce::Colour (0xff11100f), centre.x, knob.getBottom(), false));
    g.fillEllipse (knob);

    g.setColour (juce::Colour (0xff4a433e));
    g.drawEllipse (knob, 1.2f);

    g.setColour (juce::Colour (0xff171514));
    g.fillEllipse (knob.reduced (7.0f));

    g.setColour (juce::Colour (0xff2b2724));
    g.drawEllipse (knob.reduced (7.0f), 1.0f);

    const auto arcBounds = knob.reduced (2.5f);
    juce::Path backgroundArc;
    backgroundArc.addCentredArc (centre.x, centre.y, radius - 2.5f, radius - 2.5f,
                                 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff3a302b));
    g.strokePath (backgroundArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y, radius - 2.5f, radius - 2.5f,
                            0.0f, rotaryStartAngle, angle, true);
    g.setColour (juce::Colour (orange));
    g.strokePath (valueArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto indicatorRadius = radius - 13.0f;
    const auto indicator = centre.getPointOnCircumference (indicatorRadius, angle);
    g.setColour (juce::Colour (0xffffb36f));
    g.fillEllipse (indicator.x - 2.2f, indicator.y - 2.2f, 4.4f, 4.4f);

    g.setColour (juce::Colour (0x28ffffff));
    g.drawEllipse (arcBounds.reduced (7.5f), 0.6f);
}

void AuroraD80AudioProcessorEditor::AuroraLookAndFeel::drawComboBox (juce::Graphics& g,
                                                                     int width,
                                                                     int height,
                                                                     bool isButtonDown,
                                                                     int buttonX,
                                                                     int buttonY,
                                                                     int buttonW,
                                                                     int buttonH,
                                                                     juce::ComboBox& box)
{
    juce::ignoreUnused (buttonX, buttonY, buttonW, buttonH, box);

    auto bounds = juce::Rectangle<float> (0.5f, 0.5f, static_cast<float> (width) - 1.0f, static_cast<float> (height) - 1.0f);
    g.setColour (juce::Colour (isButtonDown ? 0xff211a17 : 0xff151312));
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (juce::Colour (0xff4a3f39));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    juce::Path arrow;
    const auto arrowCentreX = static_cast<float> (width - 11);
    const auto arrowCentreY = static_cast<float> (height) * 0.5f + 1.0f;
    arrow.addTriangle (arrowCentreX - 4.0f, arrowCentreY - 2.0f,
                       arrowCentreX + 4.0f, arrowCentreY - 2.0f,
                       arrowCentreX, arrowCentreY + 3.0f);
    g.setColour (juce::Colour (softOrange));
    g.fillPath (arrow);
}

juce::Font AuroraD80AudioProcessorEditor::AuroraLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    juce::ignoreUnused (box);
    return juce::FontOptions (12.0f, juce::Font::bold);
}

void AuroraD80AudioProcessorEditor::AuroraLookAndFeel::drawToggleButton (juce::Graphics& g,
                                                                         juce::ToggleButton& button,
                                                                         bool shouldDrawButtonAsHighlighted,
                                                                         bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat();
    const auto boxSize = juce::jmin (14.0f, bounds.getHeight() - 4.0f);
    const auto box = juce::Rectangle<float> (1.0f, (bounds.getHeight() - boxSize) * 0.5f, boxSize, boxSize);

    g.setColour (juce::Colour (shouldDrawButtonAsDown ? 0xff2a211d : 0xff141211));
    g.fillRoundedRectangle (box, 3.0f);

    g.setColour (juce::Colour (shouldDrawButtonAsHighlighted ? softOrange : panelBorder));
    g.drawRoundedRectangle (box, 3.0f, 1.0f);

    if (button.getToggleState())
    {
        g.setColour (juce::Colour (orange));
        g.fillRoundedRectangle (box.reduced (3.0f), 2.0f);
    }

    g.setColour (juce::Colour (lightGrey));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (button.getButtonText(),
                button.getLocalBounds().withTrimmedLeft (static_cast<int> (boxSize + 6.0f)),
                juce::Justification::centredLeft,
                false);
}

AuroraD80AudioProcessorEditor::PresetArrowButton::PresetArrowButton (const juce::String& arrowText)
    : juce::Button (arrowText), arrow (arrowText)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void AuroraD80AudioProcessorEditor::PresetArrowButton::paintButton (juce::Graphics& g,
                                                                    bool shouldDrawButtonAsHighlighted,
                                                                    bool shouldDrawButtonAsDown)
{
    const auto bounds = getLocalBounds().toFloat().reduced (0.5f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (shouldDrawButtonAsDown ? 0xff15100e : 0xff28231f),
                                             bounds.getCentreX(), bounds.getY(),
                                             juce::Colour (shouldDrawButtonAsDown ? 0xff0e0b0a : 0xff151211),
                                             bounds.getCentreX(), bounds.getBottom(), false));
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (juce::Colour (shouldDrawButtonAsHighlighted ? softOrange : panelBorder));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    g.setColour (juce::Colour (shouldDrawButtonAsDown ? 0xffff7a22 : orange));
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (arrow, getLocalBounds(), juce::Justification::centred, false);
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
    configurePresetBrowser();

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

    startTimerHz (45);
    setSize (800, 420);
}

AuroraD80AudioProcessorEditor::~AuroraD80AudioProcessorEditor()
{
    inputSlider.setLookAndFeel (nullptr);
    timeSlider.setLookAndFeel (nullptr);
    lowCutSlider.setLookAndFeel (nullptr);
    highCutSlider.setLookAndFeel (nullptr);
    depthSlider.setLookAndFeel (nullptr);
    rateSlider.setLookAndFeel (nullptr);
    driveSlider.setLookAndFeel (nullptr);
    vintageSlider.setLookAndFeel (nullptr);
    feedbackSlider.setLookAndFeel (nullptr);
    mixSlider.setLookAndFeel (nullptr);
    outputSlider.setLookAndFeel (nullptr);
    syncButton.setLookAndFeel (nullptr);
    divisionBox.setLookAndFeel (nullptr);
}

void AuroraD80AudioProcessorEditor::configureSlider (juce::Slider& slider,
                                                     juce::Label& nameLabel,
                                                     juce::Label& valueLabel,
                                                     const juce::String& labelText,
                                                     ValueFormat valueFormat)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setLookAndFeel (&lookAndFeel);
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
    valueLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    addAndMakeVisible (valueLabel);
}

void AuroraD80AudioProcessorEditor::configureSyncControls()
{
    syncButton.setButtonText ("SYNC");
    syncButton.setLookAndFeel (&lookAndFeel);
    syncButton.setColour (juce::ToggleButton::textColourId, juce::Colour (lightGrey));
    syncButton.setColour (juce::ToggleButton::tickColourId, juce::Colour (orange));
    syncButton.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff5d3728));
    syncButton.onClick = [this]
    {
        updateTimeValueLabel();
    };
    addAndMakeVisible (syncButton);

    divisionBox.addItemList (juce::StringArray { "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1 Bar" }, 1);
    divisionBox.setLookAndFeel (&lookAndFeel);
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

void AuroraD80AudioProcessorEditor::configurePresetBrowser()
{
    previousPresetButton.onClick = [this]
    {
        showPreviousPreset();
    };

    nextPresetButton.onClick = [this]
    {
        showNextPreset();
    };

    presetNameButton.setButtonText ({});
    presetNameButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    presetNameButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    presetNameButton.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    presetNameButton.setColour (juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
    presetNameButton.setColour (juce::TextButton::textColourOnId, juce::Colours::transparentBlack);
    presetNameButton.onClick = [this]
    {
        juce::PopupMenu menu;

        for (auto index = 0; index < static_cast<int> (presetNames.size()); ++index)
            menu.addItem (index + 1, presetNames[static_cast<size_t> (index)], true, index == currentPresetIndex);

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetNameButton),
                            [this] (int selectedId)
                            {
                                if (selectedId <= 0)
                                    return;

                                currentPresetIndex = selectedId - 1;
                                repaint();
                            });
    };

    addAndMakeVisible (previousPresetButton);
    addAndMakeVisible (nextPresetButton);
    addAndMakeVisible (presetNameButton);
}

void AuroraD80AudioProcessorEditor::showPreviousPreset()
{
    if (presetNames.empty())
        return;

    currentPresetIndex = (currentPresetIndex + static_cast<int> (presetNames.size()) - 1) % static_cast<int> (presetNames.size());
    repaint();
}

void AuroraD80AudioProcessorEditor::showNextPreset()
{
    if (presetNames.empty())
        return;

    currentPresetIndex = (currentPresetIndex + 1) % static_cast<int> (presetNames.size());
    repaint();
}

juce::String AuroraD80AudioProcessorEditor::getCurrentPresetName() const
{
    if (presetNames.empty())
        return {};

    return presetNames[static_cast<size_t> (currentPresetIndex)];
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
        repaint (getRackLayout (getLocalBounds()).display);
        return;
    }

    updateValueLabel (timeSlider, timeValueLabel, ValueFormat::Milliseconds);
    repaint (getRackLayout (getLocalBounds()).display);
}

void AuroraD80AudioProcessorEditor::updateMeterLevels()
{
    auto updateLevel = [] (float current, float target)
    {
        const auto coefficient = target > current ? 0.35f : 0.08f;
        return current + ((target - current) * coefficient);
    };

    for (auto channel = 0; channel < 2; ++channel)
    {
        inputMeterLevels[static_cast<size_t> (channel)] = updateLevel (inputMeterLevels[static_cast<size_t> (channel)],
                                                                       audioProcessor.getInputRmsLevel (channel));
        outputMeterLevels[static_cast<size_t> (channel)] = updateLevel (outputMeterLevels[static_cast<size_t> (channel)],
                                                                        audioProcessor.getOutputRmsLevel (channel));
    }
}

void AuroraD80AudioProcessorEditor::timerCallback()
{
    updateTimeValueLabel();
    updateMeterLevels();

    const auto layout = getRackLayout (getLocalBounds());
    repaint (layout.inputMeter.expanded (2));
    repaint (layout.outputMeter.expanded (2));
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

    auto titleArea = layout.header.withTrimmedBottom (18).toFloat();
    juce::Font titleFont (juce::FontOptions (24.0f, juce::Font::bold));
    juce::GlyphArrangement titleGlyphs;
    titleGlyphs.addFittedText (titleFont,
                               "AURORA D80",
                               titleArea.getX(),
                               titleArea.getY() + 1.0f,
                               titleArea.getWidth(),
                               titleArea.getHeight(),
                               juce::Justification::centred,
                               1);

    juce::Path titlePath;
    titleGlyphs.createPath (titlePath);

    g.setColour (juce::Colour (0xaa000000));
    g.fillPath (titlePath, juce::AffineTransform::translation (0.0f, 2.0f));

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffffff), titleArea.getCentreX(), titleArea.getY() + 4.0f,
                                             juce::Colour (0xff77706a), titleArea.getCentreX(), titleArea.getBottom() - 2.0f,
                                             false));
    g.fillPath (titlePath);

    g.setColour (juce::Colour (0x55ffffff));
    g.strokePath (titlePath, juce::PathStrokeType (0.55f));

    g.setColour (juce::Colour (0x33958d86));
    g.fillPath (titlePath, juce::AffineTransform::translation (0.0f, 0.8f));

    g.setColour (juce::Colour (0x66ffffff));
    g.drawLine (titleArea.getX() + 296.0f,
                titleArea.getY() + 15.0f,
                titleArea.getRight() - 296.0f,
                titleArea.getY() + 15.0f,
                0.7f);

    g.setColour (juce::Colour (lightGrey));
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("Cinematic Digital Delay", layout.header.withTrimmedTop (36), juce::Justification::centred);

    g.setColour (juce::Colour (0xff3c3430));
    g.drawHorizontalLine (layout.header.getBottom(), 0.0f, static_cast<float> (getWidth()));

    const auto presetBrowser = layout.presetBrowser.toFloat();
    g.setColour (juce::Colour (0xff050404).withAlpha (0.45f));
    g.fillRoundedRectangle (presetBrowser.translated (0.0f, 1.0f), 5.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff28231f), presetBrowser.getCentreX(), presetBrowser.getY(),
                                             juce::Colour (0xff141110), presetBrowser.getCentreX(), presetBrowser.getBottom(), false));
    g.fillRoundedRectangle (presetBrowser, 5.0f);
    g.setColour (juce::Colour (0xff3e352f));
    g.drawRoundedRectangle (presetBrowser, 5.0f, 1.0f);
    g.setColour (juce::Colour (0x28ff6a1a));
    g.drawRoundedRectangle (presetBrowser.reduced (2.0f), 3.5f, 0.8f);

    auto presetNameArea = layout.presetBrowser.reduced (42, 1);
    g.setColour (juce::Colour (orange));
    g.setFont (juce::FontOptions (13.5f, juce::Font::bold));
    g.drawText (getCurrentPresetName(), presetNameArea, juce::Justification::centred, false);

    const auto display = layout.display.toFloat();
    const auto syncIsEnabled = syncButton.getToggleState();
    auto displayValue = syncIsEnabled ? divisionBox.getText()
                                      : juce::String (juce::roundToInt (timeSlider.getValue())) + " ms";

    if (displayValue.isEmpty())
        displayValue = "1/4";

    const auto displayMode = syncIsEnabled ? juce::String ("SYNC") : juce::String ("MANUAL");

    g.setColour (juce::Colour (0xff050404).withAlpha (0.55f));
    g.fillRoundedRectangle (display.translated (0.0f, 2.0f), 8.0f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff383330), display.getCentreX(), display.getY(),
                                             juce::Colour (0xff11100f), display.getCentreX(), display.getBottom(), false));
    g.fillRoundedRectangle (display, 8.0f);

    g.setColour (juce::Colour (0xff080707));
    g.drawRoundedRectangle (display, 8.0f, 2.0f);

    g.setColour (juce::Colour (0x33ff6a1a));
    g.drawRoundedRectangle (display.reduced (2.0f), 6.5f, 1.0f);

    const auto screen = display.reduced (8.0f, 7.0f);
    g.setColour (juce::Colour (0xff020202));
    g.fillRoundedRectangle (screen, 4.5f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff080605), screen.getCentreX(), screen.getY(),
                                             juce::Colour (0xff000000), screen.getCentreX(), screen.getBottom(), false));
    g.fillRoundedRectangle (screen.reduced (1.0f), 4.0f);

    g.setColour (juce::Colour (0x18ff4a18));
    g.fillRoundedRectangle (screen.reduced (4.0f, 3.0f), 3.0f);

    g.setColour (juce::Colour (0xff020202));
    g.fillRoundedRectangle (screen.reduced (6.0f, 5.0f), 2.5f);

    g.setColour (juce::Colour (0xff1a1513));
    g.drawRoundedRectangle (screen, 4.5f, 1.0f);
    g.setColour (juce::Colour (0x28ff7a22));
    g.drawRoundedRectangle (screen.reduced (1.5f), 3.5f, 0.8f);

    auto displayTextArea = screen.toNearestInt().reduced (18, 5);
    auto modeArea = displayTextArea.removeFromTop (9);
    displayTextArea.removeFromTop (4);

    g.setColour (juce::Colour (0xff918984));
    g.setFont (juce::FontOptions (7.5f, juce::Font::bold));
    g.drawText (displayMode, modeArea, juce::Justification::centred);

    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.setColour (juce::Colour (0x22ff3a12));
    g.drawText (displayValue, displayTextArea.translated (0, 2), juce::Justification::centred);
    g.setColour (juce::Colour (0x55ff4f18));
    g.drawText (displayValue, displayTextArea.translated (0, 1), juce::Justification::centred);
    g.setColour (juce::Colour (0xffff7a22));
    g.drawText (displayValue, displayTextArea, juce::Justification::centred);

    drawPanel (g, layout.input, "INPUT");
    drawPanel (g, layout.delay, "DELAY");
    drawPanel (g, layout.tone, "TONE");
    drawPanel (g, layout.modulation, "MODULATION");
    drawPanel (g, layout.character, "CHARACTER");
    drawPanel (g, layout.output, "OUTPUT");

    drawStereoMeter (g, layout.inputMeter, "INPUT", inputMeterLevels);
    drawStereoMeter (g, layout.outputMeter, "OUTPUT", outputMeterLevels);
}

void AuroraD80AudioProcessorEditor::resized()
{
    const auto layout = getRackLayout (getLocalBounds());

    auto presetBrowser = layout.presetBrowser.reduced (3, 3);
    previousPresetButton.setBounds (presetBrowser.removeFromLeft (28));
    nextPresetButton.setBounds (presetBrowser.removeFromRight (28));
    presetNameButton.setBounds (presetBrowser);

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
