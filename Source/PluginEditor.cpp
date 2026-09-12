#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr auto bg0 = 0xff090909;
constexpr auto border = 0xff514941;
constexpr auto orange = 0xffff6a1a;
constexpr auto orangeHi = 0xffffa05c;
constexpr auto text = 0xffd0c8c1;
constexpr auto white = 0xfffff3e9;
constexpr float defaultMasterScale = 0.75f;
constexpr float masterCentreCorrectionX = 0.0f;
constexpr float productionKnobW = 93.73685f;
constexpr float productionKnobH = 117.0f;
constexpr float productionKnobVisualCentreX = 0.4780f;

struct Layout
{
    // Root-space coordinates from Figma production candidate v1.4
    // (node 6871:30249). The main rebuild is at (+1,-1); the Utility Bar is a
    // direct child of the root. These are logical node bounds: drawFigmaExport
    // trims each PNG's symmetric shadow/effect pixels automatically.
    juce::Rectangle<int> chassis { 1, -1, 1536, 1024 };
    juce::Rectangle<int> header { 207, 17, 1125, 84 };
    juce::Rectangle<int> inputMeter { 33, 98, 174, 685 };
    juce::Rectangle<int> outputMeter { 1331, 98, 174, 685 };
    juce::Rectangle<int> display { 407, 98, 725, 222 };
    juce::Rectangle<int> delay { 221, 334, 547, 221 };
    juce::Rectangle<int> tone { 780, 334, 537, 221 };
    juce::Rectangle<int> modulationCharacter { 221, 576, 1096, 207 };
    juce::Rectangle<int> utility { 33, 798, 1472, 80 };
    juce::Rectangle<int> presets { 33, 891, 1472, 70 };
};

struct FactoryPreset
{
    const char* category;
    const char* name;
    float input, output, time, feedback, mix, low, high, width;
    float depth, rate, drift, drive, evolve, bloom;
    int division, shape, mode, quality, oversampling;
    bool sync, pingPong;
};

const std::array<FactoryPreset, 30>& getFactoryPresets()
{
    static const std::array<FactoryPreset, 30> presets {{
        { "Cinematic", "Deep Horizon",       0.0f,-1.5f, 578, .35f,.35f,120,19000,131,15,.35f,12,10,52,25, 4,0,0,1,1, true,true },
        { "Cinematic", "Monolith Arrival",   -1.0f,-2.0f, 920, .58f,.42f, 80,11200,156,10,.16f,18,19,67,46, 5,0,1,1,2, true,true },
        { "Cinematic", "Orbital Ruins",      -2.0f,-1.0f,1280, .66f,.38f, 55, 9200,172,24,.10f,30,12,74,58, 6,1,2,2,2, true,true },
        { "Cinematic", "Distant Signal",     -1.5f,-2.5f, 740, .47f,.31f,180,15600,145,32,.28f,21, 8,61,34, 4,2,0,1,1, true,true },
        { "Cinematic", "Final Transmission", 0.5f,-3.0f,1640, .78f,.48f, 45, 7600,188,18,.12f,42,25,83,70, 6,0,1,2,3, true,true },

        { "Ambient", "Infinite Sky",         -1.0f,-2.0f, 980, .70f,.46f, 70,18000,170,18,.12f,15, 8,45,64, 5,0,0,2,1, true,true },
        { "Ambient", "Glass Tides",          -2.0f,-1.0f, 660, .52f,.40f,220,20000,158,38,.22f,10, 3,58,55, 4,0,0,2,2, true,true },
        { "Ambient", "Cloud Chamber",        -1.5f,-2.0f,1420, .74f,.52f, 40,13000,182,22,.10f,34,11,79,73, 6,1,1,1,2, true,true },
        { "Ambient", "Soft Constellations",   0.0f,-1.5f, 840, .44f,.37f,110,17500,150,28,.18f, 8, 2,49,42, 5,0,0,2,1, true,false },
        { "Ambient", "Frozen Echoes",        -3.0f,-2.5f,1180, .82f,.55f, 30, 9800,194,12,.10f,26, 6,88,81, 6,2,1,2,3, true,true },

        { "Rhythmic", "Neon Rain",           -1.0f,-1.5f, 360, .48f,.28f,180,14000,145,28,.55f,18,22,35,18, 4,0,0,1,1, true,true },
        { "Rhythmic", "Quarter Pulse",        0.0f,-1.0f, 500, .42f,.32f,120,17000,138,12,1.20f, 6,10,28,20, 4,0,0,1,2, true,false },
        { "Rhythmic", "Dotted Machines",     -1.5f,-1.5f, 375, .56f,.34f,260,12000,162,34,.72f,25,18,54,32, 3,1,2,1,2, true,true },
        { "Rhythmic", "Circuit Bounce",       0.5f,-2.0f, 250, .38f,.27f,320,15500,128,45,2.10f,14,31,26,15, 3,2,0,0,1, true,true },
        { "Rhythmic", "Polyrhythm Alley",    -2.0f,-1.0f, 750, .64f,.36f, 95,10800,176,55,.43f,36,21,62,48, 5,1,1,1,2, true,true },

        { "Tape & Analog", "Midnight Bloom", -1.0f,-2.0f, 460, .44f,.34f,150,16000,138,22,.42f,20,16,55,55, 4,0,1,1,1, true,true },
        { "Tape & Analog", "Velvet Reel",     1.0f,-3.0f, 620, .53f,.41f, 90, 8200,142,16,.26f,42,38,71,44, 4,0,1,0,1, true,true },
        { "Tape & Analog", "Oxide Memory",   -0.5f,-2.0f, 810, .61f,.39f, 65, 6900,134,20,.19f,58,46,82,52, 5,2,1,0,1, true,false },
        { "Tape & Analog", "Warm Satellite",  0.0f,-2.5f,1080, .68f,.47f, 50,10500,165,27,.13f,33,29,76,66, 5,0,2,1,2, true,true },
        { "Tape & Analog", "Cassette Mirage", 1.5f,-3.5f, 430, .72f,.43f,240, 5400,152,48,.64f,71,55,64,39, 3,2,1,0,0, true,true },

        { "Modulated", "Ghost Signal",       -1.0f,-1.5f, 690, .56f,.30f,210,12500,160,35,.28f,32,25,72,35, 4,2,2,1,2, true,true },
        { "Modulated", "Chorus Nebula",      -2.0f,-2.0f, 540, .49f,.38f,140,16500,185,62,.38f,18, 8,57,49, 4,0,0,2,2, true,false },
        { "Modulated", "Random Orbit",       -1.5f,-2.5f, 880, .63f,.44f, 75,11800,174,78,.21f,55,17,69,61, 5,2,2,1,3, true,true },
        { "Modulated", "Triangle Drift",      0.0f,-1.5f, 320, .40f,.33f,300,18200,148,52,1.35f,27,12,43,28, 3,1,0,2,1, true,true },
        { "Modulated", "Aurora Swell",       -2.5f,-1.0f,1320, .77f,.50f, 35,14500,196,46,.10f,64,23,91,79, 6,0,1,2,3, true,true },

        { "Experimental", "Event Horizon",   -3.0f,-3.0f,2000, .92f,.58f, 20, 4200,200,83,.10f,88,62,94,92, 6,2,2,0,3, false,true },
        { "Experimental", "Broken Telemetry", 2.0f,-4.0f, 147, .84f,.46f,600, 3600,120,91,4.70f,95,73,38,24, 1,2,0,0,0, false,true },
        { "Experimental", "Reverse Geometry",-1.0f,-2.0f,1111, .69f,.53f, 25,20000, 72,67,.11f,76,44,86,88, 6,1,2,2,3, true,false },
        { "Experimental", "Digital Ash",      1.0f,-3.5f, 290, .76f,.49f,900, 2800,112,42,7.20f,81,90,20,12, 2,2,0,0,0, false,true },
        { "Experimental", "Prism Collapse",  -2.0f,-4.0f,1777, .90f,.60f, 20, 6200,200,96,.10f,99,68,97,100, 6,2,2,0,3, true,true }
    }};
    return presets;
}

void installFactoryPresetLibrary()
{
    auto root = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                    .getChildFile ("Library/Audio/Presets/Prismatica Audio Labs/AuroraD80/Factory");
    const auto& presets = getFactoryPresets();
    for (int i = 0; i < (int) presets.size(); ++i)
    {
        const auto& p = presets[(size_t) i];
        auto folder = root.getChildFile (p.category);
        folder.createDirectory();

        juce::DynamicObject::Ptr data = new juce::DynamicObject();
        data->setProperty ("format", "AuroraD80 Factory Preset v1");
        data->setProperty ("index", i + 1);
        data->setProperty ("category", p.category);
        data->setProperty ("name", p.name);
        data->setProperty ("inputGain", p.input); data->setProperty ("outputGain", p.output);
        data->setProperty ("delayTimeMs", p.time); data->setProperty ("feedback", p.feedback);
        data->setProperty ("mix", p.mix); data->setProperty ("lowCutHz", p.low);
        data->setProperty ("highCutHz", p.high); data->setProperty ("width", p.width);
        data->setProperty ("modulationDepth", p.depth); data->setProperty ("modulationRate", p.rate);
        data->setProperty ("drift", p.drift); data->setProperty ("drive", p.drive);
        data->setProperty ("evolve", p.evolve); data->setProperty ("bloom", p.bloom);
        data->setProperty ("syncDivision", p.division); data->setProperty ("modShape", p.shape);
        data->setProperty ("mode", p.mode); data->setProperty ("quality", p.quality);
        data->setProperty ("oversampling", p.oversampling); data->setProperty ("syncEnabled", p.sync);
        data->setProperty ("pingPong", p.pingPong);

        auto safeName = juce::File::createLegalFileName (juce::String (i + 1).paddedLeft ('0', 2)
                                                         + " " + p.name + ".aurora");
        folder.getChildFile (safeName).replaceWithText (juce::JSON::toString (juce::var (data.get()), true));
    }
}

