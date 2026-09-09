#include "PresetManager.h"

namespace modulerack
{

juce::File PresetManager::getPresetsDirectory()
{
    const auto directory = [&]
    {
       #if JUCE_IOS
        // On iOS the AUv3 extension and the app that carries it are separate
        // sandboxes: a patch saved in one is invisible to the other unless both
        // are members of the same App Group. Configure one with
        // -DMODULERACK_APP_GROUP_ID=... and both sides share this folder.
        #if defined (MODULERACK_APP_GROUP_ID)
         const auto shared = juce::File::getContainerForSecurityApplicationGroupIdentifier (MODULERACK_APP_GROUP_ID);

         if (shared != juce::File())
             return shared.getChildFile ("Presets");
        #endif

        // No App Group: fall back to this sandbox's own Documents folder, which at
        // least shows up in the Files app (FILE_SHARING_ENABLED in CMakeLists).
        return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("Presets");
       #else
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("ModuleRack")
                   .getChildFile ("Presets");
       #endif
    }();

    directory.createDirectory();
    return directory;
}

std::unique_ptr<juce::XmlElement> PresetManager::toXml(const Modules& modules, const Clock& clock, const MidiMapper& mapper)
{
    auto xml = std::make_unique<juce::XmlElement>("ModuleRackPreset");
    xml->setAttribute("version", 1);
    xml->setAttribute("bpm", (double) clock.getBpm());

    for (auto& module : modules)
        xml->addChildElement(module->toXml().release());

    xml->addChildElement(mapper.toXml().release());

    return xml;
}

void PresetManager::fromXml(const juce::XmlElement& xml, Modules& modules, Clock& clock, MidiMapper& mapper)
{
    if (! xml.hasTagName("ModuleRackPreset"))
        return;

    clock.setBpm((float) xml.getDoubleAttribute("bpm", clock.getBpm()));

    for (auto* childXml : xml.getChildIterator())
    {
        if (childXml->hasTagName("MidiMapping"))
        {
            mapper.fromXml(*childXml);
            continue;
        }

        if (! childXml->hasTagName("Module"))
            continue;

        const auto moduleId = childXml->getStringAttribute("id");

        for (auto& module : modules)
        {
            if (module->id == moduleId)
            {
                module->fromXml(*childXml);
                break;
            }
        }
    }
}

bool PresetManager::saveToFile(const juce::File& file, const Modules& modules, const Clock& clock, const MidiMapper& mapper)
{
    auto xml = toXml(modules, clock, mapper);
    return xml->writeTo(file);
}

bool PresetManager::loadFromFile(const juce::File& file, Modules& modules, Clock& clock, MidiMapper& mapper)
{
    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr)
        return false;

    fromXml(*xml, modules, clock, mapper);
    return true;
}

} // namespace modulerack
