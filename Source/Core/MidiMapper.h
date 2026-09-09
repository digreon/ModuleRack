#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <memory>
#include "ModuleSlot.h"

namespace modulerack
{

/**
 * The control-surface mapping: a pad note-on selects a module (the on-screen
 * panel + the 8 knobs point at it); a knob CC updates whichever module is
 * currently selected. Pads never start/stop sound in the MVP -- all 8 modules
 * are always generating audio from the shared Clock (see SignalGraph).
 *
 * The note/CC numbers live in a Profile rather than being compiled in, so a
 * different controller (or an MPK Mini whose program has been edited in MPK
 * Mini Editor) is a matter of changing the numbers, at runtime, and they travel
 * with the saved patch. Everything else in the app is written against
 * ModuleSlot / module index and never against MIDI numbers.
 */
class MidiMapper
{
public:
    static constexpr int numSlots = 8;

    struct Profile
    {
        juce::String name;
        std::array<int, numSlots> padNotes {};
        std::array<int, numSlots> knobCcs {};
    };

    /** MPK Mini factory program: pads bank A from C1, knobs K1-K8 on CC 70-77.
        Check yours in MPK Mini Editor -- pad/knob assignments are user-editable
        on the device and differ between generations. */
    static Profile mpkMiniProfile()
    {
        return { "MPK Mini (pads 36-43, knobs CC 70-77)",
                 { 36, 37, 38, 39, 40, 41, 42, 43 },
                 { 70, 71, 72, 73, 74, 75, 76, 77 } };
    }

    /** Fallback for controllers whose knobs send the low CC numbers. */
    static Profile genericProfile()
    {
        return { "Generic (pads 36-43, knobs CC 1-8)",
                 { 36, 37, 38, 39, 40, 41, 42, 43 },
                 { 1, 2, 3, 4, 5, 6, 7, 8 } };
    }

    MidiMapper() { setProfile(mpkMiniProfile()); }

    /** Scans incoming MIDI for pad-select notes and knob CCs and applies them to `modules`. */
    void processIncomingMidi(const juce::MidiBuffer& midiIn,
                             std::array<std::unique_ptr<ModuleSlot>, numSlots>& modules);

    void setProfile(const Profile& profile);
    juce::String getProfileName() const { return profileName; }

    int getPadNote(int slot) const { return padNotes[(size_t) juce::jlimit(0, numSlots - 1, slot)].load(std::memory_order_relaxed); }
    int getKnobCc(int knob) const { return knobCcs[(size_t) juce::jlimit(0, numSlots - 1, knob)].load(std::memory_order_relaxed); }

    void setPadNote(int slot, int noteNumber);
    void setKnobCc(int knob, int ccNumber);

    int getSelectedModuleIndex() const { return selectedModuleIndex.load(std::memory_order_relaxed); }
    void setSelectedModuleIndex(int index)
    {
        selectedModuleIndex.store(juce::jlimit(0, numSlots - 1, index), std::memory_order_relaxed);
    }

    /** The mapping is saved with the patch, so a remapped controller survives a reload. */
    std::unique_ptr<juce::XmlElement> toXml() const;
    void fromXml(const juce::XmlElement& xml);

private:
    juce::String profileName;
    std::array<std::atomic<int>, numSlots> padNotes {};
    std::array<std::atomic<int>, numSlots> knobCcs {};
    std::atomic<int> selectedModuleIndex { 0 };
};

} // namespace modulerack
