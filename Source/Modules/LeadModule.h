#pragma once

#include "../Core/ModuleSlot.h"
#include "../Core/SimpleFilter.h"
#include "../Core/Scale.h"

namespace modulerack
{

/**
 * Generative lead line: sparser 16-step scale-degree pattern than Bass, an
 * octave higher, with slight detune (two-oscillator unison) and vibrato.
 * Same "evolve a random step per bar" generative approach as Bass, but with
 * its own pattern/randomisation so the two never lock into the same rhythm.
 */
class LeadModule : public ModuleSlot
{
public:
    LeadModule();

    void prepare(double sampleRate, int maximumBlockSize) override;
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, const Clock& clock) override;
    std::array<ModuleParam, 8>& getParams() override { return params; }

private:
    enum ParamIndex
    {
        detune = 0,
        cutoff,
        resonance,
        density,
        evolveProbability,
        octave,
        vibratoAmount,
        level
    };

    void maybeEvolvePattern();
    void triggerNote(int midiNote);

    std::array<ModuleParam, 8> params;
    std::array<int, 16> degreePattern {};
    std::array<bool, 16> densityMask {};
    int lastDensitySteps = -1;
    juce::Random random;

    double sampleRate = 44100.0;
    SimpleFilter filter;

    float phaseA = 0.0f, phaseB = 0.0f;
    float currentNoteHz = 440.0f;
    float ampEnvelope = 0.0f;
    float vibratoPhase = 0.0f;

    static constexpr int rootNote = 57; // A3
};

} // namespace modulerack
