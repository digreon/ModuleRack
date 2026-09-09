#include "KickModule.h"
#include "../Core/Euclidean.h"

namespace modulerack
{

KickModule::KickModule() : ModuleSlot("kick", "Kick")
{
    params[density].configure("density", "Density", 1.0f, 12.0f, 6.0f);
    params[startFreq].configure("startFreq", "Start Freq", 60.0f, 200.0f, 120.0f);
    params[pitchDecay].configure("pitchDecay", "Pitch Decay", 0.01f, 0.25f, 0.05f);
    params[ampDecay].configure("ampDecay", "Amp Decay", 0.05f, 0.8f, 0.3f);
    params[clickAmount].configure("clickAmount", "Click", 0.0f, 1.0f, 0.4f);
    params[drive].configure("drive", "Drive", 1.0f, 6.0f, 1.5f);
    params[level].configure("level", "Level", 0.0f, 1.5f, 1.0f);
    params[swing].configure("swing", "Swing", 0.0f, 0.5f, 0.0f);
}

void KickModule::prepare(double sampleRateIn, int)
{
    sampleRate = sampleRateIn;
    random.setSeedRandomly();
    swingDelay.prepare(sampleRateIn);
    updatePattern();
}

void KickModule::updatePattern()
{
    const int hits = (int) std::round(params[density].get());
    if (hits != lastDensitySteps)
    {
        pattern = computeEuclideanPattern(hits);
        lastDensitySteps = hits;
    }
}

void KickModule::trigger()
{
    phase = 0.0f;
    currentFreq = params[startFreq].get();
    ampEnvelope = 1.0f;
    clickEnvelope = 1.0f;
}

void KickModule::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&, const Clock& clock)
{
    updatePattern();

    const int numSamples = buffer.getNumSamples();
    const float pitchDecayCoeff = (float) std::exp(-1.0 / (params[pitchDecay].get() * sampleRate));
    const float ampDecayCoeff = (float) std::exp(-1.0 / (params[ampDecay].get() * sampleRate));
    const float clickDecayCoeff = (float) std::exp(-1.0 / (0.004 * sampleRate));
    const float endFreq = 40.0f;
    const int swingSamples = swingDelay.delaySamples(params[swing].get(), clock.getBpm());

    // Walk the block sample-by-sample so a mid-block clock tick retriggers cleanly.
    auto tickIt = clock.ticksThisBlock().begin();
    const auto tickEnd = clock.ticksThisBlock().end();

    for (int n = 0; n < numSamples; ++n)
    {
        while (tickIt != tickEnd && tickIt->sampleOffset == n)
        {
            if (pattern[(size_t) tickIt->sixteenthIndex])
            {
                const bool isOffbeat = (tickIt->sixteenthIndex % 2) != 0;

                if (isOffbeat && swingSamples > 0)
                    swingDelay.schedule({}, swingSamples);
                else
                    trigger();
            }
            ++tickIt;
        }

        NoPayload pending;
        if (swingDelay.advance(pending))
            trigger();

        currentFreq = endFreq + (currentFreq - endFreq) * pitchDecayCoeff;
        phase += currentFreq / (float) sampleRate;
        if (phase >= 1.0f)
            phase -= 1.0f;

        const float sine = std::sin(juce::MathConstants<float>::twoPi * phase);
        const float click = clickEnvelope * (random.nextFloat() * 2.0f - 1.0f) * params[clickAmount].get();
        clickEnvelope *= clickDecayCoeff;

        float sample = (sine * ampEnvelope + click) * params[level].get();
        sample = std::tanh(sample * params[drive].get()) / std::tanh(params[drive].get());
        ampEnvelope *= ampDecayCoeff;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addSample(ch, n, sample);
    }
}

} // namespace modulerack
