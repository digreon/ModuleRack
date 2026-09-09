#pragma once

#include "../Core/ModuleSlot.h"
#include "../Core/SimpleFilter.h"
#include "../Core/Scale.h"

namespace modulerack
{

/**
 * Generative bassline: a 16-step scale-degree pattern (with rests) that
 * occasionally mutates a step at the start of a bar -- a simple relative of
 * a Turing Machine sequencer. Voice is a single saw/square oscillator through
 * a resonant lowpass with an envelope-modulated cutoff.
 */
class BassModule : public ModuleSlot
{
public:
    BassModule();

    void prepare(double sampleRate, int maximumBlockSize) override;
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, const Clock& clock) override;
    std::array<ModuleParam, 8>& getParams() override { return params; }

private:
    enum ParamIndex
    {
        waveform = 0,
        cutoff,
        resonance,
        envAmount,
        decay,
        density,
        evolveProbability,
        level
    };

    void maybeEvolvePattern();
    void triggerNote(int midiNote);

    std::array<ModuleParam, 8> params;
    std::array<int, 16> degreePattern {}; // -1 == rest
    std::array<bool, 16> densityMask {};
    int lastDensitySteps = -1;
    juce::Random random;

    double sampleRate = 44100.0;
    SimpleFilter filter;

    float phase = 0.0f;
    float currentNoteHz = 55.0f;
    float ampEnvelope = 0.0f;
    float filterEnvelope = 0.0f;

    static constexpr int rootNote = 33; // A1, bass register
};

} // namespace modulerack
