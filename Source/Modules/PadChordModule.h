#pragma once

#include "../Core/ModuleSlot.h"
#include "../Core/SimpleFilter.h"
#include "../Core/Scale.h"

namespace modulerack
{

/**
 * Sustained chord pad: cycles through a fixed 4-chord diatonic progression
 * (root/VI/iv/v-style, degrees within the scale), holding each chord for
 * `chordLength` bars. Each chord tone is a detuned two-oscillator unison
 * voice; a shared JUCE ADSR envelope glides legato between chords.
 */
class PadChordModule : public ModuleSlot
{
public:
    PadChordModule();

    void prepare(double sampleRate, int maximumBlockSize) override;
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, const Clock& clock) override;
    std::array<ModuleParam, 8>& getParams() override { return params; }

private:
    enum ParamIndex
    {
        chordLength = 0,
        detune,
        attack,
        release,
        cutoff,
        filterEnvAmount,
        voicingSpread,
        level
    };

    void triggerNextChord();

    std::array<ModuleParam, 8> params;
    static constexpr int numTones = 4; // root, third, fifth, root+octave (scaled by voicingSpread)

    std::array<float, numTones> toneFreq {};
    std::array<float, numTones> tonePhaseA {};
    std::array<float, numTones> tonePhaseB {};

    static constexpr std::array<int, 4> progressionRootDegrees { 0, 5, 3, 4 };
    int progressionIndex = -1; // -1 so the very first bar triggers a chord
    int barCounter = 0;

    double sampleRate = 44100.0;
    juce::ADSR envelope;
    SimpleFilter filter;

    static constexpr int rootNote = 48; // C3
};

} // namespace modulerack