juce::Image removeBlackMatte (const juce::Image& source)
{
    if (! source.isValid())
        return {};

    juce::Image result (juce::Image::ARGB, source.getWidth(), source.getHeight(), true);
    juce::Image::BitmapData src (source, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dst (result, juce::Image::BitmapData::writeOnly);

    for (int y = 0; y < source.getHeight(); ++y)
    {
        for (int x = 0; x < source.getWidth(); ++x)
        {
            const auto colour = src.getPixelColour (x, y);
            const auto matteAlpha = juce::jmax (colour.getRed(), colour.getGreen(), colour.getBlue());

            if (matteAlpha <= 3)
            {
                dst.setPixelColour (x, y, juce::Colours::transparentBlack);
                continue;
            }

            const auto unmatte = [matteAlpha] (juce::uint8 channel)
            {
                return (juce::uint8) juce::jlimit (0, 255,
                                                   juce::roundToInt ((float) channel * 255.0f / (float) matteAlpha));
            };

            dst.setPixelColour (x, y, juce::Colour (unmatte (colour.getRed()),
                                                     unmatte (colour.getGreen()),
                                                     unmatte (colour.getBlue()),
                                                     matteAlpha));
        }
    }

    return result;
}

juce::Image makeNoSignalMeter (const juce::Image& source)
{
    if (! source.isValid())
        return {};

    auto result = source.createCopy();
    juce::Graphics g (result);

    // The 216x727 export includes a symmetric 21 px horizontal effect bound,
    // but the 0,+10 drop shadow makes the vertical bound asymmetric: 11 px
    // above and 31 px below. Using 21 px vertically moved the metal body by
    // 10 px relative to the reconstructed glass and LEDs.
    constexpr float contentX = 21.0f;
    constexpr float contentY = 11.0f;
    const auto chamber = juce::Rectangle<float> (contentX + 47.8f, contentY + 76.8f, 76.0f, 520.0f);

    g.setColour (juce::Colour (0xff050607));
    g.fillRoundedRectangle (chamber, 5.0f);

    constexpr int segments = 20;
    for (int channel = 0; channel < 2; ++channel)
    {
        const float x = contentX + (channel == 0 ? 60.8f : 99.8f);
        for (int segment = 0; segment < segments; ++segment)
        {
            const float y = contentY + 86.8f + segment * 25.0f;
            const auto colour = segment < 3 ? juce::Colour (0xffe1221a)
                              : segment < 7 ? juce::Colour (0xfff28c18)
                                            : juce::Colour (0xff39b51c);
            const auto cell = juce::Rectangle<float> (x, y, 14.0f, 17.0f);
            g.setColour (colour.withAlpha (0.055f));
            g.fillRoundedRectangle (cell, 2.5f);
            g.setColour (juce::Colour (0xe6030304));
            g.drawRoundedRectangle (cell, 2.5f, 0.8f);
        }
    }

    juce::ColourGradient glass (juce::Colour (0x15ffd69e), chamber.getX(), chamber.getCentreY(),
                                juce::Colour (0x23000000), chamber.getRight(), chamber.getCentreY(), false);
    glass.addColour (0.18, juce::Colour (0x04ffffff));
    glass.addColour (0.72, juce::Colour (0x05000000));
    g.setGradientFill (glass);
    g.fillRoundedRectangle (chamber, 5.0f);
    g.setColour (juce::Colour (0x996f777e));
    g.drawRoundedRectangle (chamber, 5.0f, 0.6f);

    return result;
}

class AuroraFloatingPanel final : public juce::Component
{
public:
    enum class Kind { Presets, SavePreset, Settings, Feedback };
    AuroraFloatingPanel (Kind k, std::function<void(int)> selectPreset,
                         std::function<void()> copyAB, std::function<void()> close,
                         std::function<void(const juce::String&)> savePreset = nullptr,
                         std::function<void(int)> action = nullptr,
                         std::function<void(float)> scaleChanged = nullptr,
                         float currentScale = 0.75f)
        : kind (k), onPreset (std::move (selectPreset)), onCopyAB (std::move (copyAB)),
          onClose (std::move (close)), onSavePreset (std::move (savePreset)), onAction (std::move (action)),
          onScaleChanged (std::move (scaleChanged)), selectedScale (currentScale)
    {
        setWantsKeyboardFocus (true);
        setSize (kind == Kind::Settings ? 390 : (kind == Kind::Feedback ? 560 : 480), kind == Kind::Settings ? 650 : (kind == Kind::Feedback ? 620 : (kind == Kind::Presets ? 315 : 190)));
        if (kind == Kind::SavePreset)
        {
            presetName.setText ("My Aurora Preset", false);
            presetName.setSelectAllWhenFocused (true);
            presetName.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff090a09));
            presetName.setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff4d4d4a));
            presetName.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (orange));
            presetName.setColour (juce::TextEditor::textColourId, juce::Colour (white));
            presetName.setFont (juce::FontOptions ("Roboto Condensed", 14.0f, juce::Font::plain));
            addAndMakeVisible (presetName);
            presetName.setBounds (18, 58, 444, 36);
        }
        if (kind == Kind::Feedback)
        {
            summary.setTextToShowWhenEmpty ("Summary (required)", juce::Colour (0xff868782));
            details.setTextToShowWhenEmpty ("Describe your feedback, bug, or feature request.", juce::Colour (0xff868782));
            name.setTextToShowWhenEmpty ("Name", juce::Colour (0xff868782));
            email.setTextToShowWhenEmpty ("Email", juce::Colour (0xff868782));
            for (auto* editor : { &summary, &details, &name, &email }) { editor->setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff10110f)); editor->setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff555852)); editor->setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (orange)); editor->setColour (juce::TextEditor::textColourId, juce::Colour (white)); addAndMakeVisible (*editor); }
            summary.setBounds (26, 128, 508, 40); details.setBounds (26, 182, 508, 150); details.setMultiLine (true); details.setReturnKeyStartsNewLine (true);
            name.setBounds (26, 475, 246, 40); email.setBounds (288, 475, 246, 40);
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setGradientFill ({ juce::Colour (0xff1a1b19), r.getCentreX(), r.getY(), juce::Colour (0xff080908), r.getCentreX(), r.getBottom(), false });
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (juce::Colour (0xff4d4d4a)); g.drawRoundedRectangle (r, 8.0f, 1.0f);
        g.setFont (juce::FontOptions ("Roboto Condensed", 13.0f, juce::Font::plain));
        g.setColour (juce::Colour (white));

        if (kind == Kind::Settings)
        {
            g.setColour (juce::Colour (0xff0a0b0a));
            g.fillRoundedRectangle ({ 10.0f, 10.0f, 370.0f, 630.0f }, 9.0f);
            g.setColour (juce::Colour (0xff555852));
            g.drawRoundedRectangle ({ 10.0f, 10.0f, 370.0f, 630.0f }, 9.0f, 1.0f);
            g.setColour (juce::Colour (white));
            g.setFont (juce::FontOptions ("Roboto Condensed", 20.0f, juce::Font::bold));
            g.drawText ("SYSTEM SETTINGS", 24, 25, 210, 30, juce::Justification::centredLeft);
            g.setColour (juce::Colour (0xffb9bbb5));
            g.drawLine (354.0f, 30.0f, 366.0f, 42.0f, 1.6f);
            g.drawLine (366.0f, 30.0f, 354.0f, 42.0f, 1.6f);
            g.setFont (juce::FontOptions ("Roboto Condensed", 11.0f, juce::Font::plain));
            g.setColour (juce::Colour (orange));
            g.drawText ("AURORA D80 CORE", 24, 54, 160, 17, juce::Justification::centredLeft);
            g.setColour (juce::Colour (0xff38d36b)); g.fillEllipse (294.0f, 56.0f, 7.0f, 7.0f);
            g.setColour (juce::Colour (0xffa6a7a1)); g.drawText ("ONLINE", 306, 52, 52, 17, juce::Justification::centredLeft);
            g.setColour (juce::Colour (0xff3a3c38)); g.drawHorizontalLine (78, 24, 376);
            g.setColour (juce::Colour (0xff989a94));
            g.drawText ("INTERFACE GRAPHICS SCALE", 24, 89, 250, 18, juce::Justification::centredLeft);
            const juce::StringArray scales { "75%", "100%", "125%", "150%", "200%" };
            for (int i = 0; i < scales.size(); ++i)
            {
                auto b = juce::Rectangle<float> (24.0f + i * 70.0f, 114.0f, 61.0f, 34.0f);
                const bool selected = std::abs (selectedScale - (0.75f + i * 0.25f)) < 0.01f;
                g.setColour (selected ? juce::Colour (0xff2d170b) : juce::Colour (0xff111210)); g.fillRoundedRectangle (b, 4.0f);
                g.setColour (selected ? juce::Colour (orange) : juce::Colour (0xff4d4d4a)); g.drawRoundedRectangle (b, 4.0f, selected ? 1.5f : 1.0f);
                g.setColour (selected ? juce::Colour (orange) : juce::Colour (text)); g.drawText (scales[i], b.toNearestInt(), juce::Justification::centred);
            }
            g.setColour (juce::Colour (0xff3a3c38)); g.drawHorizontalLine (165, 24, 376);
            g.setColour (juce::Colour (0xff989a94)); g.drawText ("SESSION & COMPARISON", 24, 177, 250, 18, juce::Justification::centredLeft);
            g.setColour (juce::Colour (white)); g.drawText ("Active Slot State", 24, 203, 170, 23, juce::Justification::centredLeft);
            g.setColour (juce::Colour (0xff151613)); g.fillRoundedRectangle ({ 294.0f, 199.0f, 66.0f, 31.0f }, 4.0f);
            g.setColour (selectedSlot == 0 ? juce::Colour (orange) : juce::Colour (0xffa6a7a1)); g.drawText ("A", 302, 199, 25, 31, juce::Justification::centred);
            g.setColour (selectedSlot == 1 ? juce::Colour (orange) : juce::Colour (0xffa6a7a1)); g.drawText ("B", 329, 199, 25, 31, juce::Justification::centred);
            auto copy = juce::Rectangle<float> (24, 240, 336, 38);
            g.setColour (juce::Colour (0xff111210)); g.fillRoundedRectangle (copy, 5.0f);
            g.setColour (hoverRow == 10 ? juce::Colour (orange) : juce::Colour (0xff4d4d4a)); g.drawRoundedRectangle (copy, 5.0f, 1.0f);
            g.setColour (hoverRow == 10 ? juce::Colour (orange) : juce::Colour (text)); g.drawText ("COPY SESSION A TO B", copy.reduced (13, 0).toNearestInt(), juce::Justification::centredLeft);
            g.setColour (juce::Colour (0xff3a3c38)); g.drawHorizontalLine (295, 24, 376);
            g.setColour (juce::Colour (0xff989a94)); g.drawText ("ENGINE OPTIMIZATION", 24, 307, 250, 18, juce::Justification::centredLeft);
            g.setColour (juce::Colour (white)); g.drawText ("Graphics Acceleration (OpenGL)", 24, 334, 250, 22, juce::Justification::centredLeft);
            g.setColour (graphicsEnabled ? juce::Colour (orange) : juce::Colour (0xff343632)); g.fillRoundedRectangle ({ 330.0f, 336.0f, 30.0f, 17.0f }, 9.0f); g.setColour (graphicsEnabled ? juce::Colour (white) : juce::Colour (0xff8a8c86)); g.fillEllipse (graphicsEnabled ? 345.0f : 332.0f, 338.0f, 13.0f, 13.0f);
            g.setColour (juce::Colour (0xff989a94)); g.drawText ("HQ Oversampling on Render", 24, 362, 260, 22, juce::Justification::centredLeft);
            g.setColour (hqEnabled ? juce::Colour (orange) : juce::Colour (0xff343632)); g.fillRoundedRectangle ({ 330.0f, 364.0f, 30.0f, 17.0f }, 9.0f); g.setColour (hqEnabled ? juce::Colour (white) : juce::Colour (0xff8a8c86)); g.fillEllipse (hqEnabled ? 345.0f : 332.0f, 366.0f, 13.0f, 13.0f);
            g.setColour (juce::Colour (0xff3a3c38)); g.drawHorizontalLine (400, 24, 376);
            g.setColour (juce::Colour (0xff989a94)); g.drawText ("QUICK ACTIONS", 24, 412, 250, 18, juce::Justification::centredLeft);
            const juce::StringArray actions { "UPDATE", "GO TO WEBSITE", "TUTORIALS", "MANUAL", "SEND FEEDBACK" };
            for (int i = 0; i < actions.size(); ++i)
            {
                auto row = juce::Rectangle<float> (24.0f, 438.0f + i * 38.0f, 336.0f, 32.0f);
                const bool highlighted = hoverRow == 30 + i;
                g.setColour (highlighted ? juce::Colour (0xff241306) : juce::Colour (0xff111210)); g.fillRoundedRectangle (row, 4.0f);
                g.setColour (highlighted ? juce::Colour (orange) : juce::Colour (0xff4d4d4a)); g.drawRoundedRectangle (row, 4.0f, 1.0f);
                g.setColour (highlighted ? juce::Colour (orangeHi) : juce::Colour (text)); g.drawText (actions[i], row.reduced (12, 0).toNearestInt(), juce::Justification::centredLeft);
            }
            return;
        }

        if (kind == Kind::Feedback)
        {
            g.setColour (juce::Colour (0xff090a09)); g.fillRoundedRectangle ({ 10.0f, 10.0f, 540.0f, 600.0f }, 10.0f);
            g.setColour (juce::Colour (0xff555852)); g.drawRoundedRectangle ({ 10.0f, 10.0f, 540.0f, 600.0f }, 10.0f, 1.0f);
            g.setColour (juce::Colour (white)); g.setFont (juce::FontOptions ("Roboto Condensed", 22.0f, juce::Font::bold)); g.drawText ("SEND FEEDBACK", 26, 28, 270, 30, juce::Justification::centredLeft);
            g.setColour (juce::Colour (orange)); g.setFont (juce::FontOptions ("Roboto Condensed", 13.0f, juce::Font::plain)); g.drawText ("AURORA D80 / PRISMATICA AUDIO LABS", 26, 61, 360, 18, juce::Justification::centredLeft);
            g.setColour (juce::Colour (0xffa6a7a1)); g.drawText ("Your feedback is reviewed by our support team.", 26, 88, 430, 20, juce::Justification::centredLeft);
            auto attachment = juce::Rectangle<float> (26, 348, 508, 104);
            g.setColour (juce::Colour (0xff10110f)); g.fillRoundedRectangle (attachment, 5.0f);
            g.setColour (hoverRow == 40 ? juce::Colour (orange) : juce::Colour (0xff555852)); g.drawRoundedRectangle (attachment, 5.0f, 1.0f);
            g.setColour (hoverRow == 40 ? juce::Colour (orange) : juce::Colour (0xffa6a7a1)); g.drawText (attachmentSelected ? "FILE ATTACHED" : "+  ATTACH FILES", attachment.toNearestInt(), juce::Justification::centred);
            for (auto [label, box] : std::initializer_list<std::pair<juce::String, juce::Rectangle<int>>> { { "CANCEL", { 280, 546, 118, 40 } }, { "SEND", { 414, 546, 120, 40 } } }) { g.setColour (juce::Colour (0xff111210)); g.fillRoundedRectangle (box.toFloat(), 5.0f); g.setColour (label == "SEND" ? juce::Colour (orange) : juce::Colour (0xff555852)); g.drawRoundedRectangle (box.toFloat(), 5.0f, 1.0f); g.setColour (label == "SEND" ? juce::Colour (orange) : juce::Colour (text)); g.drawText (label, box, juce::Justification::centred); }
            return;
        }

        g.drawText (kind == Kind::SavePreset ? "SAVE PRESET" : "PRESET BROWSER", 18, 10, 180, 26, juce::Justification::centredLeft);
        g.setColour (juce::Colour (orange));
        g.drawLine (448.0f, 15.0f, 456.0f, 23.0f, 1.4f); g.drawLine (456.0f, 15.0f, 448.0f, 23.0f, 1.4f);
        if (kind == Kind::SavePreset)
        {
            g.setColour (juce::Colour (0xff8e8f89)); g.drawText ("PRESET NAME", 18, 35, 160, 20, juce::Justification::centredLeft);
            auto save = juce::Rectangle<float> (330, 116, 132, 42);
            g.setColour (juce::Colour (hoverRow == 20 ? 0xff2d170b : 0xff111210)); g.fillRoundedRectangle (save, 5.0f);
            g.setColour (juce::Colour (hoverRow == 20 ? orange : 0xff4d4d4a)); g.drawRoundedRectangle (save, 5.0f, 1.0f);
            g.setColour (juce::Colour (orange)); g.drawText ("SAVE", save.toNearestInt(), juce::Justification::centred);
            g.setColour (juce::Colour (0xff8e8f89)); g.drawText ("SAVED IN AURORAD80 / USER", 18, 116, 295, 42, juce::Justification::centredLeft, true);
            return;
        }

        auto search = juce::Rectangle<float> (18, 42, 444, 30);
        g.setColour (juce::Colour (0xff0b0c0b)); g.fillRoundedRectangle (search, 5.0f);
        g.setColour (juce::Colour (0xff41423f)); g.drawRoundedRectangle (search, 5.0f, 1.0f);
        g.setColour (juce::Colour (0xff858680)); g.drawEllipse (29.0f, 52.0f, 7.0f, 7.0f, 1.0f); g.drawLine (35.0f, 58.0f, 39.0f, 62.0f, 1.0f);
        g.drawText ("Search presets", search.reduced (27, 0).toNearestInt(), juce::Justification::centredLeft);
        const juce::StringArray tabs { "FACTORY", "USER", "FAVORITES" };
        for (int i = 0; i < tabs.size(); ++i)
        {
            auto tab = juce::Rectangle<int> (18 + i * 94, 80, 86, 25);
            g.setColour (i == 0 ? juce::Colour (orange) : juce::Colour (0xff8e8f89)); g.drawText (tabs[i], tab, juce::Justification::centredLeft);
        }
        const auto& presets = getFactoryPresets();
        for (int row = 0; row < 5; ++row)
        {
            const int index = juce::jlimit (0, (int) presets.size() - 1, firstPreset + row);
            auto rr = juce::Rectangle<float> (18, 110.0f + row * 32.0f, 444, 29);
            if (row == hoverRow) { g.setColour (juce::Colour (0xff241306)); g.fillRoundedRectangle (rr, 4.0f); g.setColour (juce::Colour (0xff8c3805)); g.drawRoundedRectangle (rr, 4.0f, 1.0f); }
            g.setColour (juce::Colour (orange)); g.drawText (juce::String (index + 1).paddedLeft ('0', 2), (int) rr.getX()+10, (int) rr.getY(), 30, (int) rr.getHeight(), juce::Justification::centredLeft);
            g.setColour (juce::Colour (text)); g.drawText (presets[(size_t) index].name, (int) rr.getX()+48, (int) rr.getY(), 245, (int) rr.getHeight(), juce::Justification::centredLeft);
            g.setColour (juce::Colour (0xff7e7f79)); g.drawText (presets[(size_t) index].category, (int) rr.getRight()-125, (int) rr.getY(), 110, (int) rr.getHeight(), juce::Justification::centredRight);
        }
        g.setColour (juce::Colour (0xff333431)); g.drawHorizontalLine (275, 18, 462);
        g.setColour (juce::Colour (0xff8e8f89)); g.drawText ("30 FACTORY PRESETS | 6 CATEGORIES", 18, 282, 300, 22, juce::Justification::centredLeft);
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        hoverRow = kind == Kind::Presets && e.y >= 110 && e.y < 270 ? (e.y - 110) / 32
                 : kind == Kind::SavePreset && juce::Rectangle<int> (330, 116, 132, 42).contains (e.getPosition()) ? 20
                 : kind == Kind::Settings && juce::Rectangle<int> (24, 240, 336, 38).contains (e.getPosition()) ? 10
                 : kind == Kind::Settings && e.x >= 24 && e.x < 360 && e.y >= 438 && e.y < 628 ? 30 + (e.y - 438) / 38
                 : kind == Kind::Feedback && juce::Rectangle<int> (26, 348, 508, 104).contains (e.getPosition()) ? 40 : -1;
        repaint();
    }
    void mouseExit (const juce::MouseEvent&) override { hoverRow = -1; repaint(); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        if (kind == Kind::Presets) { firstPreset = juce::jlimit (0, 25, firstPreset + (w.deltaY < 0 ? 1 : -1)); repaint(); }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (kind == Kind::Presets)
        {
            if (e.y < 38 && e.x > 425) { if (onClose) onClose(); return; }
            if (e.y >= 110 && e.y < 270 && onPreset) onPreset (firstPreset + (e.y - 110) / 32);
        }
        else if (kind == Kind::SavePreset)
        {
            if (e.y < 38 && e.x > 425) { if (onClose) onClose(); return; }
            if (juce::Rectangle<int> (330, 116, 132, 42).contains (e.getPosition()) && onSavePreset)
                onSavePreset (presetName.getText());
        }
        else if (kind == Kind::Settings)
        {
            if (juce::Rectangle<int> (342, 20, 34, 34).contains (e.getPosition())) { if (onClose) onClose(); return; }
            if (e.y >= 114 && e.y < 148 && e.x >= 24 && e.x < 365)
            {
                const int index = juce::jlimit (0, 4, (e.x - 24) / 70);
                selectedScale = 0.75f + index * 0.25f;
                if (onScaleChanged) onScaleChanged (selectedScale);
                repaint();
                return;
            }
            if (juce::Rectangle<int> (24, 240, 336, 38).contains (e.getPosition()) && onCopyAB) onCopyAB();
            else if (juce::Rectangle<int> (294, 199, 33, 31).contains (e.getPosition())) { selectedSlot = 0; if (onAction) onAction (5); repaint(); }
            else if (juce::Rectangle<int> (327, 199, 33, 31).contains (e.getPosition())) { selectedSlot = 1; if (onAction) onAction (6); repaint(); }
            else if (juce::Rectangle<int> (324, 332, 42, 26).contains (e.getPosition())) { graphicsEnabled = ! graphicsEnabled; if (onAction) onAction (7); repaint(); }
            else if (juce::Rectangle<int> (324, 360, 42, 26).contains (e.getPosition())) { hqEnabled = ! hqEnabled; if (onAction) onAction (8); repaint(); }
            else if (e.x >= 24 && e.x < 360 && e.y >= 438 && e.y < 628 && onAction)
                onAction ((e.y - 438) / 38);
        }
        else if (kind == Kind::Feedback)
        {
            if (juce::Rectangle<int> (26, 348, 508, 104).contains (e.getPosition())) { attachmentSelected = ! attachmentSelected; repaint(); }
            else if (juce::Rectangle<int> (280, 546, 118, 40).contains (e.getPosition())) { if (onClose) onClose(); }
            else if (juce::Rectangle<int> (414, 546, 120, 40).contains (e.getPosition())) { if (summary.getText().trim().isNotEmpty()) { summary.clear(); details.clear(); name.clear(); email.clear(); attachmentSelected = false; repaint(); } }
        }
    }
