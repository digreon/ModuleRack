#pragma once

#include "../Core/ModuleSlot.h"
#include "../Core/SimpleFilter.h"
#include <array>

namespace modulerack
{

/**
 * Fx slot 1: filter + delay, inserted on the master mix bus (see SignalGraph).
 * Unlike the other modules, `process()` treats `buffer` as the already-summed
 * mix to modify in place rather than an empty buffer to fill.
 */
class FxFilterDelayModule : public ModuleSlot
{
public:
    FxFilterDelayModule();

    void prepare(double sampleRate, int maximumBlockSize) override;
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, const Clock& clock) override;
    std::array<ModuleParam, 8>& getParams() override { return params; }

private:
    enum ParamIndex
    {
        cutoff = 0,
        resonance,
        filterMix,
        filterType, // < 0.5 lowpass, >= 0.5 highpass
        delayTimeMs,
        delayFeedback,
        delayMix,
        level
    };

    std::array<ModuleParam, 8> params;
    double sampleRate = 44100.0;

    static constexpr int maxChannels = 2;

    // The Delay Time knob tops out at 900 ms; one second of headroom covers it at
    // any sample rate, and the lines are resized to match in prepare().
    static constexpr double maxDelaySeconds = 1.0;

    std::array<SimpleFilter, maxChannels> filters;
    std::array<juce::dsp::DelayLine<float>, maxChannels> delayLines;
    int maxDelayInSamples = 0;
};

} // namespace modulerack
