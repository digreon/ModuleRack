#include "ModuleSlot.h"

namespace modulerack
{

std::unique_ptr<juce::XmlElement> ModuleSlot::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement>("Module");
    xml->setAttribute("id", id);

    for (const auto& param : const_cast<ModuleSlot*>(this)->getParams())
    {
        auto* paramXml = xml->createNewChildElement("Param");
        paramXml->setAttribute("id", param.id);
        paramXml->setAttribute("value", (double) param.get());
    }

    return xml;
}

void ModuleSlot::fromXml(const juce::XmlElement& xml)
{
    auto& params = getParams();

    for (auto* paramXml : xml.getChildIterator())
    {
        if (! paramXml->hasTagName("Param"))
            continue;

        const auto paramId = paramXml->getStringAttribute("id");
        const float value = (float) paramXml->getDoubleAttribute("value");

        for (auto& param : params)
        {
            if (param.id == paramId)
            {
                param.set(value);
                break;
            }
        }
    }
}

} // namespace modulerack
