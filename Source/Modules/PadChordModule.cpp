#include "PadChordModule.h"

namespace modulerack
{

PadChordModule::PadChordModule() : ModuleSlot("pad", "Pad / Chord")
{
    params[chordLength].configure("chordLength", "Chord Length", 1.0f, 8.0f, 2.0f);
    params[detune].configure("detune", "Detune", 0.0f, 0.02f, 0.008f);
    params[attack].configure("attack", "Attack", 0.05f, 4.0f, 1.2f);
    params[release].configure("release", "Release", 0.05f, 4.0f, 1.5f);
    params[cutoff].configure("cutoff", "Cutoff", 300.0f, 6000.0f, 1800.0f);
    params[filterEnvAmount].configure("filterEnvAmount", "Filter Env", 0.0f, 4000.0f, 1200.0f);
    params[voicingSpread].configure("voicingSpread", "Spread", 0.0f, 1.0f, 0.4f);
    params[level].configure("level", "Level", 0.0f, 1.5f, 0.6f);
}

void PadChordModule::prepare(double sampleRateIn, int)
{
    sampleRate = sampleRateIn;
    filter.prepare(sampleRateIn);
    filter.setResonance01(0.1f);
    envelope.setSampleRate(sampleRateIn);
}

void PadChordModule::triggerNextChord()
{
    progressionIndex = (progressionIndex + 1) % (int) progressionRootDegrees.size();
    const int rootDegree = progressionRootDegrees[(size_t) progressionIndex];

    const int rootMidi = Scale::degreeToMidiNote(rootDegree, rootNote, Scale::Type::naturalMinor);
    const int thirdMidi = Scale::degreeToMidiNote(rootDegree + 2, rootNote, Scale::Type::naturalMinor);
    const int fifthMidi = Scale::degreeToMidiNote(rootDegree + 4, rootNote, Scale::Type::naturalMinor);

    toneFreq[0] = (float) juce::MidiMessage::getMidiNoteInHertz(rootMidi);
    toneFreq[1] = (float) juce::MidiMessage::getMidiNoteInHertz(thirdMidi);
    toneFreq[2] = (float) juce::MidiMessage::getMidiNoteInHertz(fifthMidi);
    toneFreq[3] = toneFreq[0] * 2.0f;

    envelope.noteOn();
}

void PadChordModule::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&, const Clock& clock)
{
    envelope.setParameters({ params[attack].get(), 0.02f, 1.0f, params[release].get() });

    const int numSamples = buffer.getNumSamples();
    const int chordLengthBars = (int) std::round(params[chordLength].get());
    const float detuneAmount = params[detune].get();

    auto tickIt = clock.ticksThisBlock().begin();
    const auto tickEnd = clock.ticksThisBlock().end();

    for (int n = 0; n < numSamples; ++n)
    {
        while (tickIt != tickEnd && tickIt->sampleOffset == n)
        {
            if (tickIt->sixteenthIndex == 0)
            {
                if (barCounter % juce::jmax(1, chordLengthBars) == 0)
                    triggerNextChord();
                ++barCounter;
            }
            else if (tickIt->sixteenthIndex == 15 && barCounter % juce::jmax(1, chordLengthBars) == 0)
            {
                // Last 16th of this chord's final bar: let go of the envelope so the
                // Release knob shapes an audible fade into the next chord instead of
                // the pad sitting in sustain forever.
                envelope.noteOff();
            }
            ++tickIt;
        }

        const float envVal = envelope.getNextSample();

        float sum = 0.0f;
        for (size_t t = 0; t < (size_t) numTones; ++t)
        {
            const float freq = toneFreq[t];
            tonePhaseA[t] += freq * (1.0f + detuneAmount) / (float) sampleRate;
            if (tonePhaseA[t] >= 1.0f) tonePhaseA[t] -= 1.0f;
            tonePhaseB[t] += freq * (1.0f - detuneAmount) / (float) sampleRate;
            if (tonePhaseB[t] >= 1.0f) tonePhaseB[t] -= 1.0f;

            const float oscA = 2.0f * tonePhaseA[t] - 1.0f;
            const float oscB = 2.0f * tonePhaseB[t] - 1.0f;
            const float toneGain = (t == (size_t) numTones - 1) ? params[voicingSpread].get() : 1.0f;

            sum += (oscA + oscB) * 0.5f * toneGain;
        }
        sum /= 3.0f;

        filter.setCutoffHz(params[cutoff].get() + envVal * params[filterEnvAmount].get());
        const float filtered = filter.processLowpass(sum);

        const float sample = filtered * envVal * params[level].get();

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addSample(ch, n, sample);
    }
}

} // namespace modulerack