private:
    Kind kind; int hoverRow = -1, firstPreset = 0;
    juce::TextEditor presetName, summary, details, name, email;
    std::function<void(int)> onPreset, onAction; std::function<void()> onCopyAB, onClose;
    std::function<void(float)> onScaleChanged;
    float selectedScale = 0.75f;
    int selectedSlot = 0;
    bool graphicsEnabled = false, hqEnabled = false, attachmentSelected = false;
    std::function<void(const juce::String&)> onSavePreset;
};

class AuroraChoicePanel final : public juce::Component
{
public:
    AuroraChoicePanel (juce::StringArray choices, int selected, std::function<void(int)> choose,
                       std::function<void()> dismiss)
        : items (std::move (choices)), selectedIndex (selected), onChoose (std::move (choose)), onDismiss (std::move (dismiss))
    {
        setWantsKeyboardFocus (true);
        setSize (132, juce::jmin (404, 12 + items.size() * rowHeight));
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (juce::Colour (0xff090a09)); g.fillRoundedRectangle (r, 10.0f);
        g.setColour (juce::Colour (0xff555852)); g.drawRoundedRectangle (r, 10.0f, 1.0f);
        const int visibleRows = (getHeight() - 12) / rowHeight;
        for (int row = 0; row < visibleRows && first + row < items.size(); ++row)
        {
            const int index = first + row;
            auto area = juce::Rectangle<int> (8, 6 + row * rowHeight, getWidth() - 16, rowHeight);
            const bool selected = index == selectedIndex, hovered = index == hoverIndex;
            if (selected || hovered) { g.setColour (selected ? juce::Colour (0xff21130c) : juce::Colour (0xff1b1c1a)); g.fillRoundedRectangle (area.toFloat(), 5.0f); }
            if (hovered) { g.setColour (juce::Colour (orangeHi)); g.drawRoundedRectangle (area.toFloat(), 5.0f, 1.0f); }
            g.setColour (selected || hovered ? juce::Colour (orange) : juce::Colour (0xffdedbd6));
            g.setFont (juce::FontOptions ("Roboto Condensed", 16.0f, juce::Font::plain));
            g.drawText (items[index], area.reduced (12, 0), juce::Justification::centredLeft);
            if (selected) { g.setColour (juce::Colour (orange)); g.fillEllipse ((float) area.getX() + 3.0f, (float) area.getCentreY() - 3.5f, 7.0f, 7.0f); }
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int index = first + (e.y - 6) / rowHeight;
        if (index >= 0 && index < items.size() && onChoose) onChoose (index);
        if (onDismiss) onDismiss();
    }
    void mouseMove (const juce::MouseEvent& e) override
    {
        hoverIndex = first + (e.y - 6) / rowHeight;
        if (hoverIndex < 0 || hoverIndex >= items.size()) hoverIndex = -1;
        repaint();
    }
    void mouseExit (const juce::MouseEvent&) override { hoverIndex = -1; repaint(); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
    {
        const int visibleRows = (getHeight() - 12) / rowHeight;
        first = juce::jlimit (0, juce::jmax (0, items.size() - visibleRows), first + (wheel.deltaY < 0 ? 1 : -1));
        repaint();
    }
private:
    static constexpr int rowHeight = 38;
    juce::StringArray items; int selectedIndex = 0, first = 0, hoverIndex = -1;
    std::function<void(int)> onChoose; std::function<void()> onDismiss;
};
}

AuroraD80AudioProcessorEditor::AuroraLookAndFeel::AuroraLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0e0e0d));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xffd5d5d1));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff2b1a10));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (orange));
    setColour (juce::ComboBox::textColourId, juce::Colour (white));

    knobBody = juce::ImageCache::getFromMemory (BinaryData::knob_body_png, BinaryData::knob_body_pngSize);
    knobTicks = juce::Drawable::createFromImageData (BinaryData::knob_ticks_svg, BinaryData::knob_ticks_svgSize);
    knobShadow = juce::Drawable::createFromImageData (BinaryData::knob_shadow_svg, BinaryData::knob_shadow_svgSize);
    knobBezel = juce::Drawable::createFromImageData (BinaryData::knob_bezel_svg, BinaryData::knob_bezel_svgSize);
    knobPointerGlow = juce::Drawable::createFromImageData (BinaryData::knob_pointer_glow_svg, BinaryData::knob_pointer_glow_svgSize);
    knobPointer = juce::Drawable::createFromImageData (BinaryData::knob_pointer_svg, BinaryData::knob_pointer_svgSize);
    knobPointerHighlight = juce::Drawable::createFromImageData (BinaryData::knob_pointer_highlight_svg, BinaryData::knob_pointer_highlight_svgSize);
}

void AuroraD80AudioProcessorEditor::AuroraLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                                                         float p, float start, float end, juce::Slider&)
{
    // The production knob is assembled from the exact Figma assets of
    // "Applied / Knob / Input Trim" (same atomic knob reused across controls).
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);

    auto drawSvg = [&g] (const std::unique_ptr<juce::Drawable>& d, juce::Rectangle<float> target, float opacity = 1.0f)
    {
        if (d != nullptr)
            d->drawWithin (g, target, juce::RectanglePlacement::stretchToFit, opacity);
    };

    // Normalised placements copied from Figma node 6512:6749 (93.7368 x 117).
    drawSvg (knobTicks,  { r.getX() + r.getWidth() * 0.0511f, r.getY() + r.getHeight() * 0.1754f,
                          r.getWidth() * 0.8467f, r.getHeight() * 0.5790f });
    drawSvg (knobShadow, { r.getX() + r.getWidth() * 0.1387f, r.getY() + r.getHeight() * 0.2632f,
                          r.getWidth() * 0.6715f, r.getHeight() * 0.5380f });

    const float bezelSize = r.getWidth() * 0.6642f;
    const auto knobCentre = juce::Point<float> (r.getX() + r.getWidth() * 0.4780f,
                                                 r.getY() + r.getHeight() * 0.5385f);
    auto bezelRect = juce::Rectangle<float> (bezelSize, bezelSize).withCentre (knobCentre);
    drawSvg (knobBezel, bezelRect);

    const float bodySize = r.getWidth() * 0.6277f;
    auto bodyRect = juce::Rectangle<float> (bodySize, bodySize).withCentre (knobCentre);
    if (knobBody.isValid())
        g.drawImage (knobBody, bodyRect, juce::RectanglePlacement::stretchToFit);
    else
    {
        g.setColour (juce::Colour (0xff252525));
        g.fillEllipse (bodyRect);
    }

    // Figma's reference pointer is vertical at the parameter midpoint. Rotate
    // the real pointer/glow/highlight assets around the approved knob centre.
    const float angle = start + p * (end - start);
    const float rotation = angle - juce::MathConstants<float>::twoPi;
    juce::Graphics::ScopedSaveState state (g);
    g.addTransform (juce::AffineTransform::rotation (rotation, knobCentre.x, knobCentre.y));

    auto glowRect = juce::Rectangle<float> (r.getX() + r.getWidth() * 0.4599f, r.getY() + r.getHeight() * 0.2982f,
                                             r.getWidth() * 0.0321f, r.getHeight() * 0.1345f);
    auto pointerRect = juce::Rectangle<float> (r.getX() + r.getWidth() * 0.4628f, r.getY() + r.getHeight() * 0.2924f,
                                                r.getWidth() * 0.0263f, r.getHeight() * 0.1403f);
    auto highlightRect = juce::Rectangle<float> (r.getX() + r.getWidth() * 0.4723f, r.getY() + r.getHeight() * 0.2982f,
                                                  r.getWidth() * 0.0081f, r.getHeight() * 0.1228f);
    drawSvg (knobPointerGlow, glowRect, 0.95f);
    drawSvg (knobPointer, pointerRect);
    drawSvg (knobPointerHighlight, highlightRect, 0.95f);
}

void AuroraD80AudioProcessorEditor::AuroraLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool down,
                                                                     int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float> (0.5f, 0.5f, (float) w - 1, (float) h - 1);
    g.setGradientFill ({ juce::Colour (down ? 0xff17120f : 0xff28231f), r.getCentreX(), r.getY(), juce::Colour (0xff0d0b0a), r.getCentreX(), r.getBottom(), false });
    g.fillRoundedRectangle (r, 4.0f); g.setColour (juce::Colour (border)); g.drawRoundedRectangle (r, 4.0f, 1.0f);
    juce::Path a; a.addTriangle ((float) w - 17, h * 0.45f, (float) w - 9, h * 0.45f, (float) w - 13, h * 0.62f);
    g.setColour (juce::Colour (orange)); g.fillPath (a);
}

juce::Font AuroraD80AudioProcessorEditor::AuroraLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    const float size = juce::jlimit (7.0f, 12.0f, box.getHeight() * 0.34f);
    return juce::FontOptions ("Roboto Condensed", size, juce::Font::plain);
}

juce::Font AuroraD80AudioProcessorEditor::AuroraLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    const float size = juce::jlimit (7.0f, 14.0f, buttonHeight * 0.32f);
    return juce::FontOptions ("Roboto Condensed", size, juce::Font::plain);
}

juce::Font AuroraD80AudioProcessorEditor::AuroraLookAndFeel::getPopupMenuFont()
{
    return juce::FontOptions ("Roboto Condensed", 13.0f, juce::Font::plain);
}

void AuroraD80AudioProcessorEditor::AuroraLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    // JUCE's default 2 px popup border leaves a light native frame on macOS.
    // The Figma menu is a self-contained black rounded surface instead.
    const auto r = juce::Rectangle<float> (1.0f, 1.0f, (float) width - 2.0f, (float) height - 2.0f);
    g.setColour (juce::Colour (0xff090a09));
    g.fillRoundedRectangle (r, 10.0f);
    g.setColour (juce::Colour (0xff555852));
    g.drawRoundedRectangle (r, 10.0f, 1.0f);
}

void AuroraD80AudioProcessorEditor::AuroraLookAndFeel::drawPopupMenuBackgroundWithOptions (
    juce::Graphics& g, int width, int height, const juce::PopupMenu::Options&)
{
    drawPopupMenuBackground (g, width, height);
}

void AuroraD80AudioProcessorEditor::AuroraLookAndFeel::drawPopupMenuItem (
    juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator,
    bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
    const juce::String& itemText, const juce::String& shortcutKeyText,
    const juce::Drawable* icon, const juce::Colour* textColour)
{
    if (isSeparator)
    {
        g.setColour (juce::Colour (0xff3b3d39));
        g.fillRect (area.reduced (11, 0).withHeight (1).withCentre (area.getCentre()));
        return;
    }

    auto r = area.toFloat().reduced (4.0f, 1.5f);
    if (isHighlighted && isActive)
    {
        g.setGradientFill ({ juce::Colour (0xff2d170b), r.getCentreX(), r.getY(),
                             juce::Colour (0xff160d08), r.getCentreX(), r.getBottom(), false });
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (juce::Colour (orange));
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
    }

    auto colour = textColour != nullptr ? *textColour
                                        : juce::Colour (isActive ? (isHighlighted ? orangeHi : text) : 0xff666762);
    g.setColour (colour);
    g.setFont (getPopupMenuFont());
    auto textArea = area.reduced (14, 0);
    if (isTicked)
    {
        g.setColour (juce::Colour (orange));
        g.fillEllipse ((float) area.getX() + 7.0f, (float) area.getCentreY() - 3.0f, 6.0f, 6.0f);
    }
    if (icon != nullptr)
        icon->drawWithin (g, textArea.removeFromLeft (20).toFloat(), juce::RectanglePlacement::centred, isActive ? 1.0f : 0.45f);
    g.setColour (colour);
    g.drawFittedText (itemText, textArea, juce::Justification::centredLeft, 1);

    if (shortcutKeyText.isNotEmpty())
        g.drawFittedText (shortcutKeyText, area.reduced (12, 0), juce::Justification::centredRight, 1);
    if (hasSubMenu)
    {
        juce::Path arrow;
        const float x = (float) area.getRight() - 14.0f;
        const float y = (float) area.getCentreY();
        arrow.addTriangle (x - 3.0f, y - 5.0f, x + 3.0f, y, x - 3.0f, y + 5.0f);
        g.setColour (isHighlighted ? juce::Colour (orangeHi) : juce::Colour (0xff9b9d96));
        g.fillPath (arrow);
    }
}

