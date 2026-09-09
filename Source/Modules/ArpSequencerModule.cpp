#include "ArpSequencerModule.h"
#include "../Core/Euclidean.h"

namespace modulerack
{

ArpSequencerModule::ArpSequencerModule() : ModuleSlot("arp", "Arp / Sequencer")
{
    params[density].configure("density", "Density", 2.0f, 16.0f, 10.0f);
    params[evolveProbability].configure("evolveProbability", "Evolve", 0.0f, 1.0f, 0.12f);
    params[registerLength].configure("registerLength", "Register Len", 2.0f, 8.0f, 5.0f);
    params[octaveRange].configure("octaveRange", "Octave Range", 1.0f, 3.0f, 2.0f);
    params[noteLength].configure("noteLength", "Note Length", 0.1f, 0.9f, 0.4f);
    params[waveform].configure("waveform", "Waveform", 0.0f, 1.0f, 0.0f);
    params[glide].configure("glide", "Glide", 0.0f, 0.08f, 0.015f);
    params[level].configure("level", "Level", 0.0f, 1.5f, 0.5f);
}

void ArpSequencerModule::prepare(double sampleRateIn, int)
{
    sampleRate = sampleRateIn;
    random.setSeedRandomly();
    shiftRegister = (juce::uint16) random.nextInt();
}

void ArpSequencerModule::triggerNote(int midiNote, const Clock& clock)
{
    targetFreq = (float) juce::MidiMessage::getMidiNoteInHertz(midiNote);
    ampEnvelope = 1.0f;

    const double secondsPerSixteenth = 60.0 / (double) clock.getBpm() / 4.0;
    const double decayTime = juce::jmax(0.02, secondsPerSixteenth * params[noteLength].get() * 4.0);
    ampDecayCoeff = (float) std::exp(-1.0 / (decayTime * sampleRate));
}

void ArpSequencerModule::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&, const Clock& clock)
{
    const int hits = (int) std::round(params[density].get());
    if (hits != lastDensitySteps)
    {
        densityMask = computeEuclideanPattern(hits);
        lastDensitySteps = hits;
    }

    const int numSamples = buffer.getNumSamples();
    const int registerBits = juce::jlimit(2, 8, (int) std::round(params[registerLength].get()));
    const int octaves = juce::jmax(1, (int) std::round(params[octaveRange].get()));
    const bool useSquare = params[waveform].get() >= 0.5f;
    const float glideCoeff = params[glide].get() <= 0.0f
                                  ? 0.0f
                                  : (float) std::exp(-1.0 / (params[glide].get() * sampleRate));

    auto tickIt = clock.ticksThisBlock().begin();
    const auto tickEnd = clock.ticksThisBlock().end();

    for (int n = 0; n < numSamples; ++n)
    {
        while (tickIt != tickEnd && tickIt->sampleOffset == n)
        {
            const bool oldMsb = (shiftRegister & 0x8000) != 0;
            const bool newBit = random.nextFloat() < params[evolveProbability].get() ? ! oldMsb : oldMsb;
            shiftRegister = (juce::uint16) ((shiftRegister << 1) | (newBit ? 1 : 0));

            if (densityMask[(size_t) tickIt->sixteenthIndex])
            {
                const int mask = (1 << registerBits) - 1;
                const int value = shiftRegister & mask;
                const int degree = value % 7;
                const int octave = (value / 7) % octaves;
                triggerNote(Scale::degreeToMidiNote(degree + octave * 7, rootNote, Scale::Type::naturalMinor), clock);
            }

            ++tickIt;
        }

        currentFreq = glideCoeff > 0.0f ? (targetFreq + (currentFreq - targetFreq) * glideCoeff) : targetFreq;
        phase += currentFreq / (float) sampleRate;
        if (phase >= 1.0f)
            phase -= 1.0f;

        const float osc = useSquare ? (phase < 0.5f ? 1.0f : -1.0f) : (1.0f - std::abs(2.0f * phase - 1.0f) * 2.0f);
        const float sample = osc * ampEnvelope * params[level].get();
        ampEnvelope *= ampDecayCoeff;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addSample(ch, n, sample);
    }
}

} // namespace modulerack
