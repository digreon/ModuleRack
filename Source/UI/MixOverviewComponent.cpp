#include "MixOverviewComponent.h"

namespace modulerack
{

namespace
{
    /** "Fx1: Filter/Delay" -> "Fx1", "Percussion" -> "Perc": enough to tell the
        eight pads apart when each one is only a finger wide. */
    juce::String abbreviate(const juce::String& name)
    {
        auto shortened = name.upToFirstOccurrenceOf(":", false, false)
                             .upToFirstOccurrenceOf("/", false, false)
                             .trim();

        return shortened.length() > 6 ? shortened.substring(0, 4) : shortened;
    }
}

MixOverviewComponent::MixOverviewComponent(SignalGraph& graphToUse, MidiMapper& mapperToUse)
    : graph(graphToUse), mapper(mapperToUse)
{
    for (int i = 0; i < 8; ++i)
    {
        auto& button = padButtons[(size_t) i];
        button.setButtonText(graph.getModules()[(size_t) i]->name);
        button.setClickingTogglesState(false);
        button.onClick = [this, i] { mapper.setSelectedModuleIndex(i); };
        addAndMakeVisible(button);
    }

    startTimerHz(15);
    refreshSelection();
}

MixOverviewComponent::~MixOverviewComponent()
{
    stopTimer();
}

void MixOverviewComponent::resized()
{
    auto area = getLocalBounds();
    const int cellWidth = area.getWidth() / 8;
    const bool useFullNames = cellWidth >= 96;

    for (int i = 0; i < 8; ++i)
    {
        auto& button = padButtons[(size_t) i];
        const auto& name = graph.getModules()[(size_t) i]->name;
        button.setButtonText(useFullNames ? name : abbreviate(name));
        button.setBounds(area.getX() + i * cellWidth, area.getY(), cellWidth - 4, area.getHeight());
    }
}

void MixOverviewComponent::timerCallback()
{
    refreshSelection();
}

void MixOverviewComponent::refreshSelection()
{
    const int selected = mapper.getSelectedModuleIndex();

    // Only touch the buttons when something changed -- setColour() repaints, and
    // this runs 15 times a second.
    if (selected == lastSelectedIndex)
        return;

    lastSelectedIndex = selected;

    for (int i = 0; i < 8; ++i)
    {
        auto& button = padButtons[(size_t) i];
        button.setColour(juce::TextButton::buttonColourId,
                         i == selected ? juce::Colours::orange : juce::Colours::darkgrey);
        button.setTooltip("Pad note " + juce::String(mapper.getPadNote(i)));
    }
}

} // namespace modulerack