void AuroraD80AudioProcessorEditor::AuroraLookAndFeel::getIdealPopupMenuItemSize (
    const juce::String& itemText, bool isSeparator, int, int& idealWidth, int& idealHeight)
{
    idealHeight = isSeparator ? 7 : 27;
    idealWidth = isSeparator ? 72 : juce::jmax (120, itemText.length() * 7 + 38);
}

int AuroraD80AudioProcessorEditor::AuroraLookAndFeel::getPopupMenuBorderSize() { return 0; }
int AuroraD80AudioProcessorEditor::AuroraLookAndFeel::getPopupMenuBorderSizeWithOptions (const juce::PopupMenu::Options&) { return 0; }

void AuroraD80AudioProcessorEditor::AuroraLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hi, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    auto top = on ? juce::Colour (0xff5b2d18) : juce::Colour (down ? 0xff17120f : 0xff292420);
    auto bottom = on ? juce::Colour (0xff24140e) : juce::Colour (0xff0e0c0b);
    g.setGradientFill ({ top, r.getCentreX(), r.getY(), bottom, r.getCentreX(), r.getBottom(), false });
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (on ? juce::Colour (orange) : juce::Colour (hi ? orangeHi : border)); g.drawRoundedRectangle (r, 5.0f, on ? 1.5f : 1.0f);
}

AuroraD80AudioProcessorEditor::AuroraD80AudioProcessorEditor (AuroraD80AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), designSurface (*this)
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);

    // Figma stays the master canvas at 1536x1024, but the plugin opens at 75%
    // (1152x768) for MacBook-sized displays. Host corner resizing is disabled
    // in this production-fidelity build so every control remains pixel-locked.
    addAndMakeVisible (designSurface);
    setSize (1152, 768);
    setResizable (false, false);

    chassisImage      = juce::ImageCache::getFromMemory (BinaryData::chassis_png, BinaryData::chassis_pngSize);
    headerImage       = removeBlackMatte (juce::ImageCache::getFromMemory (BinaryData::header_png, BinaryData::header_pngSize));
    displayImage      = juce::ImageCache::getFromMemory (BinaryData::display_png, BinaryData::display_pngSize);
    inputMeterImage   = makeNoSignalMeter (juce::ImageCache::getFromMemory (BinaryData::input_meter_png, BinaryData::input_meter_pngSize));
    outputMeterImage  = makeNoSignalMeter (juce::ImageCache::getFromMemory (BinaryData::output_meter_png, BinaryData::output_meter_pngSize));
    delayCoreImage    = juce::ImageCache::getFromMemory (BinaryData::delay_core_png, BinaryData::delay_core_pngSize);
    toneStereoImage   = juce::ImageCache::getFromMemory (BinaryData::tone_stereo_png, BinaryData::tone_stereo_pngSize);
    modCharacterImage = juce::ImageCache::getFromMemory (BinaryData::mod_character_png, BinaryData::mod_character_pngSize);
    utilityBarImage   = juce::ImageCache::getFromMemory (BinaryData::utility_bar_png, BinaryData::utility_bar_pngSize);
    legacyUtilityBarImage = juce::ImageCache::getFromMemory (BinaryData::utility_bar_legacy_png, BinaryData::utility_bar_legacy_pngSize);
    presetBarImage    = juce::ImageCache::getFromMemory (BinaryData::preset_bar_png, BinaryData::preset_bar_pngSize);
    auto headerSvg = juce::String::fromUTF8 (BinaryData::header_brand_svg, BinaryData::header_brand_svgSize)
        .replace ("<rect width=\"1096\" height=\"67\" fill=\"black\"/>\n", {})
        .replace ("<rect x=\"-221\" y=\"-23\" width=\"1536\" height=\"1024\" rx=\"14\" fill=\"#030303\"/>\n", {});
    headerBrand       = juce::Drawable::createFromImageData (headerSvg.toRawUTF8(), headerSvg.getNumBytesAsUTF8());
    freezeIcon        = juce::Drawable::createFromImageData (BinaryData::freeze_icon_svg, BinaryData::freeze_icon_svgSize);
    bypassIcon        = juce::Drawable::createFromImageData (BinaryData::bypass_icon_svg, BinaryData::bypass_icon_svgSize);
    breeSerifTypeface = juce::Typeface::createSystemTypefaceFor (BinaryData::BreeSerifRegular_ttf,
                                                                 BinaryData::BreeSerifRegular_ttfSize);
    arimoTypeface = juce::Typeface::createSystemTypefaceFor (BinaryData::Arimo_ttf, BinaryData::Arimo_ttfSize);

    configureSlider (inputSlider, inputLabel, inputValue, "INPUT TRIM", ValueFormat::GainDb);
    configureSlider (outputSlider, outputLabel, outputValue, "OUTPUT TRIM", ValueFormat::GainDb);
    configureSlider (timeSlider, timeLabel, timeValue, "TIME", ValueFormat::Milliseconds);
    configureSlider (feedbackSlider, feedbackLabel, feedbackValue, "FEEDBACK", ValueFormat::Percent01);
    configureSlider (mixSlider, mixLabel, mixValue, "MIX", ValueFormat::Percent01);
    configureSlider (lowCutSlider, lowCutLabel, lowCutValue, "LOW CUT", ValueFormat::Hz);
    configureSlider (highCutSlider, highCutLabel, highCutValue, "HIGH CUT", ValueFormat::KHz);
    configureSlider (widthSlider, widthLabel, widthValue, "WIDTH", ValueFormat::Percent100);
    configureSlider (depthSlider, depthLabel, depthValue, "DEPTH", ValueFormat::Percent100);
    configureSlider (rateSlider, rateLabel, rateValue, "RATE", ValueFormat::RateHz);
    configureSlider (driftSlider, driftLabel, driftValue, "DRIFT", ValueFormat::Percent100);
    configureSlider (driveSlider, driveLabel, driveValue, "DRIVE", ValueFormat::Percent100);
    configureSlider (evolveSlider, evolveLabel, evolveValue, "EVOLVE", ValueFormat::Percent100);
    configureSlider (bloomSlider, bloomLabel, bloomValue, "BLOOM", ValueFormat::Percent100);

    configureButton (syncButton, "SYNC");
    configureButton (pingPongButton, "PING-PONG");
    configureButton (freezeButton, "❄  FREEZE");
    configureButton (bypassButton, "BYPASS");

    configureChoice (divisionBox, { "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1/1",
                                    "1/64D", "1/32D", "1/16D", "1/8D", "1/4D", "1/2D", "1/1D",
                                    "1/64T", "1/32T", "1/16T", "1/8T", "1/4T", "1/2T", "1/1T" });
    configureChoice (shapeBox, { "SINE", "TRI", "SQUARE", "SAW", "RANDOM" });
    configureChoice (modeBox, { "DIGITAL", "TAPE", "ANALOG" });
    configureChoice (oversamplingBox, { "OFF", "2X", "4X", "8X" });
    qualitySlider.setSliderStyle (juce::Slider::LinearHorizontal);
    qualitySlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    qualitySlider.setRange (0.0, 2.0, 1.0); qualitySlider.setMouseDragSensitivity (110);
    qualitySlider.onValueChange = [this] { designSurface.repaint(); };
    designSurface.addAndMakeVisible (qualitySlider);

    for (auto* b : { &modeCycleButton, &oversamplingCycleButton })
    { b->setClickingTogglesState (false); b->setAlpha (0.01f); designSurface.addAndMakeVisible (*b); }
    modeCycleButton.onClick = [this]
    {
        modeBox.setSelectedItemIndex ((modeBox.getSelectedItemIndex() + 1) % modeBox.getNumItems(), juce::sendNotificationSync);
    };
    oversamplingCycleButton.onClick = [this]
    {
        oversamplingBox.setSelectedItemIndex ((oversamplingBox.getSelectedItemIndex() + 1) % oversamplingBox.getNumItems(), juce::sendNotificationSync);
    };
    for (const auto& preset : getFactoryPresets())
        presetNames.emplace_back (preset.name);
    installFactoryPresetLibrary();
    configurePresetBrowser();

    auto& apvts = audioProcessor.parameters;
    inputAttachment = std::make_unique<SliderAttachment> (apvts, "inputGain", inputSlider);
    outputAttachment = std::make_unique<SliderAttachment> (apvts, "outputGain", outputSlider);
    timeAttachment = std::make_unique<SliderAttachment> (apvts, "delayTimeMs", timeSlider);
    feedbackAttachment = std::make_unique<SliderAttachment> (apvts, "feedback", feedbackSlider);
    mixAttachment = std::make_unique<SliderAttachment> (apvts, "mix", mixSlider);
    lowCutAttachment = std::make_unique<SliderAttachment> (apvts, "lowCutHz", lowCutSlider);
    highCutAttachment = std::make_unique<SliderAttachment> (apvts, "highCutHz", highCutSlider);
    widthAttachment = std::make_unique<SliderAttachment> (apvts, "width", widthSlider);
    depthAttachment = std::make_unique<SliderAttachment> (apvts, "modulationDepth", depthSlider);
    rateAttachment = std::make_unique<SliderAttachment> (apvts, "modulationRate", rateSlider);
    driftAttachment = std::make_unique<SliderAttachment> (apvts, "drift", driftSlider);
    driveAttachment = std::make_unique<SliderAttachment> (apvts, "drive", driveSlider);
    evolveAttachment = std::make_unique<SliderAttachment> (apvts, "vintage", evolveSlider);
    bloomAttachment = std::make_unique<SliderAttachment> (apvts, "bloom", bloomSlider);
    syncAttachment = std::make_unique<ButtonAttachment> (apvts, "syncEnabled", syncButton);
    pingPongAttachment = std::make_unique<ButtonAttachment> (apvts, "pingPong", pingPongButton);
    freezeAttachment = std::make_unique<ButtonAttachment> (apvts, "freeze", freezeButton);
    bypassAttachment = std::make_unique<ButtonAttachment> (apvts, "bypass", bypassButton);
    divisionAttachment = std::make_unique<ComboBoxAttachment> (apvts, "syncDivision", divisionBox);
    shapeAttachment = std::make_unique<ComboBoxAttachment> (apvts, "modShape", shapeBox);
    modeAttachment = std::make_unique<ComboBoxAttachment> (apvts, "mode", modeBox);
    qualityAttachment = std::make_unique<SliderAttachment> (apvts, "quality", qualitySlider);
    oversamplingAttachment = std::make_unique<ComboBoxAttachment> (apvts, "oversampling", oversamplingBox);

    // Attachments can restore a host state without invoking a Slider callback.
    // Refresh every value readout once here so the visible text always matches
    // the parameter value from the first frame onward.
    updateValueLabel (inputSlider, inputValue, ValueFormat::GainDb);
    updateValueLabel (outputSlider, outputValue, ValueFormat::GainDb);
    updateValueLabel (timeSlider, timeValue, ValueFormat::Milliseconds);
    updateValueLabel (feedbackSlider, feedbackValue, ValueFormat::Percent01);
    updateValueLabel (mixSlider, mixValue, ValueFormat::Percent01);
    updateValueLabel (lowCutSlider, lowCutValue, ValueFormat::Hz);
    updateValueLabel (highCutSlider, highCutValue, ValueFormat::KHz);
    updateValueLabel (widthSlider, widthValue, ValueFormat::Percent100);
    updateValueLabel (depthSlider, depthValue, ValueFormat::Percent100);
    updateValueLabel (rateSlider, rateValue, ValueFormat::RateHz);
    updateValueLabel (driftSlider, driftValue, ValueFormat::Percent100);
    updateValueLabel (driveSlider, driveValue, ValueFormat::Percent100);
    updateValueLabel (evolveSlider, evolveValue, ValueFormat::Percent100);
    updateValueLabel (bloomSlider, bloomValue, ValueFormat::Percent100);

    syncPresetIdentity();
    saveSnapshotToSlot (0); saveSnapshotToSlot (1);
    startTimerHz (30);
}

AuroraD80AudioProcessorEditor::~AuroraD80AudioProcessorEditor() { setLookAndFeel (nullptr); }

void AuroraD80AudioProcessorEditor::configureSlider (juce::Slider& s, juce::Label& name, juce::Label& value, const juce::String& label, ValueFormat format)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f, true);
    s.setMouseDragSensitivity (220); designSurface.addAndMakeVisible (s);
    name.setText (label, juce::dontSendNotification); name.setJustificationType (juce::Justification::centred); name.setColour (juce::Label::textColourId, juce::Colour (text)); name.setFont (juce::FontOptions ("Roboto Condensed", 12.5f, juce::Font::plain)); designSurface.addAndMakeVisible (name);
    // The approved module exports already contain the precisely centred knob
    // names. Keep the live label available for accessibility, but do not paint
    // a second, slightly offset copy over the Figma artwork.
    name.setVisible (true);
    value.setJustificationType (juce::Justification::centred); value.setColour (juce::Label::textColourId, juce::Colour (white)); value.setFont (juce::FontOptions ("Roboto Condensed", 12.0f, juce::Font::plain)); designSurface.addAndMakeVisible (value);
    s.onValueChange = [this, &s, &value, format] { updateValueLabel (s, value, format); designSurface.repaint(); };
    updateValueLabel (s, value, format);
}

void AuroraD80AudioProcessorEditor::configureChoice (juce::ComboBox& b, const juce::StringArray& items)
{
    b.addItemList (items, 1);
    b.setJustificationType (juce::Justification::centred);
    b.onChange = [this] { designSurface.repaint(); };
    designSurface.addAndMakeVisible (b);
}

void AuroraD80AudioProcessorEditor::configureButton (juce::TextButton& b, const juce::String& label, bool toggle)
{
    b.setButtonText (label);
    b.setClickingTogglesState (toggle);
    b.setColour (juce::TextButton::textColourOffId, juce::Colour (text));
    b.setColour (juce::TextButton::textColourOnId, juce::Colour (orangeHi));
    b.onStateChange = [this] { designSurface.repaint(); };
    designSurface.addAndMakeVisible (b);
}

