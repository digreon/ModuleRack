#include "PercussionModule.h"
#include "../Core/Euclidean.h"

namespace modulerack
{

PercussionModule::PercussionModule() : ModuleSlot("percussion", "Percussion")
{
    params[density].configure("density", "Density", 2.0f, 16.0f, 9.0f);
    params[openHatProbability].configure("openHatProbability", "Open Prob", 0.0f, 1.0f, 0.2f);
    params[decayClosed].configure("decayClosed", "Decay Closed", 0.01f, 0.15f, 0.04f);
    params[decayOpen].configure("decayOpen", "Decay Open", 0.1f, 0.6f, 0.25f);
    params[hpfCutoff].configure("hpfCutoff", "HPF Cutoff", 2000.0f, 9000.0f, 6000.0f);
    params[accentAmount].configure("accentAmount", "Accent", 0.0f, 1.0f, 0.3f);
    params[swing].configure("swing", "Swing", 0.0f, 0.5f, 0.1f);
    params[level].configure("level", "Level", 0.0f, 1.5f, 0.5f);
}

void PercussionModule::prepare(double sampleRateIn, int)
{
    sampleRate = sampleRateIn;
    random.setSeedRandomly();
    swingDelay.prepare(sampleRateIn);
    filter.prepare(sampleRateIn);
    filter.setResonance01(0.0f);
}

void PercussionModule::fireHat(HatHit hit)
{
    ampEnvelope = 1.0f;
    const float decayTime = hit.open ? params[decayOpen].get() : params[decayClosed].get();
    decayCoeff = (float) std::exp(-1.0 / (decayTime * sampleRate));
    accentGain = hit.accent ? 1.0f : (1.0f - params[accentAmount].get() * 0.5f);
}

void PercussionModule::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&, const Clock& clock)
{
    const int hits = (int) std::round(params[density].get());
    if (hits != lastDensitySteps)
    {
        pattern = computeEuclideanPattern(hits);
        lastDensitySteps = hits;
    }

    const int numSamples = buffer.getNumSamples();
    const int swingSamples = swingDelay.delaySamples(params[swing].get(), clock.getBpm());

    filter.setCutoffHz(params[hpfCutoff].get());

    auto tickIt = clock.ticksThisBlock().begin();
    const auto tickEnd = clock.ticksThisBlock().end();

    for (int n = 0; n < numSamples; ++n)
    {
        while (tickIt != tickEnd && tickIt->sampleOffset == n)
        {
            if (pattern[(size_t) tickIt->sixteenthIndex])
            {
                const HatHit hit { random.nextFloat() < params[openHatProbability].get(),
                                   tickIt->sixteenthIndex % 4 == 0 };
                const bool isOffbeat = (tickIt->sixteenthIndex % 2) != 0;

                if (isOffbeat && swingSamples > 0)
                    swingDelay.schedule(hit, swingSamples);
                else
                    fireHat(hit);
            }
            ++tickIt;
        }

        HatHit pending;
        if (swingDelay.advance(pending))
            fireHat(pending);

        const float noise = random.nextFloat() * 2.0f - 1.0f;
        const float filtered = filter.processHighpass(noise);
        const float sample = filtered * ampEnvelope * accentGain * params[level].get();
        ampEnvelope *= decayCoeff;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addSample(ch, n, sample);
    }
}

} // namespace modulerack
