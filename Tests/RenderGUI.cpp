#include "../Source/PluginEditor.h"

int main (int argc, char** argv)
{
    if (argc < 2 || argc > 3)
        return 2;

    juce::MessageManager::getInstance();
    bool ok = false;
    {
        AuroraD80AudioProcessor processor;
        if (argc == 3 && juce::String (argv[2]) == "max")
        {
            auto set = [&processor] (const char* id, float value)
            {
                if (auto* parameter = processor.parameters.getParameter (id))
                    parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
            };
            set ("delayTimeMs", 2000.0f);
            set ("feedback", 0.95f);
            set ("mix", 1.0f);
            set ("lowCutHz", 2000.0f);
            set ("highCutHz", 20000.0f);
            set ("width", 200.0f);
            set ("modulationDepth", 100.0f);
            set ("modulationRate", 10.0f);
            set ("drift", 100.0f);
            set ("drive", 100.0f);
            set ("vintage", 100.0f);
            set ("bloom", 100.0f);
        }
        AuroraD80AudioProcessorEditor editor (processor);
        editor.resized();

        auto image = editor.createComponentSnapshot (editor.getLocalBounds(), true, 1.0f);
        auto stream = juce::File (argv[1]).createOutputStream();
        ok = stream != nullptr && juce::PNGImageFormat().writeImageToStream (image, *stream);
    }
    juce::MessageManager::deleteInstance();
    return ok ? 0 : 1;
}
