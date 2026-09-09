#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "../Core/SignalGraph.h"
#include "../Core/MidiMapper.h"

namespace modulerack
{

/**
 * Shows the currently-selected module's name and its 8 knob-bound params as
 * rotary sliders. The sliders are two-way: dragging one calls
 * setParamNormalised() directly (so the plugin is usable with a mouse, no
 * MPK Mini required), and refresh() re-reads values so an incoming MIDI CC
 * (or a pad switching the selected module) is reflected on screen.
 */
class ModulePanel : public juce::Component, private juce::Timer
{
public:
    ModulePanel(SignalGraph& graphToUse, MidiMapper& mapperToUse);
    ~ModulePanel() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;
    void refresh();
    void rebuildForSelectedModule();

    SignalGraph& graph;
    MidiMapper& mapper;
    int lastSelectedIndex = -1;
    bool updatingFromCode = false;

    juce::Label titleLabel;
    std::array<juce::Slider, 8> knobSliders;
    std::array<juce::Label, 8> knobLabels;
};

} // namespace modulerack
