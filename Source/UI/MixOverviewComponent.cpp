#include "MixOverviewComponent.h"

namespace modulerack
{

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

    for (int i = 0; i < 8; ++i)
        padButtons[(size_t) i].setBounds(area.getX() + i * cellWidth, area.getY(), cellWidth - 4, area.getHeight());
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
