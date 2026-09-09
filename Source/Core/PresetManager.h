#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include "ModuleSlot.h"
#include "MidiMapper.h"
#include "Clock.h"

namespace modulerack
{

/**
 * Saves/loads a full "patch" -- the state of all 8 modules, the tempo and the
 * controller mapping -- as a single human-readable XML file. XML (rather than
 * binary) was chosen so presets are easy to diff/inspect and to hand-edit while
 * designing modules.
 */
class PresetManager
{
public:
    using Modules = std::array<std::unique_ptr<ModuleSlot>, 8>;

    static juce::File getPresetsDirectory();

    /** The file a patch of this name is stored in, with illegal characters stripped.
        Takes the directory explicitly so it can be tested without writing into the
        real preset folder. */
    static juce::File fileForPresetName(const juce::File& directory, const juce::String& name);

    /** Every patch in `directory`, ordered by name the way a person reads a list. */
    static juce::Array<juce::File> getPresetFiles(const juce::File& directory);

    static std::unique_ptr<juce::XmlElement> toXml(const Modules& modules, const Clock& clock, const MidiMapper& mapper);
    static void fromXml(const juce::XmlElement& xml, Modules& modules, Clock& clock, MidiMapper& mapper);

    static bool saveToFile(const juce::File& file, const Modules& modules, const Clock& clock, const MidiMapper& mapper);
    static bool loadFromFile(const juce::File& file, Modules& modules, Clock& clock, MidiMapper& mapper);
};

} // namespace modulerack
