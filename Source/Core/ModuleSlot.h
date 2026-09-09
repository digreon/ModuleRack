#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include "Clock.h"

namespace modulerack
{

/**
 * One of the 8 knob-bound parameters exposed by a module while it is the selected
 * slot. The value is atomic because it is written from the UI thread (mouse) and
 * from the audio thread (incoming MIDI CC), and read by both.
 */
struct ModuleParam
{
    void configure(juce::String paramId, juce::String paramLabel,
                   float minimum, float maximum, float defaultVal)
    {
        id = std::move(paramId);
        label = std::move(paramLabel);
        minValue = minimum;
        maxValue = maximum;
        defaultValue = defaultVal;
        value.store(defaultVal, std::memory_order_relaxed);
    }

    float get() const { return value.load(std::memory_order_relaxed); }

    void set(float newValue)
    {
        value.store(juce::jlimit(minValue, maxValue, newValue), std::memory_order_relaxed);
    }

    void setNormalised(float value01)
    {
        set(minValue + juce::jlimit(0.0f, 1.0f, value01) * (maxValue - minValue));
    }

    float getNormalised() const
    {
        if (maxValue <= minValue)
            return 0.0f;

        return (get() - minValue) / (maxValue - minValue);
    }

    juce::String id;
    juce::String label;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.5f;

private:
    std::atomic<float> value { 0.5f };
};

/**
 * Base class for one of the 8 fixed module slots (Kick, Bass, Lead, Pad/Chord,
 * Arp/Sequencer, Percussion, Fx1, Fx2). All modules run continuously and mix
 * into the shared signal graph -- pressing a pad on the MPK Mini never starts
 * or stops a module, it only changes which module's 8 params the on-screen
 * panel and the 8 knobs currently point at (see MidiMapper).
 *
 * Fx1/Fx2 reuse the same interface but treat `buffer` as the accumulated mix
 * bus to process in place rather than an empty buffer to fill (see SignalGraph).
 */
class ModuleSlot
{
public:
    ModuleSlot(juce::String moduleId, juce::String displayName)
        : id(std::move(moduleId)), name(std::move(displayName))
    {
    }

    virtual ~ModuleSlot() = default;

    const juce::String id;
    const juce::String name;

    virtual void prepare(double sampleRate, int maximumBlockSize) = 0;
    virtual void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, const Clock& clock) = 0;

    /** Exposes this module's 8 knob-bound params, in the fixed order the MPK Mini knobs address. */
    virtual std::array<ModuleParam, 8>& getParams() = 0;

    void setParamNormalised(int knobIndex, float value01)
    {
        auto& params = getParams();
        if (juce::isPositiveAndBelow(knobIndex, (int) params.size()))
            params[(size_t) knobIndex].setNormalised(value01);
    }

    /** 0-1 envelope this tick, used by Fx2's sidechain compressor (only meaningful for Kick). */
    virtual float getSidechainEnvelope() const { return 0.0f; }

    /** Only implemented by Fx2 (reverb / sidechain compressor). */
    virtual void setSidechainEnvelope(float) {}

    std::unique_ptr<juce::XmlElement> toXml() const;
    void fromXml(const juce::XmlElement& xml);
};

} // namespace modulerack
