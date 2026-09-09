#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../Core/SignalGraph.h"
#include "../Core/MidiMapper.h"

namespace modulerack
{

/**
 * Row of 8 pad buttons mirroring the MPK Mini's pads: shows all 8 module
 * names at all times (all 8 are always sounding) and highlights whichever
 * one is currently selected for knob/panel control. Clicking a button
 * selects that module too, so the plugin is fully usable without hardware.
 */
class MixOverviewComponent : public juce::Component, private juce::Timer
{
public:
    MixOverviewComponent(SignalGraph& graphToUse, MidiMapper& mapperToUse);
    ~MixOverviewComponent() override;

    void resized() override;

private:
    void timerCallback() override;
    void refreshSelection();

    SignalGraph& graph;
    MidiMapper& mapper;
    std::array<juce::TextButton, 8> padButtons;
    int lastSelectedIndex = -1;
};

} // namespace modulerack
