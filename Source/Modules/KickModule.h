#pragma once

#include "../Core/ModuleSlot.h"
#include "../Core/SwingDelay.h"

namespace modulerack
{

/**
 * Procedural kick drum: a Euclidean 16-step pattern generator triggers a
 * pitch-sweeping sine voice plus a short noise click. Also exposes its
 * amplitude envelope as the sidechain source for FxReverbCompModule.
 */
class KickModule : public ModuleSlot
{
public:
    KickModule();

    void prepare(double sampleRate, int maximumBlockSize) override;
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, const Clock& clock) override;
    std::array<ModuleParam, 8>& getParams() override { return params; }
    float getSidechainEnvelope() const override { return ampEnvelope; }

private:
    void trigger();
    void updatePattern();

    enum ParamIndex
    {
        density = 0,
        startFreq,
        pitchDecay,
        ampDecay,
        clickAmount,
        drive,
        level,
        swing
    };

    struct NoPayload {};

    std::array<ModuleParam, 8> params;
    std::array<bool, 16> pattern {};
    int lastDensitySteps = -1;
    SwingDelay<NoPayload> swingDelay;

    double sampleRate = 44100.0;
    juce::Random random;

    // Voice state
    float phase = 0.0f;
    float currentFreq = 0.0f;
    float ampEnvelope = 0.0f;
    float clickEnvelope = 0.0f;
};

} // namespace modulerack
