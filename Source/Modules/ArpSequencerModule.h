#pragma once

#include "../Core/ModuleSlot.h"
#include "../Core/Scale.h"

namespace modulerack
{

/**
 * Generative sequencer in the spirit of Mutable Instruments Marbles / a
 * Turing Machine module: a 16-bit shift register shifts one bit every 16th
 * note, with `evolveProbability` chance to flip the incoming bit each step.
 * The register's low bits are read out as a scale-degree + octave index, so
 * the melody drifts and occasionally mutates rather than repeating exactly,
 * and never fully resets.
 */
class ArpSequencerModule : public ModuleSlot
{
public:
    ArpSequencerModule();

    void prepare(double sampleRate, int maximumBlockSize) override;
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, const Clock& clock) override;
    std::array<ModuleParam, 8>& getParams() override { return params; }

private:
    enum ParamIndex
    {
        density = 0,
        evolveProbability,
        registerLength,
        octaveRange,
        noteLength,
        waveform,
        glide,
        level
    };

    void triggerNote(int midiNote, const Clock& clock);

    std::array<ModuleParam, 8> params;
    std::array<bool, 16> densityMask {};
    int lastDensitySteps = -1;

    juce::uint16 shiftRegister = 0;
    juce::Random random;

    double sampleRate = 44100.0;
    float phase = 0.0f;
    float currentFreq = 220.0f;
    float targetFreq = 220.0f;
    float ampEnvelope = 0.0f;
    float ampDecayCoeff = 0.999f;

    static constexpr int rootNote = 69; // A4
};

} // namespace modulerack
