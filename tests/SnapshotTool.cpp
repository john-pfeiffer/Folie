// Dev tool, not a test: renders the plugin editor to PNG files headlessly
// (run under xvfb-run on Linux). Lets UI layout be inspected without a DAW.
//
//   xvfb-run -a ./FolieSnapshot [outDir]
//
// Writes folie-ui-default.png (default size) and folie-ui-min.png (minimum
// size) into outDir (default: current directory).

#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const juce::File outDir = argc > 1
        ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1])
        : juce::File::getCurrentWorkingDirectory();
    outDir.createDirectory();

    FolieAudioProcessor processor;
    processor.prepareToPlay (48000.0, 512);

    struct Shot { const char* name; int w, h; };
    for (const auto& shot : { Shot { "folie-ui-default.png", 1100, 740 },
                              Shot { "folie-ui-min.png", 900, 620 } })
    {
        std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
        editor->setSize (shot.w, shot.h);

        auto image = editor->createComponentSnapshot (editor->getLocalBounds());

        const auto file = outDir.getChildFile (shot.name);
        file.deleteFile();
        juce::FileOutputStream stream (file);
        if (! stream.openedOk())
        {
            std::printf ("FAILED to open %s for writing\n", file.getFullPathName().toRawUTF8());
            return 1;
        }

        juce::PNGImageFormat png;
        png.writeImageToStream (image, stream);
        std::printf ("wrote %s (%dx%d)\n", file.getFullPathName().toRawUTF8(),
                     image.getWidth(), image.getHeight());
    }

    return 0;
}
