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
    configureSlider (inputSlider, inputLabel, "INPUT");
    configureSlider (timeSlider, timeLabel, "TIME");
    configureSlider (feedbackSlider, feedbackLabel, "FEEDBACK");
    configureSlider (mixSlider, mixLabel, "MIX");
    configureSlider (outputSlider, outputLabel, "OUTPUT");

    inputAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "inputGain", inputSlider);
    timeAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "delayTimeMs", timeSlider);
    feedbackAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "feedback", feedbackSlider);
    mixAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "mix", mixSlider);
    outputAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "outputGain", outputSlider);

    setSize (800, 420);
}

AuroraD80AudioProcessorEditor::~AuroraD80AudioProcessorEditor()
{
}

void AuroraD80AudioProcessorEditor::configureSlider (juce::Slider& slider,
                                                     juce::Label& label,
                                                     const juce::String& labelText)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 88, 24);
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffff6a1a));
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff3a2520));
    slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffffb36f));
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffffdcc6));
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff17110f));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff3f241b));
    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colour (0xffff8a3d));
    label.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    addAndMakeVisible (label);
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
    g.drawRoundedRectangle (bounds.reduced (18.0f), 10.0f, 1.5f);

    g.setColour (juce::Colour (0xffff6a1a));
    g.setFont (juce::FontOptions (34.0f, juce::Font::bold));
    g.drawText ("AURORA D80", 0, 36, getWidth(), 42, juce::Justification::centred);

    g.setColour (juce::Colour (0xffffb36f));
    g.setFont (juce::FontOptions (16.0f));
    g.drawText ("Cinematic Digital Delay", 0, 78, getWidth(), 28, juce::Justification::centred);
}

void AuroraD80AudioProcessorEditor::resized()
{
    auto controlsArea = getLocalBounds().withTrimmedTop (142).reduced (44, 28);
    const auto controlWidth = controlsArea.getWidth() / 5;

    juce::Slider* sliders[] = { &inputSlider, &timeSlider, &feedbackSlider, &mixSlider, &outputSlider };
    juce::Label* labels[] = { &inputLabel, &timeLabel, &feedbackLabel, &mixLabel, &outputLabel };

    for (auto index = 0; index < 5; ++index)
    {
        auto controlBounds = controlsArea.removeFromLeft (controlWidth).reduced (12, 0);
        labels[index]->setBounds (controlBounds.removeFromTop (26));
        sliders[index]->setBounds (controlBounds);
    }
}
