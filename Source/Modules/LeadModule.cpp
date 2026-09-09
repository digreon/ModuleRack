#include "LeadModule.h"
#include "../Core/Euclidean.h"

namespace modulerack
{

LeadModule::LeadModule() : ModuleSlot("lead", "Lead")
{
    params[detune].configure("detune", "Detune", 0.0f, 0.02f, 0.006f);
    params[cutoff].configure("cutoff", "Cutoff", 400.0f, 8000.0f, 3000.0f);
    params[resonance].configure("resonance", "Resonance", 0.0f, 0.9f, 0.15f);
    params[density].configure("density", "Density", 1.0f, 8.0f, 3.0f);
    params[evolveProbability].configure("evolveProbability", "Evolve", 0.0f, 1.0f, 0.2f);
    params[octave].configure("octave", "Octave", -1.0f, 2.0f, 0.0f);
    params[vibratoAmount].configure("vibratoAmount", "Vibrato", 0.0f, 0.02f, 0.004f);
    params[level].configure("level", "Level", 0.0f, 1.5f, 0.7f);

    degreePattern = { 0, -1, 3, -1, -1, 2, -1, -1,
                       4, -1, -1, 3, -1, -1, 0, -1 };
}

void LeadModule::prepare(double sampleRateIn, int)
{
    sampleRate = sampleRateIn;
    filter.prepare(sampleRateIn);
    random.setSeedRandomly();
}

void LeadModule::maybeEvolvePattern()
{
    if (random.nextFloat() < params[evolveProbability].get())
    {
        const int step = random.nextInt(16);
        static constexpr std::array<int, 6> candidateDegrees { -1, -1, 0, 2, 3, 4 };
        degreePattern[(size_t) step] = candidateDegrees[(size_t) random.nextInt(6)];
    }
}

void LeadModule::triggerNote(int midiNote)
{
    const int octaveShift = (int) std::round(params[octave].get());
    currentNoteHz = (float) juce::MidiMessage::getMidiNoteInHertz(midiNote + octaveShift * 12);
    ampEnvelope = 1.0f;
}

void LeadModule::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&, const Clock& clock)
{
    const int hits = (int) std::round(params[density].get());
    if (hits != lastDensitySteps)
    {
        densityMask = computeEuclideanPattern(hits);
        lastDensitySteps = hits;
    }

    const int numSamples = buffer.getNumSamples();
    const float decayCoeff = (float) std::exp(-1.0 / (0.35 * sampleRate));

    filter.setCutoffHz(params[cutoff].get());
    filter.setResonance01(params[resonance].get());

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
                triggerNote(Scale::degreeToMidiNote(degree, rootNote, Scale::Type::dorian));

            ++tickIt;
        }

        vibratoPhase += 5.5f / (float) sampleRate;
        if (vibratoPhase >= 1.0f)
            vibratoPhase -= 1.0f;
        const float vibrato = std::sin(juce::MathConstants<float>::twoPi * vibratoPhase) * params[vibratoAmount].get();

        const float freqA = currentNoteHz * (1.0f + vibrato + params[detune].get());
        const float freqB = currentNoteHz * (1.0f + vibrato - params[detune].get());

        phaseA += freqA / (float) sampleRate;
        if (phaseA >= 1.0f) phaseA -= 1.0f;
        phaseB += freqB / (float) sampleRate;
        if (phaseB >= 1.0f) phaseB -= 1.0f;

        const float oscA = 2.0f * phaseA - 1.0f;
        const float oscB = 2.0f * phaseB - 1.0f;
        const float filtered = filter.processLowpass((oscA + oscB) * 0.5f);

        const float sample = filtered * ampEnvelope * params[level].get();
        ampEnvelope *= decayCoeff;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addSample(ch, n, sample);
    }
}

} // namespace modulerack
