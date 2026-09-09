#include "FxFilterDelayModule.h"

namespace modulerack
{

FxFilterDelayModule::FxFilterDelayModule() : ModuleSlot("fx1", "Fx1: Filter/Delay")
{
    params[cutoff].configure("cutoff", "Cutoff", 200.0f, 12000.0f, 8000.0f);
    params[resonance].configure("resonance", "Resonance", 0.0f, 0.9f, 0.1f);
    params[filterMix].configure("filterMix", "Filter Mix", 0.0f, 1.0f, 0.0f);
    params[filterType].configure("filterType", "Filter Type", 0.0f, 1.0f, 0.0f);
    params[delayTimeMs].configure("delayTimeMs", "Delay Time", 30.0f, 900.0f, 375.0f);
    params[delayFeedback].configure("delayFeedback", "Feedback", 0.0f, 0.85f, 0.3f);
    params[delayMix].configure("delayMix", "Delay Mix", 0.0f, 1.0f, 0.2f);
    params[level].configure("level", "Level", 0.0f, 1.5f, 1.0f);
}

void FxFilterDelayModule::prepare(double sampleRateIn, int maximumBlockSize)
{
    sampleRate = sampleRateIn;

    juce::dsp::ProcessSpec spec { sampleRateIn, (juce::uint32) maximumBlockSize, 1 };
    maxDelayInSamples = (int) (maxDelaySeconds * sampleRateIn) + 1;

    for (auto& filter : filters)
        filter.prepare(sampleRateIn);

    for (auto& delayLine : delayLines)
    {
        delayLine.prepare(spec);
        delayLine.setMaximumDelayInSamples(maxDelayInSamples);
        delayLine.reset();
    }
}

void FxFilterDelayModule::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&, const Clock&)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin(buffer.getNumChannels(), maxChannels);
    const bool highpass = params[filterType].get() >= 0.5f;
    const float delaySamples = juce::jmin(params[delayTimeMs].get() * 0.001f * (float) sampleRate,
                                          (float) maxDelayInSamples);
    const float feedback = params[delayFeedback].get();
    const float wetMix = params[delayMix].get();
    const float dryWetFilter = params[filterMix].get();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        filters[(size_t) ch].setCutoffHz(params[cutoff].get());
        filters[(size_t) ch].setResonance01(params[resonance].get());
        delayLines[(size_t) ch].setDelay(delaySamples);
    }

    for (int n = 0; n < numSamples; ++n)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float dry = buffer.getSample(ch, n);
            const float filtered = highpass ? filters[(size_t) ch].processHighpass(dry)
                                             : filters[(size_t) ch].processLowpass(dry);
            const float afterFilter = dry + (filtered - dry) * dryWetFilter;

            auto& delayLine = delayLines[(size_t) ch];
            const float delayed = delayLine.popSample(0);
            delayLine.pushSample(0, afterFilter + delayed * feedback);

            const float wet = afterFilter + (delayed - afterFilter) * wetMix;
            buffer.setSample(ch, n, wet * params[level].get());
        }
    }
}

} // namespace modulerack
