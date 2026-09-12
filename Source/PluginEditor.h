#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class AuroraD80AudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit AuroraD80AudioProcessorEditor (AuroraD80AudioProcessor&);
    ~AuroraD80AudioProcessorEditor() override;

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
        void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
        void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override;
        juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
        juce::Font getPopupMenuFont() override;
        void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
        void drawPopupMenuBackgroundWithOptions (juce::Graphics&, int width, int height,
                                                 const juce::PopupMenu::Options&) override;
        void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area,
                                bool isSeparator, bool isActive, bool isHighlighted,
                                bool isTicked, bool hasSubMenu, const juce::String& text,
                                const juce::String& shortcutKeyText,
                                const juce::Drawable* icon, const juce::Colour* textColour) override;
        void getIdealPopupMenuItemSize (const juce::String&, bool isSeparator,
                                        int standardMenuItemHeight, int& idealWidth, int& idealHeight) override;
        int getPopupMenuBorderSize() override;
        int getPopupMenuBorderSizeWithOptions (const juce::PopupMenu::Options&) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;

    private:
        juce::Image knobBody;
        std::unique_ptr<juce::Drawable> knobTicks, knobShadow, knobBezel;
        std::unique_ptr<juce::Drawable> knobPointerGlow, knobPointer, knobPointerHighlight;
    };

    class DesignSurface final : public juce::Component
    {
    public:
        explicit DesignSurface (AuroraD80AudioProcessorEditor& o) : owner (o) { setOpaque (true); }
        void paint (juce::Graphics& g) override { owner.paintDesign (g); }
        void mouseDown (const juce::MouseEvent& e) override;
    private:
        AuroraD80AudioProcessorEditor& owner;
    };

    enum class ValueFormat { GainDb, Percent01, Percent100, Milliseconds, Hz, KHz, RateHz };

    void paintDesign (juce::Graphics&);
    void configureSlider (juce::Slider&, juce::Label&, juce::Label&, const juce::String&, ValueFormat);
    void configureChoice (juce::ComboBox&, const juce::StringArray&);
    void configureButton (juce::TextButton&, const juce::String&, bool toggle = true);
    void configurePresetBrowser();
    void updateValueLabel (juce::Slider&, juce::Label&, ValueFormat);
    void updateDynamicText();
    void syncPresetIdentity();
    void updateMeters();
    void timerCallback() override;
    void applyPreset (int index);
    void showFactoryPresetMenu();
    void showMainMenu();
    void showPresetBrowser();
    void showSavePresetBrowser();
    void showSettingsPanel();
    void showFeedbackPanel();
    void showChoiceMenu (juce::ComboBox&, juce::Point<int>, const juce::StringArray&);
    void setEditorScale (float scale);
    void closeOverlay();
    void randomiseMusicalParameters();
    void saveSnapshotToSlot (int slot);
    void recallSnapshotFromSlot (int slot);

    AuroraD80AudioProcessor& audioProcessor;
    AuroraLookAndFeel lookAndFeel;
    DesignSurface designSurface;

    // Core controls
    juce::Slider inputSlider, outputSlider, timeSlider, feedbackSlider, mixSlider;
    juce::Slider lowCutSlider, highCutSlider, widthSlider;
    juce::Slider depthSlider, rateSlider, driftSlider;
    juce::Slider driveSlider, evolveSlider, bloomSlider;

    juce::Label inputLabel, outputLabel, timeLabel, feedbackLabel, mixLabel;
    juce::Label lowCutLabel, highCutLabel, widthLabel;
    juce::Label depthLabel, rateLabel, driftLabel;
    juce::Label driveLabel, evolveLabel, bloomLabel;

    juce::Label inputValue, outputValue, timeValue, feedbackValue, mixValue;
    juce::Label lowCutValue, highCutValue, widthValue;
    juce::Label depthValue, rateValue, driftValue;
    juce::Label driveValue, evolveValue, bloomValue;

    juce::TextButton syncButton, pingPongButton;
    juce::ComboBox divisionBox, shapeBox;

    // Utility bar
    juce::ComboBox modeBox, oversamplingBox;
    juce::Slider qualitySlider;
    juce::TextButton modeCycleButton, oversamplingCycleButton;
    juce::TextButton freezeButton, bypassButton;

    // Preset / workflow bar
    juce::TextButton previousPresetButton { "‹" }, nextPresetButton { "›" };
    juce::TextButton loadButton { "LOAD" }, saveAsButton { "SAVE AS" };
    juce::TextButton aButton { "A" }, bButton { "B" }, undoRedoButton { "UNDO/REDO" };
    juce::TextButton diceButton { "⚄" }, menuButton { "☰" };
    juce::TextButton presetDisplayButton;
    juce::TextButton favouriteButton;
    juce::Label presetNumberLabel, presetNameLabel;
    std::unique_ptr<juce::Component> activeOverlay;
    std::vector<juce::String> presetNames;
    int currentPresetIndex = 0;
    bool presetIdentityValid = false;
    juce::ValueTree snapshotA { "Snapshot" }, snapshotB { "Snapshot" };
    juce::ValueTree undoState { "Snapshot" };

    std::array<float, 2> inputMeterLevels { 0.0f, 0.0f };
    std::array<float, 2> outputMeterLevels { 0.0f, 0.0f };
    float waveformPhase = 0.0f;
   #if JUCE_DEBUG
    bool debugSnapshotWritten = false;
   #endif

    // Figma production artwork (embedded by Projucer after FETCH_FIGMA_ASSETS.command).
    juce::Image chassisImage, headerImage, displayImage, inputMeterImage, outputMeterImage;
    juce::Image delayCoreImage, toneStereoImage, modCharacterImage, utilityBarImage, legacyUtilityBarImage, presetBarImage;
    std::unique_ptr<juce::Drawable> headerBrand, freezeIcon, bypassIcon;
    juce::Typeface::Ptr breeSerifTypeface, arimoTypeface;
    int activeSnapshotSlot = 0;
    float editorScale = 0.75f;
    bool graphicsAccelerationEnabled = false;
    bool hqOversamplingEnabled = false;

    // Attachments
    std::unique_ptr<SliderAttachment> inputAttachment, outputAttachment, timeAttachment, feedbackAttachment, mixAttachment;
    std::unique_ptr<SliderAttachment> lowCutAttachment, highCutAttachment, widthAttachment;
    std::unique_ptr<SliderAttachment> depthAttachment, rateAttachment, driftAttachment;
    std::unique_ptr<SliderAttachment> driveAttachment, evolveAttachment, bloomAttachment;
    std::unique_ptr<ButtonAttachment> syncAttachment, pingPongAttachment, freezeAttachment, bypassAttachment;
    std::unique_ptr<ComboBoxAttachment> divisionAttachment, shapeAttachment, modeAttachment, oversamplingAttachment;
    std::unique_ptr<SliderAttachment> qualityAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuroraD80AudioProcessorEditor)
};
