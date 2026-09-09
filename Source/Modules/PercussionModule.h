#pragma once

#include "../Core/ModuleSlot.h"
#include "../Core/SimpleFilter.h"
#include "../Core/SwingDelay.h"

namespace modulerack
{

/**
 * Hi-hat / percussion layer: a second, independent Euclidean pattern (so it
 * never simply doubles the kick) triggers highpass-filtered noise bursts,
 * randomly choosing between a short "closed" and longer "open" decay.
 */
class PercussionModule : public ModuleSlot
{
public:
    PercussionModule();

    void prepare(double sampleRate, int maximumBlockSize) override;
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, const Clock& clock) override;
    std::array<ModuleParam, 8>& getParams() override { return params; }

private:
    enum ParamIndex
    {
        density = 0,
        openHatProbability,
        decayClosed,
        decayOpen,
        hpfCutoff,
        accentAmount,
        swing,
        level
    };

    struct HatHit
    {
        bool open = false;
        bool accent = false;
    };

    void fireHat(HatHit hit);

    std::array<ModuleParam, 8> params;
    std::array<bool, 16> pattern {};
    int lastDensitySteps = -1;
    SwingDelay<HatHit> swingDelay;

    double sampleRate = 44100.0;
    juce::Random random;
    SimpleFilter filter;

    float ampEnvelope = 0.0f;
    float decayCoeff = 0.9f;
    float accentGain = 1.0f;
};

} // namespace modulerack
