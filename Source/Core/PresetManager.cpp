#include "PresetManager.h"

namespace modulerack
{

juce::File PresetManager::getPresetsDirectory()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("ModuleRack")
                   .getChildFile("Presets");
    dir.createDirectory();
    return dir;
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
