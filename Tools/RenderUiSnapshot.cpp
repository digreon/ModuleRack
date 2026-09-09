/*
    Renders the plugin editor straight into PNG files, with no window and no
    display server -- a way to look at the interface, and to see what a layout
    change did, from a machine that can't run the plugin.

        ModuleRackUiSnapshot [output directory]
*/

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <iostream>

#include "PluginEditor.h"
#include "PluginProcessor.h"

namespace
{

/** Finds a button by its label, so a snapshot can show a view that is normally
    a tap away without widening the editor's interface just for this tool. */
juce::Button* findButton(juce::Component& parent, const juce::String& text)
{
    for (auto* child : parent.getChildren())
    {
        if (auto* button = dynamic_cast<juce::Button*>(child))
            if (button->getButtonText() == text)
                return button;

        if (auto* found = findButton(*child, text))
            return found;
    }

    return nullptr;
}

bool writeSnapshot(const juce::File& file, juce::Component& component)
{
    juce::Image image(juce::Image::ARGB, component.getWidth(), component.getHeight(), true);

    {
        juce::Graphics g(image);
        component.paintEntireComponent(g, true);
    }

    file.deleteFile();
    std::unique_ptr<juce::FileOutputStream> stream(file.createOutputStream().release());

    if (stream == nullptr)
        return false;

    juce::PNGImageFormat png;

    if (! png.writeImageToStream(image, *stream))
        return false;

    std::cout << "wrote " << file.getFullPathName() << std::endl;
    return true;
}

} // namespace

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    const juce::File outputDirectory = argc > 1
        ? juce::File::getCurrentWorkingDirectory().getChildFile(argv[1])
        : juce::File::getCurrentWorkingDirectory();
    outputDirectory.createDirectory();

    struct Shot
    {
        const char* fileName;
        int width, height;
        int selectedModule;
        bool showPresets;
    };

    const Shot shots[]
    {
        { "ui-1-kick.png",     900, 560, 0, false },
        { "ui-2-arp.png",      900, 560, 4, false },
        { "ui-3-presets.png",  900, 560, 0, true  },
        { "ui-4-compact.png",  480, 340, 1, false },
    };

    for (const auto& shot : shots)
    {
        modulerack::ModuleRackProcessor processor;
        processor.prepareToPlay(48000.0, 512);

        // Set before the editor exists: the panel binds to the selected module as
        // it is built, the same way a pad press rebuilds it at runtime.
        processor.getMidiMapper().setSelectedModuleIndex(shot.selectedModule);

        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());

        if (editor == nullptr)
        {
            std::cout << "editor could not be created" << std::endl;
            return 1;
        }

        editor->setSize(shot.width, shot.height);

        if (shot.showPresets)
        {
            if (auto* presetsButton = findButton(*editor, "Presets"))
                presetsButton->setToggleState(true, juce::sendNotificationSync);
        }

        if (! writeSnapshot(outputDirectory.getChildFile(shot.fileName), *editor))
        {
            std::cout << "could not write " << shot.fileName << std::endl;
            return 1;
        }
    }

    return 0;
}
