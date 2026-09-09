#include "FxReverbCompModule.h"

namespace modulerack
{

FxReverbCompModule::FxReverbCompModule() : ModuleSlot("fx2", "Fx2: Reverb/Comp")
{
    params[reverbSize].configure("reverbSize", "Size", 0.0f, 1.0f, 0.5f);
    params[reverbDamping].configure("reverbDamping", "Damping", 0.0f, 1.0f, 0.5f);
    params[reverbMix].configure("reverbMix", "Mix", 0.0f, 1.0f, 0.25f);
    params[reverbWidth].configure("reverbWidth", "Width", 0.0f, 1.0f, 1.0f);
    params[sidechainAmount].configure("sidechainAmount", "SC Amount", 0.0f, 1.0f, 0.5f);
    params[sidechainAttack].configure("sidechainAttack", "SC Attack", 0.001f, 0.1f, 0.005f);
    params[sidechainRelease].configure("sidechainRelease", "SC Release", 0.02f, 0.6f, 0.18f);
    params[level].configure("level", "Level", 0.0f, 1.5f, 1.0f);
}

void FxReverbCompModule::prepare(double sampleRateIn, int maximumBlockSize)
{
    sampleRate = sampleRateIn;
    juce::dsp::ProcessSpec spec { sampleRateIn, (juce::uint32) maximumBlockSize, 2 };
    reverb.prepare(spec);
}

void FxReverbCompModule::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&, const Clock&)
{
    juce::dsp::Reverb::Parameters reverbParams;
    reverbParams.roomSize = params[reverbSize].get();
    reverbParams.damping = params[reverbDamping].get();
    reverbParams.wetLevel = params[reverbMix].get();
    reverbParams.dryLevel = 1.0f - params[reverbMix].get();
    reverbParams.width = params[reverbWidth].get();
    reverb.setParameters(reverbParams);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Ducking target: how far to pull the gain down for the current sidechain envelope.
    const float duckTarget = 1.0f - incomingSidechainEnvelope * params[sidechainAmount].get();
    const bool ducking = duckTarget < duckGain;
    const float coeff = ducking
                             ? (float) std::exp(-1.0 / (params[sidechainAttack].get() * sampleRate))
                             : (float) std::exp(-1.0 / (params[sidechainRelease].get() * sampleRate));

    for (int n = 0; n < numSamples; ++n)
    {
        duckGain = duckTarget + (duckGain - duckTarget) * coeff;

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.setSample(ch, n, buffer.getSample(ch, n) * duckGain);
    }

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);

    buffer.applyGain(params[level].get());
}

} // namespace modulerack
