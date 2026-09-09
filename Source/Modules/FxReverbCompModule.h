#pragma once

#include "../Core/ModuleSlot.h"

namespace modulerack
{

/**
 * Fx slot 2: reverb + sidechain compressor, inserted after Fx1 on the master
 * bus. The sidechain "compressor" is a simple gain-ducking envelope driven by
 * the Kick module's amplitude envelope (see SignalGraph::process(), which
 * reads Kick::getSidechainEnvelope() and feeds it in via setSidechainEnvelope()
 * every block, before Fx2's own process() call).
 */
class FxReverbCompModule : public ModuleSlot
{
public:
    FxReverbCompModule();

    void prepare(double sampleRate, int maximumBlockSize) override;
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, const Clock& clock) override;
    std::array<ModuleParam, 8>& getParams() override { return params; }

    void setSidechainEnvelope(float env) override { incomingSidechainEnvelope = env; }

private:
    enum ParamIndex
    {
        reverbSize = 0,
        reverbDamping,
        reverbMix,
        reverbWidth,
        sidechainAmount,
        sidechainAttack,
        sidechainRelease,
        level
    };

    std::array<ModuleParam, 8> params;
    double sampleRate = 44100.0;

    juce::dsp::Reverb reverb;
    float incomingSidechainEnvelope = 0.0f;
    float duckGain = 1.0f;
};

} // namespace modulerack