void AuroraD80AudioProcessorEditor::configurePresetBrowser()
{
    for (auto* b : { &previousPresetButton, &nextPresetButton, &loadButton, &saveAsButton, &aButton, &bButton, &undoRedoButton, &diceButton, &menuButton, &favouriteButton })
    { b->setClickingTogglesState (false); b->setColour (juce::TextButton::textColourOffId, juce::Colour (b == &diceButton ? orange : text)); designSurface.addAndMakeVisible (*b); }
    presetNumberLabel.setColour (juce::Label::textColourId, juce::Colour (orange)); presetNumberLabel.setJustificationType (juce::Justification::centred); presetNumberLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold)); designSurface.addAndMakeVisible (presetNumberLabel);
    presetNameLabel.setColour (juce::Label::textColourId, juce::Colour (orange)); presetNameLabel.setJustificationType (juce::Justification::centredLeft); presetNameLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold)); designSurface.addAndMakeVisible (presetNameLabel);
    presetNumberLabel.setVisible (false);
    presetNameLabel.setVisible (false);
    presetNumberLabel.setInterceptsMouseClicks (false, false); presetNameLabel.setInterceptsMouseClicks (false, false);
    presetDisplayButton.setButtonText ({});
    presetDisplayButton.setClickingTogglesState (false); presetDisplayButton.setAlpha (0.0f); designSurface.addAndMakeVisible (presetDisplayButton);
    favouriteButton.setClickingTogglesState (true); favouriteButton.setAlpha (0.01f);
    favouriteButton.onClick = [this] { designSurface.repaint(); };

    previousPresetButton.onClick = [this]
    {
        applyPreset (presetIdentityValid ? (currentPresetIndex + (int) presetNames.size() - 1) % (int) presetNames.size()
                                         : (int) presetNames.size() - 1);
    };
    nextPresetButton.onClick = [this]
    {
        applyPreset (presetIdentityValid ? (currentPresetIndex + 1) % (int) presetNames.size() : 0);
    };
    diceButton.onClick = [this] { randomiseMusicalParameters(); };
    aButton.onClick = [this] { activeSnapshotSlot = 0; if (juce::ModifierKeys::getCurrentModifiersRealtime().isShiftDown()) saveSnapshotToSlot (0); else recallSnapshotFromSlot (0); designSurface.repaint(); };
    bButton.onClick = [this] { activeSnapshotSlot = 1; if (juce::ModifierKeys::getCurrentModifiersRealtime().isShiftDown()) saveSnapshotToSlot (1); else recallSnapshotFromSlot (1); designSurface.repaint(); };
    undoRedoButton.onClick = [this]
    {
        if (! undoState.isValid()) return;
        const auto current = audioProcessor.parameters.copyState();
        audioProcessor.parameters.replaceState (undoState.createCopy());
        undoState = current;
        syncPresetIdentity();
        designSurface.repaint();
    };
    loadButton.onClick = [this] { applyPreset (presetIdentityValid ? currentPresetIndex : 0); };
    saveAsButton.onClick = [this] { showSavePresetBrowser(); };
    presetDisplayButton.onClick = [this] { showPresetBrowser(); };
    menuButton.onClick = [this] { showSettingsPanel(); };
    updateDynamicText();
}

void AuroraD80AudioProcessorEditor::closeOverlay()
{
    activeOverlay.reset();
}

void AuroraD80AudioProcessorEditor::DesignSurface::mouseDown (const juce::MouseEvent& e)
{
    const auto x = e.x / owner.editorScale, y = e.y / owner.editorScale;
    if (juce::Rectangle<float> (394.37f, 451.0f, 71.0f, 36.0f).contains (x, y))
        owner.showChoiceMenu (owner.divisionBox, e.getPosition(), owner.divisionBox.getItemText (1).isNotEmpty() ? [&] { juce::StringArray a; for (int i = 1; i <= owner.divisionBox.getNumItems(); ++i) a.add (owner.divisionBox.getItemText (i)); return a; }() : juce::StringArray());
    else if (juce::Rectangle<float> (638.0f, 683.0f, 88.0f, 38.0f).contains (x, y))
        owner.showChoiceMenu (owner.shapeBox, e.getPosition(), [&] { juce::StringArray a; for (int i = 1; i <= owner.shapeBox.getNumItems(); ++i) a.add (owner.shapeBox.getItemText (i)); return a; }());
}

void AuroraD80AudioProcessorEditor::showChoiceMenu (juce::ComboBox& box, juce::Point<int> anchor, const juce::StringArray& items)
{
    closeOverlay();
    activeOverlay = std::make_unique<AuroraChoicePanel> (items, box.getSelectedItemIndex(),
        [&box] (int index) { box.setSelectedItemIndex (index, juce::sendNotificationSync); },
        [this] { juce::MessageManager::callAsync ([this] { closeOverlay(); }); });
    addAndMakeVisible (*activeOverlay);
    auto x = anchor.x; auto y = anchor.y + 28;
    activeOverlay->setBounds (x, juce::jmin (y, getHeight() - activeOverlay->getHeight() - 4), activeOverlay->getWidth(), activeOverlay->getHeight());
    activeOverlay->toFront (true);
}

void AuroraD80AudioProcessorEditor::setEditorScale (float scale)
{
    editorScale = juce::jlimit (0.75f, 2.0f, scale);
    setSize (juce::roundToInt (1536.0f * editorScale), juce::roundToInt (1024.0f * editorScale));
    resized();
}

void AuroraD80AudioProcessorEditor::showPresetBrowser()
{
    closeOverlay();
    activeOverlay = std::make_unique<AuroraFloatingPanel> (AuroraFloatingPanel::Kind::Presets,
        [this] (int index) { applyPreset (index); juce::MessageManager::callAsync ([this] { closeOverlay(); }); },
        nullptr, [this] { juce::MessageManager::callAsync ([this] { closeOverlay(); }); });
    addAndMakeVisible (*activeOverlay);
    activeOverlay->setBounds (juce::roundToInt ((153.0f + masterCentreCorrectionX) * editorScale),
                              juce::roundToInt (891.0f * editorScale) - 315, 480, 315);
    activeOverlay->toFront (true);
}

void AuroraD80AudioProcessorEditor::showSavePresetBrowser()
{
    closeOverlay();
    activeOverlay = std::make_unique<AuroraFloatingPanel> (AuroraFloatingPanel::Kind::SavePreset, nullptr, nullptr,
        [this] { juce::MessageManager::callAsync ([this] { closeOverlay(); }); },
        [this] (const juce::String& requestedName)
        {
            const auto name = requestedName.trim().isEmpty() ? juce::String ("My Aurora Preset") : requestedName.trim();
            auto folder = juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                              .getChildFile ("Library/Audio/Presets/Prismatica Audio Labs/AuroraD80/User");
            folder.createDirectory();
            if (auto xml = audioProcessor.parameters.copyState().createXml())
                folder.getChildFile (juce::File::createLegalFileName (name) + ".aurora").replaceWithText (xml->toString());
            presetIdentityValid = false;
            presetNumberLabel.setText ("--", juce::dontSendNotification);
            presetNameLabel.setText (name, juce::dontSendNotification);
            juce::MessageManager::callAsync ([this] { closeOverlay(); });
        });
    addAndMakeVisible (*activeOverlay);
    activeOverlay->setBounds (juce::roundToInt ((153.0f + masterCentreCorrectionX) * editorScale),
                              juce::roundToInt (891.0f * editorScale) - 190, 480, 190);
    activeOverlay->toFront (true);
}

void AuroraD80AudioProcessorEditor::showSettingsPanel()
{
    closeOverlay();
    activeOverlay = std::make_unique<AuroraFloatingPanel> (AuroraFloatingPanel::Kind::Settings, nullptr,
        [this] { snapshotB = snapshotA.createCopy(); activeSnapshotSlot = 1; designSurface.repaint(); },
        [this] { juce::MessageManager::callAsync ([this] { closeOverlay(); }); }, nullptr,
        [this] (int action)
        {
            if (action == 0)
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon,
                                                         "Aurora D80", "Aurora D80 is up to date.");
            else if (action == 1)
                juce::URL ("https://www.prismaticalabs.com").launchInDefaultBrowser();
            else if (action == 2)
                juce::URL ("https://www.prismaticalabs.com/tutorials").launchInDefaultBrowser();
            else if (action == 3)
                juce::URL ("https://www.prismaticalabs.com/manual").launchInDefaultBrowser();
            else if (action == 4)
                showFeedbackPanel();
            else if (action == 5)
                recallSnapshotFromSlot (0);
            else if (action == 6)
                recallSnapshotFromSlot (1);
            else if (action == 7)
            {
                graphicsAccelerationEnabled = ! graphicsAccelerationEnabled;
                designSurface.setBufferedToImage (! graphicsAccelerationEnabled);
            }
            else if (action == 8)
            {
                hqOversamplingEnabled = ! hqOversamplingEnabled;
                oversamplingBox.setSelectedItemIndex (hqOversamplingEnabled ? 3 : 1, juce::sendNotificationSync);
            }
        }, [this] (float scale) { setEditorScale (scale); }, editorScale);
    addAndMakeVisible (*activeOverlay);
    activeOverlay->setBounds (getWidth() - 405, getHeight() - 665, 390, 650);
    activeOverlay->toFront (true);
}

void AuroraD80AudioProcessorEditor::showFeedbackPanel()
{
    closeOverlay();
    activeOverlay = std::make_unique<AuroraFloatingPanel> (AuroraFloatingPanel::Kind::Feedback, nullptr, nullptr,
        [this] { juce::MessageManager::callAsync ([this] { closeOverlay(); }); });
    addAndMakeVisible (*activeOverlay);
    activeOverlay->setBounds ((getWidth() - 560) / 2, (getHeight() - 620) / 2, 560, 620);
    activeOverlay->toFront (true);
}

void AuroraD80AudioProcessorEditor::showFactoryPresetMenu()
{
    juce::PopupMenu root;
    const auto& presets = getFactoryPresets();
    const juce::StringArray categories { "Cinematic", "Ambient", "Rhythmic", "Tape & Analog", "Modulated", "Experimental" };

    for (const auto& category : categories)
    {
        juce::PopupMenu folder;
        for (int i = 0; i < (int) presets.size(); ++i)
            if (category == presets[(size_t) i].category)
                folder.addItem (1000 + i, presets[(size_t) i].name, true, presetIdentityValid && i == currentPresetIndex);
        root.addSubMenu (category, folder);
    }

    auto options = juce::PopupMenu::Options().withTargetComponent (&menuButton)
                                               .withMinimumWidth (240)
                                               .withStandardItemHeight (34);
    root.showMenuAsync (options, [this] (int result)
    {
        if (result >= 1000 && result < 1030)
            applyPreset (result - 1000);
    });
}

void AuroraD80AudioProcessorEditor::showMainMenu()
{
    juce::PopupMenu menu;
    juce::PopupMenu presetFolders;
    const auto& presets = getFactoryPresets();
    const juce::StringArray categories { "Cinematic", "Ambient", "Rhythmic", "Tape & Analog", "Modulated", "Experimental" };
    for (const auto& category : categories)
    {
        juce::PopupMenu folder;
        for (int i = 0; i < (int) presets.size(); ++i)
            if (category == presets[(size_t) i].category)
                folder.addItem (1000 + i, presets[(size_t) i].name, true, presetIdentityValid && i == currentPresetIndex);
        presetFolders.addSubMenu (category, folder);
    }

    menu.addSectionHeader ("AURORA D80");
    menu.addSubMenu ("Factory Presets", presetFolders);
    menu.addItem (1, "Random Atmosphere");
    menu.addItem (2, "Reset to Deep Horizon");
    menu.addSeparator();
    menu.addItem (3, "Store current state in B");

    auto options = juce::PopupMenu::Options().withTargetComponent (&menuButton)
                                               .withMinimumWidth (250)
                                               .withStandardItemHeight (34);
    menu.showMenuAsync (options, [this] (int result)
    {
        if (result >= 1000 && result < 1030) applyPreset (result - 1000);
        else if (result == 1) randomiseMusicalParameters();
        else if (result == 2) applyPreset (0);
        else if (result == 3) saveSnapshotToSlot (1);
    });
}

void AuroraD80AudioProcessorEditor::updateValueLabel (juce::Slider& s, juce::Label& l, ValueFormat f)
{
    const auto v = s.getValue();
    juce::String out;
    switch (f)
    {
        case ValueFormat::GainDb:
        {
            out = juce::String (v >= 0.0 ? "+" : "") + juce::String (v, 1) + " dB";
            break;
        }
        case ValueFormat::Percent01: out = juce::String (juce::roundToInt (v * 100.0)) + "%"; break;
        case ValueFormat::Percent100: out = juce::String (juce::roundToInt (v)) + "%"; break;
        case ValueFormat::Milliseconds: out = juce::String (juce::roundToInt (v)) + " ms"; break;
        case ValueFormat::Hz: out = juce::String (juce::roundToInt (v)) + " Hz"; break;
        case ValueFormat::KHz: out = juce::String (v / 1000.0, 1) + " kHz"; break;
        case ValueFormat::RateHz: out = juce::String (v, 2) + " Hz"; break;
    }
    l.setText (out, juce::dontSendNotification);
}

void AuroraD80AudioProcessorEditor::updateDynamicText()
{
    if (! presetIdentityValid)
    {
        presetNumberLabel.setText ("--", juce::dontSendNotification);
        presetNameLabel.setText ("Custom State", juce::dontSendNotification);
        return;
    }

    presetNumberLabel.setText (juce::String (currentPresetIndex + 1).paddedLeft ('0', 2), juce::dontSendNotification);
    if (! presetNames.empty())
        presetNameLabel.setText (presetNames[(size_t) currentPresetIndex], juce::dontSendNotification);
}

void AuroraD80AudioProcessorEditor::syncPresetIdentity()
{
    const auto current = [this] (const char* id)
    {
        if (auto* value = audioProcessor.parameters.getRawParameterValue (id))
            return value->load();
        return 0.0f;
    };
    const auto close = [] (float a, float b, float tolerance)
    {
        return std::abs (a - b) <= tolerance;
    };

    presetIdentityValid = false;
    const auto& presets = getFactoryPresets();
    for (int i = 0; i < (int) presets.size(); ++i)
    {
        const auto& v = presets[(size_t) i];
        if (close (current ("inputGain"), v.input, 0.02f)
            && close (current ("outputGain"), v.output, 0.02f)
            && close (current ("delayTimeMs"), v.time, 0.5f)
            && close (current ("feedback"), v.feedback, 0.001f)
            && close (current ("mix"), v.mix, 0.001f)
            && close (current ("lowCutHz"), v.low, 0.5f)
            && close (current ("highCutHz"), v.high, 1.0f)
            && close (current ("width"), v.width, 0.02f)
            && close (current ("modulationDepth"), v.depth, 0.02f)
            && close (current ("modulationRate"), v.rate, 0.002f)
            && close (current ("drift"), v.drift, 0.02f)
            && close (current ("drive"), v.drive, 0.02f)
            && close (current ("vintage"), v.evolve, 0.02f)
            && close (current ("bloom"), v.bloom, 0.02f)
            && close (current ("syncDivision"), (float) v.division, 0.01f)
            && close (current ("modShape"), (float) v.shape, 0.01f)
            && close (current ("mode"), (float) v.mode, 0.01f)
            && close (current ("quality"), (float) v.quality, 0.01f)
            && close (current ("oversampling"), (float) v.oversampling, 0.01f)
            && close (current ("syncEnabled"), v.sync ? 1.0f : 0.0f, 0.01f)
            && close (current ("pingPong"), v.pingPong ? 1.0f : 0.0f, 0.01f))
        {
            currentPresetIndex = i;
            presetIdentityValid = true;
            break;
        }
    }
    updateDynamicText();
}

