#include "MidiMapper.h"

namespace modulerack
{

void MidiMapper::setProfile(const Profile& profile)
{
    profileName = profile.name;

    for (int i = 0; i < numSlots; ++i)
    {
        padNotes[(size_t) i].store(profile.padNotes[(size_t) i], std::memory_order_relaxed);
        knobCcs[(size_t) i].store(profile.knobCcs[(size_t) i], std::memory_order_relaxed);
    }
}

void MidiMapper::setPadNote(int slot, int noteNumber)
{
    if (juce::isPositiveAndBelow(slot, numSlots))
        padNotes[(size_t) slot].store(juce::jlimit(0, 127, noteNumber), std::memory_order_relaxed);
}

void MidiMapper::setKnobCc(int knob, int ccNumber)
{
    if (juce::isPositiveAndBelow(knob, numSlots))
        knobCcs[(size_t) knob].store(juce::jlimit(0, 127, ccNumber), std::memory_order_relaxed);
}

void MidiMapper::processIncomingMidi(const juce::MidiBuffer& midiIn,
                                     std::array<std::unique_ptr<ModuleSlot>, numSlots>& modules)
{
    for (const auto metadata : midiIn)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            const int note = message.getNoteNumber();

            for (int i = 0; i < numSlots; ++i)
            {
                if (getPadNote(i) == note)
                {
                    setSelectedModuleIndex(i);
                    break;
                }
            }
        }
        else if (message.isController())
        {
            const int cc = message.getControllerNumber();

            for (int i = 0; i < numSlots; ++i)
            {
                if (getKnobCc(i) == cc)
                {
                    const float value01 = (float) message.getControllerValue() / 127.0f;
                    modules[(size_t) getSelectedModuleIndex()]->setParamNormalised(i, value01);
                    break;
                }
            }
        }
    }
}

std::unique_ptr<juce::XmlElement> MidiMapper::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement>("MidiMapping");
    xml->setAttribute("profile", profileName);

    for (int i = 0; i < numSlots; ++i)
    {
        auto* slotXml = xml->createNewChildElement("Slot");
        slotXml->setAttribute("index", i);
        slotXml->setAttribute("padNote", getPadNote(i));
        slotXml->setAttribute("knobCc", getKnobCc(i));
    }

    return xml;
}

void MidiMapper::fromXml(const juce::XmlElement& xml)
{
    if (! xml.hasTagName("MidiMapping"))
        return;

    profileName = xml.getStringAttribute("profile", profileName);

    for (auto* slotXml : xml.getChildIterator())
    {
        if (! slotXml->hasTagName("Slot"))
            continue;

        const int index = slotXml->getIntAttribute("index", -1);
        if (! juce::isPositiveAndBelow(index, numSlots))
            continue;

        setPadNote(index, slotXml->getIntAttribute("padNote", getPadNote(index)));
        setKnobCc(index, slotXml->getIntAttribute("knobCc", getKnobCc(index)));
    }
}

} // namespace modulerack
