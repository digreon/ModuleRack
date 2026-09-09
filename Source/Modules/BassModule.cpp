#include "BassModule.h"
#include "../Core/Euclidean.h"

namespace modulerack
{

BassModule::BassModule() : ModuleSlot("bass", "Bass")
{
    params[waveform].configure("waveform", "Waveform", 0.0f, 1.0f, 0.0f);
    params[cutoff].configure("cutoff", "Cutoff", 100.0f, 3000.0f, 500.0f);
    params[resonance].configure("resonance", "Resonance", 0.0f, 0.9f, 0.3f);
    params[envAmount].configure("envAmount", "Env Amount", 0.0f, 4000.0f, 1500.0f);
    params[decay].configure("decay", "Decay", 0.05f, 0.6f, 0.18f);
    params[density].configure("density", "Density", 2.0f, 12.0f, 8.0f);
    params[evolveProbability].configure("evolveProbability", "Evolve", 0.0f, 1.0f, 0.15f);
    params[level].configure("level", "Level", 0.0f, 1.5f, 0.9f);

    degreePattern = { 0, -1, -1, 0, -1, -1, 3, -1,
                       0, -1, -1, 0, -1, 4, -1, -1 };
}

void BassModule::prepare(double sampleRateIn, int)
{
    sampleRate = sampleRateIn;
    filter.prepare(sampleRateIn);
    random.setSeedRandomly();
}

void BassModule::maybeEvolvePattern()
{
    if (random.nextFloat() < params[evolveProbability].get())
    {
        const int step = random.nextInt(16);
        static constexpr std::array<int, 5> candidateDegrees { -1, 0, 2, 3, 4 };
        degreePattern[(size_t) step] = candidateDegrees[(size_t) random.nextInt(5)];
    }
}

void BassModule::triggerNote(int midiNote)
{
    currentNoteHz = (float) juce::MidiMessage::getMidiNoteInHertz(midiNote);
    phase = 0.0f;
    ampEnvelope = 1.0f;
    filterEnvelope = 1.0f;
}

void BassModule::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&, const Clock& clock)
{
    const int hits = (int) std::round(params[density].get());
    if (hits != lastDensitySteps)
    {
        densityMask = computeEuclideanPattern(hits);
        lastDensitySteps = hits;
    }

    const int numSamples = buffer.getNumSamples();
    const float decayCoeff = (float) std::exp(-1.0 / (params[decay].get() * sampleRate));
    const bool useSquare = params[waveform].get() >= 0.5f;

    auto tickIt = clock.ticksThisBlock().begin();
    const auto tickEnd = clock.ticksThisBlock().end();

    for (int n = 0; n < numSamples; ++n)
    {
        while (tickIt != tickEnd && tickIt->sampleOffset == n)
        {
            if (tickIt->sixteenthIndex == 0)
                maybeEvolvePattern();

            const int degree = degreePattern[(size_t) tickIt->sixteenthIndex];
            if (degree >= 0 && densityMask[(size_t) tickIt->sixteenthIndex])
                triggerNote(Scale::degreeToMidiNote(degree, rootNote, Scale::Type::minorPentatonic));

            ++tickIt;
        }

        phase += currentNoteHz / (float) sampleRate;
        if (phase >= 1.0f)
            phase -= 1.0f;

        const float osc = useSquare ? (phase < 0.5f ? 1.0f : -1.0f) : (2.0f * phase - 1.0f);

        filter.setCutoffHz(params[cutoff].get() + filterEnvelope * params[envAmount].get());
        filter.setResonance01(params[resonance].get());
        const float filtered = filter.processLowpass(osc);

        const float sample = filtered * ampEnvelope * params[level].get();
        ampEnvelope *= decayCoeff;
        filterEnvelope *= decayCoeff;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addSample(ch, n, sample);
    }
}

} // namespace modulerack