void AuroraD80AudioProcessorEditor::updateMeters()
{
    constexpr float silenceFloor = 0.001f; // -60 dBFS
    constexpr float release = 0.82f;       // about 250 ms at 30 Hz
    for (int c = 0; c < 2; ++c)
    {
        const auto sanitise = [] (float value)
        {
            return std::isfinite (value) ? juce::jlimit (0.0f, 4.0f, value) : 0.0f;
        };
        const auto in = sanitise (audioProcessor.getInputRmsLevel (c));
        const auto out = sanitise (audioProcessor.getOutputRmsLevel (c));
        auto& input = inputMeterLevels[(size_t) c];
        auto& output = outputMeterLevels[(size_t) c];
        input = juce::jmax (in, input * release);
        output = juce::jmax (out, output * release);
        if (input < silenceFloor) input = 0.0f;
        if (output < silenceFloor) output = 0.0f;
    }
}

void AuroraD80AudioProcessorEditor::timerCallback()
{
    updateMeters();
    const auto rateHz = juce::jlimit (0.1f, 10.0f, (float) rateSlider.getValue());
    waveformPhase += juce::MathConstants<float>::twoPi * rateHz / 30.0f;
    waveformPhase = std::fmod (waveformPhase, juce::MathConstants<float>::twoPi);
    designSurface.repaint();

   #if JUCE_DEBUG
    // Opt-in render hook used by automated GUI QA. It is inert in normal use
    // and excluded from Release builds.
    if (! debugSnapshotWritten)
    {
        const auto path = juce::SystemStats::getEnvironmentVariable ("AURORA_GUI_SNAPSHOT_PATH", {});
        if (path.isNotEmpty())
        {
            debugSnapshotWritten = true;
            auto image = createComponentSnapshot (getLocalBounds(), true, 1.0f);
            if (auto stream = juce::File (path).createOutputStream())
                juce::PNGImageFormat().writeImageToStream (image, *stream);
        }
    }
   #endif
}

void AuroraD80AudioProcessorEditor::applyPreset (int i)
{
    undoState = audioProcessor.parameters.copyState();
    currentPresetIndex = juce::jlimit (0, (int) presetNames.size() - 1, i);
    presetIdentityValid = true;
    const auto& v = getFactoryPresets()[(size_t) currentPresetIndex];
    auto set = [this] (const char* id, float value) { if (auto* p = audioProcessor.parameters.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (value)); };
    set ("inputGain", v.input); set ("outputGain", v.output); set ("delayTimeMs", v.time);
    set ("feedback", v.feedback); set ("mix", v.mix); set ("lowCutHz", v.low); set ("highCutHz", v.high);
    set ("width", v.width); set ("modulationDepth", v.depth); set ("modulationRate", v.rate);
    set ("drift", v.drift); set ("drive", v.drive); set ("vintage", v.evolve); set ("bloom", v.bloom);
    set ("syncDivision", (float) v.division); set ("modShape", (float) v.shape); set ("mode", (float) v.mode);
    set ("quality", (float) v.quality); set ("oversampling", (float) v.oversampling);
    set ("syncEnabled", v.sync ? 1.0f : 0.0f); set ("pingPong", v.pingPong ? 1.0f : 0.0f);
    set ("freeze", 0.0f); set ("bypass", 0.0f);
    updateDynamicText(); designSurface.repaint();
}

void AuroraD80AudioProcessorEditor::randomiseMusicalParameters()
{
    undoState = audioProcessor.parameters.copyState();
    juce::Random r;
    auto set = [this] (const char* id, float value) { if (auto* p = audioProcessor.parameters.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (value)); };
    set ("delayTimeMs", r.nextFloat() * 900.0f + 120.0f); set ("feedback", r.nextFloat() * 0.55f + 0.18f); set ("mix", r.nextFloat() * 0.35f + 0.20f);
    set ("width", r.nextFloat() * 80.0f + 90.0f); set ("modulationDepth", r.nextFloat() * 35.0f); set ("modulationRate", r.nextFloat() * 0.7f + 0.10f);
    set ("drift", r.nextFloat() * 35.0f); set ("drive", r.nextFloat() * 28.0f); set ("vintage", r.nextFloat() * 72.0f); set ("bloom", r.nextFloat() * 65.0f);
    presetIdentityValid = false;
    presetNumberLabel.setText ("--", juce::dontSendNotification);
    presetNameLabel.setText ("Random Atmosphere", juce::dontSendNotification); designSurface.repaint();
}

void AuroraD80AudioProcessorEditor::saveSnapshotToSlot (int slot)
{
    auto copy = audioProcessor.parameters.copyState();
    if (slot == 0) snapshotA = copy; else snapshotB = copy;
}
void AuroraD80AudioProcessorEditor::recallSnapshotFromSlot (int slot)
{
    auto state = slot == 0 ? snapshotA : snapshotB;
    if (state.isValid())
    {
        audioProcessor.parameters.replaceState (state.createCopy());
        syncPresetIdentity();
    }
}

void AuroraD80AudioProcessorEditor::paint (juce::Graphics& g)
{
    // Only paint the host/editor background here. All actual plugin artwork is
    // painted inside designSurface so artwork and controls share one transform.
    g.fillAll (juce::Colour (bg0));
}

