#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <memory>
#include "ModuleSlot.h"
#include "Clock.h"

namespace modulerack
{

/**
 * Fixed MVP signal path (see project README, "Phase 1"):
 *
 *   [Kick][Bass][Lead][Pad][Arp][Percussion] --sum--> mix bus --> Fx1 (filter/delay)
 *                                                                --> Fx2 (reverb / kick sidechain-comp)
 *                                                                --> master output
 *
 * No per-module sends and no patch matrix yet -- every sound module always
 * feeds the same mix bus, and the two Fx slots are always inserted on that
 * bus in series. A free-patching mod matrix is deliberately deferred to
 * Phase 2 (see README) so the audio graph itself never needs to change shape
 * at runtime for the MVP.
 */
class SignalGraph
{
public:
    static constexpr int numModules = 8;
    static constexpr int kickIndex = 0;
    static constexpr int bassIndex = 1;
    static constexpr int leadIndex = 2;
    static constexpr int padIndex = 3;
    static constexpr int arpIndex = 4;
    static constexpr int percussionIndex = 5;
    static constexpr int fx1Index = 6;
    static constexpr int fx2Index = 7;

    /** Gain applied to each sound module as it sums into the mix bus (see process()). */
    static constexpr float mixTrim = 0.25f;

    SignalGraph();

    void prepare(double sampleRate, int maximumBlockSize);
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);

    std::array<std::unique_ptr<ModuleSlot>, numModules>& getModules() { return modules; }
    Clock& getClock() { return clock; }

private:
    std::array<std::unique_ptr<ModuleSlot>, numModules> modules;
    Clock clock;

    juce::AudioBuffer<float> mixBuffer;
    juce::AudioBuffer<float> scratchBuffer;
};

} // namespace modulerack