void AuroraD80AudioProcessorEditor::paintDesign (juce::Graphics& g)
{
    // All coordinates below are literal Figma master coordinates (1536x1024).
    // Apply one root centring correction before scaling so raster artwork,
    // custom paint and component hit areas share the same master-space origin.
    g.addTransform (juce::AffineTransform (editorScale, 0.0f, masterCentreCorrectionX * editorScale,
                                           0.0f, editorScale, 0.0f));
    Layout L;

    auto drawImageAt = [&g] (const juce::Image& image, juce::Rectangle<int> target)
    {
        if (image.isValid())
            g.drawImage (image, target.toFloat(), juce::RectanglePlacement::stretchToFit);
    };

    // Figma exports include the effect bounds outside each real node. The meter
    // PNG is 216x727 for a 174x685 node (21 px per side); other modules use
    // 8/10 px. Those outer pixels contain a black export matte, not artwork.
    // Crop to the actual node bounds so the metal outline is authoritative and
    // no rectangular black padding is composited over the chassis.
    auto drawFigmaExport = [&g, &drawImageAt] (const juce::Image& image, juce::Rectangle<int> nodeBounds, bool asymmetricMeter = false)
    {
        if (! image.isValid())
            return;

        if (asymmetricMeter && image.getWidth() >= 195 && image.getHeight() >= 696)
        {
            g.drawImage (image,
                         nodeBounds.getX(), nodeBounds.getY(), nodeBounds.getWidth(), nodeBounds.getHeight(),
                         21, 11, 174, 685, false);
            return;
        }

        const int extraW = image.getWidth()  - nodeBounds.getWidth();
        const int extraH = image.getHeight() - nodeBounds.getHeight();
        if (extraW >= 0 && extraH >= 0 && extraW % 2 == 0 && extraH % 2 == 0)
        {
            g.drawImage (image,
                         nodeBounds.getX(), nodeBounds.getY(),
                         nodeBounds.getWidth(), nodeBounds.getHeight(),
                         extraW / 2, extraH / 2,
                         nodeBounds.getWidth(), nodeBounds.getHeight(), false);
            return;
        }

        drawImageAt (image, nodeBounds);
    };

    if (chassisImage.isValid())
        drawImageAt (chassisImage, L.chassis);
    else
        g.fillAll (juce::Colour (bg0));

    // Production Figma modules. These preserve the real brushed-metal fills,
    // borders, inset shadows and section-rule alignment from the approved node.
    drawFigmaExport (inputMeterImage,  L.inputMeter, true);
    drawFigmaExport (outputMeterImage, L.outputMeter, true);
    drawFigmaExport (displayImage,     L.display);
    drawFigmaExport (delayCoreImage,   L.delay);
    drawFigmaExport (toneStereoImage,  L.tone);
    drawFigmaExport (modCharacterImage, L.modulationCharacter);
    // Use the earlier approved Utility export as one coherent surface. Mixing
    // the newer strip with a cropped legacy logo produced doubled borders and
    // inconsistent control centres.
    drawFigmaExport (utilityBarImage, L.utility);
    drawFigmaExport (presetBarImage,   L.presets);

    if (headerBrand != nullptr)
        headerBrand->drawWithin (g, { 221.0f, 23.0f, 1096.0f, 67.0f }, juce::RectanglePlacement::stretchToFit, 1.0f);
    else
        drawImageAt (headerImage, L.header);

    // These details live directly on the Figma root rather than inside the
    // chassis export, so reproduce them in root coordinates here.
    auto drawChassisScrew = [&g] (float centreX, float centreY)
    {
        const auto screw = juce::Rectangle<float> (centreX - 10.75f, centreY - 10.75f, 21.5f, 21.5f);
        g.setGradientFill ({ juce::Colour (0xff363939), screw.getX(), screw.getY(),
                             juce::Colour (0xff080909), screw.getRight(), screw.getBottom(), false });
        g.fillEllipse (screw);
        g.setColour (juce::Colour (0xff575b5a));
        g.drawEllipse (screw.reduced (1.0f), 1.0f);
        g.setColour (juce::Colour (0xff070808));
        g.drawLine (centreX - 4.5f, centreY + 3.5f, centreX + 4.5f, centreY - 3.5f, 2.0f);
        g.setColour (juce::Colour (0x604f5351));
        g.drawLine (centreX - 4.0f, centreY + 2.0f, centreX + 4.0f, centreY - 4.0f, 0.8f);
    };
    drawChassisScrew (41.5f, 33.0f);
    drawChassisScrew (1496.5f, 33.0f);
    drawChassisScrew (41.5f, 989.0f);
    drawChassisScrew (1496.5f, 989.0f);

    g.setColour (juce::Colour (0xff565753));
    g.fillRect (58.0f, 989.0f, 1215.0f, 2.0f);
    g.setColour (juce::Colour (0xffd0d0cb));
    g.setFont (juce::FontOptions ("Roboto Condensed", 12.0f, juce::Font::plain));
    g.drawText ("MADE IN DOMINICAN REPUBLIC", 1282, 982, 195, 14,
                juce::Justification::centredRight, false);

    if (false)
    {
    // The display waveform reflects the modulation Shape parameter. Only the
    // waveform viewport is replaced; the Figma glass, bezel and readout remain.
    // Keep the animated viewport clear of the baked status legend below it.
    const auto waveArea = juce::Rectangle<float> (462.0f, 153.0f, 634.0f, 103.0f);
    g.setGradientFill ({ juce::Colour (0xff111412), waveArea.getX(), waveArea.getCentreY(),
                         juce::Colour (0xff050706), waveArea.getRight(), waveArea.getCentreY(), false });
    g.fillRoundedRectangle (waveArea, 3.0f);
    auto drawWave = [&g, this, waveArea] (float baseline, float phaseOffset, float opacity)
    {
        juce::Graphics::ScopedSaveState clipped (g);
        g.reduceClipRegion (waveArea.getSmallestIntegerContainer());
        juce::Path p; const int shape = shapeBox.getSelectedItemIndex();
        const float depth = juce::jlimit (0.0f, 1.0f, (float) depthSlider.getValue() / 100.0f);
        const float drift = juce::jlimit (0.0f, 1.0f, (float) driftSlider.getValue() / 100.0f);
        const float feedback = juce::jlimit (0.0f, 0.95f, (float) feedbackSlider.getValue());
        const float timeScale = juce::jmap ((float) timeSlider.getValue(), 1.0f, 2000.0f, 1.28f, 0.72f);
        for (int i = 0; i <= 124; ++i)
        {
            const float x = waveArea.getX() + i * (waveArea.getWidth() / 124.0f);
            const float t = i * 0.19f * timeScale + waveformPhase + phaseOffset;
            float sample = 0.0f;
            if (shape == 1) sample = 2.0f / juce::MathConstants<float>::pi * std::asin (std::sin (t));
            else if (shape == 2) sample = juce::jlimit (-1.0f, 1.0f, std::sin (t * 1.73f) * 0.65f + std::sin (t * 3.17f + drift * 2.0f) * 0.35f);
            else sample = std::sin (t);
            const float decay = std::exp (-i / (45.0f + feedback * 100.0f));
            const float amplitude = 24.0f + depth * 20.0f + feedback * 22.0f;
            const float y = baseline - sample * amplitude * decay;
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (juce::Colour (0xffff6105).withAlpha (opacity));
        g.strokePath (p, juce::PathStrokeType (1.35f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };
    drawWave (204.0f, 0.0f, 0.92f);
    drawWave (230.0f, 1.45f, 0.72f);

    // Dynamic display v1.4. Draw the numeric readout as actual seven-segment
    // geometry so it is identical on every host and never depends on an
    // end-user font installation.
    const auto readout = juce::Rectangle<float> (641.0f, 205.0f, 255.0f, 103.0f);
    g.setColour (juce::Colour (0xff050505));
    g.fillRoundedRectangle (readout.reduced (5.0f), 8.0f);
    const auto digits = juce::String (juce::roundToInt (timeSlider.getValue()));
    const float digitW = digits.length() <= 3 ? 42.0f : 33.0f;
    const float digitH = 68.0f;
    const float digitGap = digits.length() <= 3 ? 7.0f : 4.0f;
    const float digitsW = digits.length() * digitW + (digits.length() - 1) * digitGap;
    const float digitStartX = 804.0f - digitsW;
    const float digitY = 222.0f;
    const float thickness = digits.length() <= 3 ? 6.0f : 5.0f;
    const std::array<unsigned int, 10> segmentMask { 0x3f, 0x06, 0x5b, 0x4f, 0x66,
                                                     0x6d, 0x7d, 0x07, 0x7f, 0x6f };
    auto drawSegmentDigit = [&g, &segmentMask] (int digit, float x, float y, float w, float h, float t)
    {
        const float half = h * 0.5f;
        const std::array<juce::Rectangle<float>, 7> segments {{
            { x + t, y, w - 2.0f * t, t }, { x + w - t, y + t, t, half - 1.5f * t },
            { x + w - t, y + half + 0.5f * t, t, half - 1.5f * t },
            { x + t, y + h - t, w - 2.0f * t, t }, { x, y + half + 0.5f * t, t, half - 1.5f * t },
            { x, y + t, t, half - 1.5f * t }, { x + t, y + half - 0.5f * t, w - 2.0f * t, t }
        }};
        const auto mask = segmentMask[(size_t) juce::jlimit (0, 9, digit)];
        for (size_t i = 0; i < segments.size(); ++i)
            if ((mask & (1u << i)) != 0)
            {
                g.setColour (juce::Colour (0x35ff5b00));
                g.fillRoundedRectangle (segments[i].expanded (2.0f), t * 0.55f);
                g.setColour (juce::Colour (0xffff6508));
                g.fillRoundedRectangle (segments[i], t * 0.45f);
            }
    };
    for (int i = 0; i < digits.length(); ++i)
        drawSegmentDigit (digits.substring (i, i + 1).getIntValue(),
                          digitStartX + i * (digitW + digitGap), digitY, digitW, digitH, thickness);

    const float unitX = 821.0f, unitY = 239.0f, unitH = 42.0f;
    g.setColour (juce::Colour (0xffff6508));
    juce::Path unitM;
    unitM.startNewSubPath (unitX, unitY + unitH); unitM.lineTo (unitX, unitY);
    unitM.lineTo (unitX + 10.0f, unitY + 17.0f); unitM.lineTo (unitX + 20.0f, unitY);
    unitM.lineTo (unitX + 20.0f, unitY + unitH);
    g.strokePath (unitM, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    drawSegmentDigit (5, unitX + 28.0f, unitY, 27.0f, unitH, 4.0f);

    // Replace the static reference status with the live sync division.
    // Reuse a clean one-pixel strip from the original glass gradient. Stretching
    // it vertically removes the baked reference status without introducing a
    // visible rectangular patch.
    g.drawImage (displayImage, 650, 127, 250, 25, 253, 30, 250, 1, false);
    g.setColour (juce::Colour (0xffff630d));
    g.setFont (arimoTypeface != nullptr
                   ? juce::Font (juce::FontOptions (arimoTypeface).withHeight (14.0f))
                   : juce::Font (juce::FontOptions ("Arial", 14.0f, juce::Font::plain)));
    const auto syncStatus = syncButton.getToggleState()
        ? "SYNCED: " + divisionBox.getText().toUpperCase() + " NOTE | 120 BPM"
        : "FREE TIME | 120 BPM";
    g.drawText (syncStatus, 650, 127, 250, 25, juce::Justification::centred, false);
    }

    auto drawDisplayDigit = [&g] (int digit, float x, float y, float w, float h, float t)
    {
        static const std::array<unsigned int, 10> masks { 0x3f, 0x06, 0x5b, 0x4f, 0x66,
                                                          0x6d, 0x7d, 0x07, 0x7f, 0x6f };
        const float half = h * 0.5f;
        const std::array<juce::Rectangle<float>, 7> segments {{
            { x + t, y, w - 2.0f * t, t }, { x + w - t, y + t, t, half - 1.5f * t },
            { x + w - t, y + half + 0.5f * t, t, half - 1.5f * t },
            { x + t, y + h - t, w - 2.0f * t, t }, { x, y + half + 0.5f * t, t, half - 1.5f * t },
            { x, y + t, t, half - 1.5f * t }, { x + t, y + half - 0.5f * t, w - 2.0f * t, t }
        }};
        const auto mask = masks[(size_t) juce::jlimit (0, 9, digit)];
        for (size_t i = 0; i < segments.size(); ++i)
        {
            if ((mask & (1u << i)) == 0)
                continue;
            g.setColour (juce::Colour (0x40ff5b00));
            g.fillRoundedRectangle (segments[i].expanded (2.0f), t * 0.55f);
            g.setColour (juce::Colour (0xffff6508));
            g.fillRoundedRectangle (segments[i], t * 0.45f);
        }
    };
    const juce::String displayDigits { "587" };
    const float displayDigitW = 42.0f;
    const float displayDigitH = 68.0f;
    const float displayDigitGap = 7.0f;
    const float displayDigitsWidth = displayDigits.length() * displayDigitW + (displayDigits.length() - 1) * displayDigitGap;
    const float displayDigitX = 804.0f - displayDigitsWidth;
    for (int i = 0; i < displayDigits.length(); ++i)
        drawDisplayDigit (displayDigits.substring (i, i + 1).getIntValue(), displayDigitX + i * (displayDigitW + displayDigitGap), 222.0f, displayDigitW, displayDigitH, 6.0f);

    g.setColour (juce::Colour (0xffff6508));
    juce::Path unitM;
    unitM.startNewSubPath (821.0f, 281.0f); unitM.lineTo (821.0f, 239.0f);
    unitM.lineTo (831.0f, 256.0f); unitM.lineTo (841.0f, 239.0f); unitM.lineTo (841.0f, 281.0f);
    g.strokePath (unitM, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    drawDisplayDigit (5, 849.0f, 239.0f, 27.0f, 42.0f, 4.0f);

    // Exact min/mid/max scale numbers from Figma frame 09 / Knob Scale Numbers.
    auto scaleText = [&g] (const juce::String& t, float x, float y, float w, float h)
    {
        g.setColour (juce::Colour (0xff8f8f88));
        g.setFont (juce::FontOptions ("Roboto Condensed", 10.0f, juce::Font::plain));
        const float safeWidth = juce::jmax (w, (float) t.length() * 6.0f + 4.0f);
        const float safeX = x + (w - safeWidth) * 0.5f;
        g.drawText (t, juce::roundToInt (safeX), juce::roundToInt (y),
                    juce::roundToInt (safeWidth), juce::roundToInt (h),
                    juce::Justification::centred, false);
    };
    auto centredScaleText = [&scaleText] (const juce::String& t, float centreX, float y, float w, float h)
    {
        scaleText (t, centreX - w * 0.5f, y, w, h);
    };
    auto knobCentreX = [] (float knobX)
    {
        return knobX + productionKnobW * productionKnobVisualCentreX;
    };
    auto knobScale = [&centredScaleText, &knobCentreX] (float knobX, const juce::String& top,
                                                       const juce::String& low, const juce::String& high,
                                                       float topY, float bottomY,
                                                       float leftOffset = 39.0f, float rightOffset = 39.0f)
    {
        const float centreX = knobCentreX (knobX);
        centredScaleText (top,  centreX, topY,    top.length()  > 2 ? 18.0f : 12.0f, 10.0f);
        centredScaleText (low,  centreX - leftOffset,  bottomY, low.length()  > 2 ? 18.0f : 12.0f, 10.0f);
        centredScaleText (high, centreX + rightOffset, bottomY, high.length() > 2 ? 22.0f : 12.0f, 10.0f);
    };

    /* Micro scale labels are intentionally omitted in the final GUI: the
       surrounding Figma module artwork already provides the visual hierarchy. */
    /*
    knobScale (255.2715f,  "0",    "-12", "+12", 147.0f, 230.0f, 36.0f, 36.0f);
    knobScale (1183.7000f, "0",    "-12", "+12", 150.5f, 233.5f, 36.0f, 36.0f);
    knobScale (255.2715f,  "1000", "1",   "2000",421.5f, 498.5f, 42.0f, 42.0f);
    knobScale (509.8333f,  "48",   "0",   "95",  421.5f, 498.5f, 39.0f, 39.0f);
    knobScale (637.0716f,  "50",   "0",   "100", 421.5f, 498.5f, 39.0f, 39.0f);
    knobScale (840.4500f,  "500",  "20",  "2k",  418.4f, 501.4f, 39.0f, 39.0f);
    knobScale (1007.9500f, "10k",  "1k",  "20k", 418.4f, 501.4f, 39.0f, 39.0f);
    knobScale (1183.7000f, "100",  "0",   "200", 418.4f, 501.4f, 39.0f, 39.0f);
    knobScale (255.2715f,  "50",   "0",   "100", 648.3f, 731.3f, 39.0f, 39.0f);
    knobScale (382.5667f,  "1",    "0.1", "10",  648.3f, 731.3f, 39.0f, 39.0f);
    knobScale (509.8333f,  "50",   "0",   "100", 648.3f, 731.3f, 39.0f, 39.0f);
    knobScale (840.5000f,  "50",   "0",   "100", 648.3f, 731.3f, 39.0f, 39.0f);
    knobScale (1008.0000f, "50",   "0",   "100", 648.3f, 731.3f, 39.0f, 39.0f);
    knobScale (1183.7000f, "50",   "0",   "100", 648.3f, 731.3f, 39.0f, 39.0f);
    */

    // Keep Figma's exact selector/button artwork authoritative, and only repaint
    // the small state/value areas that genuinely change at runtime.
    auto uiText = [&g] (const juce::String& t, juce::Rectangle<float> r, float size, juce::Colour colour, juce::Justification just)
    {
        g.setColour (colour);
        g.setFont (juce::FontOptions ("Roboto Condensed", size, juce::Font::plain));
        g.drawText (t, r.getSmallestIntegerContainer(), just);
    };

    const auto presetNumber = presetIdentityValid ? juce::String (currentPresetIndex + 1).paddedLeft ('0', 2) : "--";
    const auto presetName = presetIdentityValid && ! presetNames.empty()
        ? presetNames[(size_t) currentPresetIndex]
        : presetNameLabel.getText();
    uiText (presetNumber, { 153.0f, 905.0f, 71.0f, 42.0f }, 18.0f, juce::Colour (orange), juce::Justification::centred);
    uiText (presetName, { 258.0f, 905.0f, 388.0f, 42.0f }, 18.0f, juce::Colour (orange), juce::Justification::centredLeft);

    auto hover = [] (const juce::Component& c) { return c.isMouseOver (true); };
    auto pressed = [] (const juce::Component& c) { return c.isMouseButtonDown(); };
    auto drawSurface = [&g] (juce::Rectangle<float> r, float radius, bool isHover, bool isPressed, bool active)
    {
        const auto top = isPressed ? juce::Colour (0xff17100c)
                                   : (isHover ? juce::Colour (0xff34261f) : juce::Colour (0xff232321));
        const auto bottom = active ? juce::Colour (0xff21120c) : juce::Colour (0xff090a09);
        g.setGradientFill ({ top, r.getCentreX(), r.getY(), bottom, r.getCentreX(), r.getBottom(), false });
        g.fillRoundedRectangle (r, radius);
        g.setColour (active ? juce::Colour (orange) : juce::Colour (isHover ? 0xff777a73 : 0xff525257));
        g.drawRoundedRectangle (r, radius, active || isHover ? 1.25f : 1.0f);
        g.setColour (isHover ? juce::Colour (0x30ff6105) : juce::Colour (0x1fff6105));
        g.drawHorizontalLine (juce::roundToInt (r.getY() + 1.0f), r.getX() + radius, r.getRight() - radius);
    };
    auto drawDownArrow = [&g] (float x, float y, juce::Colour colour)
    {
        juce::Path arrow;
        arrow.addTriangle (x - 4.0f, y - 2.5f, x + 4.0f, y - 2.5f, x, y + 3.5f);
        g.setColour (colour); g.fillPath (arrow);
    };

    // Delay toggles and selectors are fully redrawn so their material and hover
    // state never depend on baked preview text.
    const auto syncRect = juce::Rectangle<float> (414.37f, 416.0f, 30.0f, 30.0f);
    drawSurface (syncRect, 4.0f, hover (syncButton), pressed (syncButton), syncButton.getToggleState());
    g.setColour (syncButton.getToggleState() ? juce::Colour (orange) : juce::Colour (0xff777872));
    g.fillEllipse (syncRect.reduced (8.0f));

    const auto divisionRect = juce::Rectangle<float> (394.37f, 451.0f, 71.0f, 36.0f);
    drawSurface (divisionRect, 5.0f, hover (divisionBox), pressed (divisionBox), false);
    uiText (divisionBox.getText().toUpperCase(), divisionRect.reduced (5.0f, 0.0f).withTrimmedRight (7.0f),
            15.0f, juce::Colour (0xffdeded9), juce::Justification::centred);
    drawDownArrow (divisionRect.getRight() - 13.0f, divisionRect.getCentreY(), juce::Colour (orange));

    const auto pingRect = juce::Rectangle<float> (383.37f, 490.0f, 92.0f, 34.0f);
    drawSurface (pingRect, 5.0f, hover (pingPongButton), pressed (pingPongButton), pingPongButton.getToggleState());
    uiText ("PING-PONG", pingRect.reduced (3.0f, 0.0f), 14.0f,
            pingPongButton.getToggleState() ? juce::Colour (orange) : juce::Colour (0xff9e9e99), juce::Justification::centred);

    const auto shapeRect = juce::Rectangle<float> (638.0f, 683.0f, 88.0f, 38.0f);
    drawSurface (shapeRect, 5.0f, hover (shapeBox), pressed (shapeBox), false);
    uiText (shapeBox.getText().toUpperCase(), shapeRect.reduced (11.0f, 0.0f).withTrimmedRight (13.0f),
            15.0f, juce::Colour (orange), juce::Justification::centred);
    drawDownArrow (shapeRect.getRight() - 13.0f, shapeRect.getCentreY(), juce::Colour (orange));

    // MODE: complete 215x52 housing, 121x50 value field and independent arrows.
    const auto modeRect = juce::Rectangle<float> (138.0f, 812.0f, 215.0f, 52.0f);
    const auto modeValue = juce::Rectangle<float> (186.0f, 813.0f, 121.0f, 50.0f);
    drawSurface (modeRect, 6.0f, hover (modeCycleButton), pressed (modeCycleButton), false);
    g.setGradientFill ({ juce::Colour (hover (modeCycleButton) ? 0xff181918 : 0xff111211), modeValue.getCentreX(), modeValue.getY(),
                         juce::Colour (0xff070807), modeValue.getCentreX(), modeValue.getBottom(), false });
    g.fillRect (modeValue);
    g.setColour (juce::Colour (0xff353833));
    g.drawVerticalLine ((int) modeValue.getX(), modeValue.getY() + 4.0f, modeValue.getBottom() - 4.0f);
    g.drawVerticalLine ((int) modeValue.getRight(), modeValue.getY() + 4.0f, modeValue.getBottom() - 4.0f);
    uiText (modeBox.getText().toUpperCase(), modeValue, 18.0f, juce::Colour (orange), juce::Justification::centred);
    juce::Path modePrevious, modeNext;
    modePrevious.addTriangle (154.0f, 838.0f, 170.0f, 828.0f, 170.0f, 848.0f);
    modeNext.addTriangle (337.0f, 838.0f, 321.0f, 828.0f, 321.0f, 848.0f);
    g.setColour (juce::Colour (0xffd0d0cc));
    g.fillPath (modePrevious); g.fillPath (modeNext);
    if (hover (modeCycleButton) || pressed (modeCycleButton))
    {
        g.setColour (pressed (modeCycleButton) ? juce::Colour (orange) : juce::Colour (orangeHi));
        g.drawRoundedRectangle (modeRect.reduced (0.75f), 6.0f, 1.0f);
    }

    // QUALITY uses one material knob and one illuminated position only. The
    // clean Figma Utility export contains the labels and track but no baked
    // knob, so the live control can be rendered without any masking patch.
    const std::array<float, 3> qualityX { 425.0f, 487.5f, 552.0f };
    const int qIndex = juce::jlimit (0, 2, juce::roundToInt (qualitySlider.getValue()));
    const bool qualityHover = hover (qualitySlider);
    const auto qualityKnob = juce::Rectangle<float> (18.0f, 18.0f)
                                 .withCentre ({ qualityX[(size_t) qIndex], 841.0f });
    g.setColour (juce::Colour (0xff191a18));
    g.fillEllipse (qualityKnob);
    g.setColour (qualityHover ? juce::Colour (orangeHi) : juce::Colour (0xff656760));
    g.drawEllipse (qualityKnob, qualityHover ? 1.5f : 1.0f);
    g.setColour (juce::Colour (orange));
    g.fillEllipse (qualityKnob.reduced (6.0f));

    // Oversampling, Freeze and Bypass use complete housings, centred glyphs and
    // honest active/hover/pressed states.
    const auto oversamplingRect = juce::Rectangle<float> (1093.0f, 810.0f, 56.0f, 56.0f);
    g.setColour (juce::Colour (0xff0a0b0a));
    g.fillRect (oversamplingRect);
    drawSurface (oversamplingRect, 7.0f, hover (oversamplingCycleButton), pressed (oversamplingCycleButton), false);
    uiText (oversamplingBox.getText().toUpperCase(), oversamplingRect,
            oversamplingBox.getText().length() > 2 ? 15.0f : 20.0f, juce::Colour (orange), juce::Justification::centred);

    // Freeze and Bypass icons: use the exact exported SVG glyphs, tinting only
    // the runtime state. This keeps shape fidelity while making Off/On honest.
    auto drawTintedIcon = [&g] (const std::unique_ptr<juce::Drawable>& source, juce::Rectangle<float> area, bool active)
    {
        if (source == nullptr) return;
        auto copy = source->createCopy();
        if (! active)
        {
            copy->replaceColour (juce::Colour (0xffff6508), juce::Colour (0xff8c8c87));
            copy->replaceColour (juce::Colour (0xffff6308), juce::Colour (0xff8c8c87));
            copy->replaceColour (juce::Colour (0xffff4f0b), juce::Colour (0xff8c8c87));
            copy->replaceColour (juce::Colour (0xfff28c18), juce::Colour (0xff8c8c87));
        }
        copy->drawWithin (g, area, juce::RectanglePlacement::stretchToFit, active ? 1.0f : 0.88f);
    };
    const auto freezeRect = juce::Rectangle<float> (735.0f, 810.0f, 56.0f, 56.0f);
    g.setColour (juce::Colour (0xff080908));
    g.fillRect (freezeRect);
    drawSurface (freezeRect, 7.0f, hover (freezeButton), pressed (freezeButton), freezeButton.getToggleState());
    drawTintedIcon (freezeIcon, freezeRect.withSizeKeepingCentre (28.0f, 28.0f), freezeButton.getToggleState());

    const auto bypassRect = juce::Rectangle<float> (1243.0f, 810.0f, 56.0f, 56.0f);
    g.setColour (juce::Colour (0xff0c0d0c));
    g.fillRect (bypassRect);
    drawSurface (bypassRect, 7.0f, hover (bypassButton), pressed (bypassButton), bypassButton.getToggleState());
    drawTintedIcon (bypassIcon, bypassRect.withSizeKeepingCentre (24.0f, 24.0f), ! bypassButton.getToggleState());

    // The clean preset export keeps the exact A/B housing and divider; only
    // the two state-dependent labels are rendered live.
    uiText ("A", { 903.0f, 905.0f, 30.0f, 42.0f }, 14.0f, activeSnapshotSlot == 0 ? juce::Colour (orange) : juce::Colour (0xff9e9e99), juce::Justification::centred);
    uiText ("B", { 933.0f, 905.0f, 30.0f, 42.0f }, 14.0f, activeSnapshotSlot == 1 ? juce::Colour (orange) : juce::Colour (0xff9e9e99), juce::Justification::centred);

    auto drawHoverOutline = [&g, &hover, &pressed] (const juce::Component& component, juce::Rectangle<float> r, float radius)
    {
        if (! hover (component) && ! pressed (component)) return;
        g.setColour (pressed (component) ? juce::Colour (0x38ff6105) : juce::Colour (0x20ff8a42));
        g.fillRoundedRectangle (r.reduced (1.0f), juce::jmax (1.0f, radius - 1.0f));
        g.setColour (pressed (component) ? juce::Colour (orange) : juce::Colour (orangeHi));
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
    };
    drawHoverOutline (previousPresetButton, { 668, 905, 45, 42 }, 5.0f);
    drawHoverOutline (nextPresetButton,     { 713, 905, 45, 42 }, 5.0f);
    drawHoverOutline (saveAsButton,         { 778, 905, 110, 42 }, 5.0f);
    drawHoverOutline (aButton,              { 903, 905, 30, 42 }, 4.0f);
    drawHoverOutline (bButton,              { 933, 905, 30, 42 }, 4.0f);
    drawHoverOutline (undoRedoButton,       { 978, 905, 100, 42 }, 5.0f);
    drawHoverOutline (diceButton,           { 1093, 905, 42, 42 }, 5.0f);
    drawHoverOutline (menuButton,           { 1443, 898, 56, 56 }, 7.0f);

    // Live meter LEDs. The raster was converted to a real No-Signal base when
    // loaded, so this is now the only illuminated signal layer.
    auto drawLiveMeter = [&g] (juce::Rectangle<int> module, const std::array<float, 2>& levels)
    {
        constexpr int segments = 20;
        constexpr float floorDb = -60.0f;
        constexpr float dbPerSegment = 3.0f;
        for (int c = 0; c < 2; ++c)
        {
            const float linear = juce::jlimit (0.0f, 4.0f, levels[(size_t) c]);
            const float db = juce::Decibels::gainToDecibels (linear, floorDb);
            const int lit = db <= floorDb ? 0
                                          : juce::jlimit (1, segments,
                                                          (int) std::ceil ((db - floorDb) / dbPerSegment));
            const float x = (float) module.getX() + (c == 0 ? 60.8f : 99.8f);
            for (int i = 0; i < segments; ++i)
            {
                const float y = (float) module.getY() + 86.8f + i * 25.0f;
                const int fromBottom = segments - 1 - i;
                const bool on = fromBottom < lit;
                if (! on)
                    continue;

                juce::Colour col = i < 3 ? juce::Colour (0xffe1221a)
                                  : i < 7 ? juce::Colour (0xfff28c18)
                                          : juce::Colour (0xff39b51c);

                const auto cell = juce::Rectangle<float> (x, y, 14.0f, 17.0f);
                g.setColour (col);
                g.fillRoundedRectangle (cell, 2.5f);
                g.setColour (juce::Colour (0xe6030304));
                g.drawRoundedRectangle (cell, 2.5f, 0.8f);
                g.setColour (col.withAlpha (0.24f));
                g.drawRoundedRectangle (cell.expanded (1.0f), 3.0f, 1.0f);
            }

            if (db >= -0.1f)
            {
                g.setColour (juce::Colour (0xffe1221a));
                g.fillRoundedRectangle ({ (float) module.getX() + 58.8f,
                                          (float) module.getY() + 70.8f,
                                          54.0f, 3.0f }, 2.0f);
            }
        }
    };
    drawLiveMeter (L.inputMeter, inputMeterLevels);
    drawLiveMeter (L.outputMeter, outputMeterLevels);
}

void AuroraD80AudioProcessorEditor::resized()
{
    const float ui = editorScale;
    designSurface.setBounds (getLocalBounds());
    designSurface.setTransform (juce::AffineTransform());

    auto R = [this] (float x, float y, float w, float h)
    {
        return juce::Rectangle<int> (juce::roundToInt ((x + masterCentreCorrectionX) * editorScale),
                                     juce::roundToInt (y * editorScale),
                                     juce::roundToInt (w * editorScale),
                                     juce::roundToInt (h * editorScale));
    };

    // Applied knobs are children of Figma frame 6512:6748 at (+4,-6).
    // Use their ROOT-space positions, then centre labels/values on the actual
    // knob centre. This fixes the previous systematic 8 px horizontal / 12 px
    // vertical drift relative to the module artwork nested at (-4,+6).
    auto placeKnobCentred = [&R, ui, this] (juce::Slider& slider, juce::Label& name, juce::Label& value,
                                      float x, float y, float textCentreX, float labelY, float valueY,
                                      float labelWidth = 116.0f, float valueWidth = 108.0f,
                                      float labelSize = 15.0f, float valueSize = 16.0f)
    {
        slider.setBounds (R (x, y, productionKnobW, productionKnobH));
        name.setBounds (R (textCentreX - labelWidth * 0.5f, labelY, labelWidth, 20.0f));
        value.setBounds (R (textCentreX - valueWidth * 0.5f, valueY, valueWidth, 18.0f));
        name.setFont (breeSerifTypeface != nullptr
                          ? juce::Font (juce::FontOptions (breeSerifTypeface).withHeight (labelSize * ui))
                          : juce::Font (juce::FontOptions ("Georgia", labelSize * ui, juce::Font::plain)));
        value.setFont (arimoTypeface != nullptr
                           ? juce::Font (juce::FontOptions (arimoTypeface).withHeight (valueSize * ui))
                           : juce::Font (juce::FontOptions ("Arial", valueSize * ui, juce::Font::plain)));
    };

    // Exact root coordinates from Applied Knob frame 6693:10137 and text
    // centres from Knob Scale Numbers frame 6693:10153.
    placeKnobCentred (inputSlider, inputLabel, inputValue,
                      255.2715f, 142.5f, 301.0f, 132.0f, 254.0f, 90.0f, 90.0f);
    placeKnobCentred (outputSlider, outputLabel, outputValue,
                      1183.7000f, 146.0f, 1229.0f, 132.0f, 254.0f, 104.0f, 90.0f);

    placeKnobCentred (timeSlider, timeLabel, timeValue,
                      255.2715f, 410.8f, 303.14f, 407.5f, 513.5f, 72.0f, 92.0f, 16.0f, 16.0f);
    placeKnobCentred (feedbackSlider, feedbackLabel, feedbackValue,
                      509.8333f, 410.8f, 555.70f, 409.5f, 513.5f, 96.0f, 78.0f);
    placeKnobCentred (mixSlider, mixLabel, mixValue,
                      637.0716f, 410.8f, 683.94f, 409.5f, 513.5f, 64.0f, 78.0f);

    placeKnobCentred (lowCutSlider, lowCutLabel, lowCutValue,
                      840.4500f, 412.9f, 886.32f, 411.6f, 515.6f, 92.0f, 96.0f);
    placeKnobCentred (highCutSlider, highCutLabel, highCutValue,
                      1007.9500f, 412.9f, 1053.82f, 411.6f, 515.6f, 98.0f, 116.0f);
    placeKnobCentred (widthSlider, widthLabel, widthValue,
                      1183.7000f, 412.9f, 1228.57f, 411.6f, 515.6f, 84.0f, 92.0f);

    placeKnobCentred (depthSlider, depthLabel, depthValue,
                      255.2715f, 643.5808f, 301.14f, 642.28f, 746.28f, 82.0f, 76.0f);
    placeKnobCentred (rateSlider, rateLabel, rateValue,
                      382.5667f, 643.58f, 428.44f, 642.28f, 746.28f, 72.0f, 100.0f);
    placeKnobCentred (driftSlider, driftLabel, driftValue,
                      509.8333f, 643.58f, 555.70f, 642.28f, 746.28f, 76.0f, 76.0f);
    placeKnobCentred (driveSlider, driveLabel, driveValue,
                      840.5000f, 643.58f, 886.37f, 642.28f, 746.28f, 78.0f, 76.0f);
    placeKnobCentred (evolveSlider, evolveLabel, evolveValue,
                      1008.0000f, 643.58f, 1053.87f, 642.28f, 746.28f, 88.0f, 76.0f);
    placeKnobCentred (bloomSlider, bloomLabel, bloomValue,
                      1183.7000f, 643.58f, 1228.57f, 642.28f, 746.28f, 84.0f, 76.0f);

    // Delay / Shape hit areas use the production module's true root coordinates.
    syncButton.setBounds    (R (414.37f, 416.0f, 30, 30));
    divisionBox.setBounds   (R (394.37f, 451.0f, 71, 36));
    pingPongButton.setBounds(R (383.37f, 490.0f, 92, 34));
    shapeBox.setBounds      (R (638.0f, 683.0f, 88, 38));
    divisionBox.setInterceptsMouseClicks (false, false);
    shapeBox.setInterceptsMouseClicks (false, false);

    for (auto* c : { static_cast<juce::Component*>(&syncButton), static_cast<juce::Component*>(&pingPongButton),
                     static_cast<juce::Component*>(&divisionBox), static_cast<juce::Component*>(&shapeBox) })
        c->setAlpha (0.01f);

    // Utility Bar v1.4 root coordinates and hit targets.
    modeBox.setBounds         (R (138, 812, 215, 52));
    modeBox.setInterceptsMouseClicks (false, false);
    modeCycleButton.setBounds (R (138, 812, 215, 52));
    qualitySlider.setBounds   (R (410, 815, 155, 52));
    freezeButton.setBounds    (R (735, 810, 56, 56));
    oversamplingBox.setBounds (R (1093, 810, 56, 56));
    oversamplingBox.setInterceptsMouseClicks (false, false);
    oversamplingCycleButton.setBounds (R (1093, 810, 56, 56));
    bypassButton.setBounds    (R (1243, 810, 56, 56));

    // Preset Browser v1.4.
    presetNumberLabel.setVisible (false);
    presetNameLabel.setVisible (false);
    presetNumberLabel.setBounds (R (153, 905, 71, 42));
    presetNameLabel.setBounds   (R (258, 905, 388, 42));
    presetDisplayButton.setBounds (R (153, 905, 500, 42));
    presetDisplayButton.toFront (false);
    favouriteButton.setVisible (false);
    previousPresetButton.setBounds (R (668, 905, 45, 42));
    nextPresetButton.setBounds     (R (713, 905, 45, 42));
    saveAsButton.setBounds         (R (778, 905, 110, 42));
    loadButton.setVisible (false);
    aButton.setBounds    (R (903, 905, 30, 42));
    bButton.setBounds    (R (933, 905, 30, 42));
    undoRedoButton.setBounds (R (978, 905, 100, 42));
    diceButton.setBounds (R (1093, 905, 42, 42));
    menuButton.setBounds (R (1443, 898, 56, 56));

    for (auto* c : { static_cast<juce::Component*>(&modeBox), static_cast<juce::Component*>(&modeCycleButton),
                     static_cast<juce::Component*>(&qualitySlider), static_cast<juce::Component*>(&oversamplingBox),
                     static_cast<juce::Component*>(&oversamplingCycleButton), static_cast<juce::Component*>(&freezeButton),
                     static_cast<juce::Component*>(&bypassButton), static_cast<juce::Component*>(&previousPresetButton),
                     static_cast<juce::Component*>(&nextPresetButton), static_cast<juce::Component*>(&saveAsButton),
                     static_cast<juce::Component*>(&aButton), static_cast<juce::Component*>(&bButton), static_cast<juce::Component*>(&undoRedoButton),
                     static_cast<juce::Component*>(&diceButton), static_cast<juce::Component*>(&menuButton), static_cast<juce::Component*>(&favouriteButton) })
        c->setAlpha (0.0f);

    const auto presetNumberFont = arimoTypeface != nullptr
        ? juce::Font (juce::FontOptions (arimoTypeface).withHeight (26.0f * ui))
        : juce::Font (juce::FontOptions ("Arial", 26.0f * ui, juce::Font::plain));
    const auto presetNameFont = arimoTypeface != nullptr
        ? juce::Font (juce::FontOptions (arimoTypeface).withHeight (18.0f * ui))
        : juce::Font (juce::FontOptions ("Arial", 18.0f * ui, juce::Font::plain));
    presetNumberLabel.setFont (presetNumberFont);
    presetNameLabel.setFont   (presetNameFont);
}
