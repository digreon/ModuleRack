#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace modulerack
{

/**
 * Shared 16th-note swing helper for the rhythmic modules. An off-beat 16th is
 * held back by `amount` x half a 16th note before it fires, which is what a
 * shuffle/swing control does on a drum machine.
 *
 * `PayloadType` is whatever the module needs to remember about the held-back
 * hit while it waits (Percussion carries its open/accent flags; Kick has
 * nothing to carry and uses an empty struct).
 */
template <typename PayloadType>
class SwingDelay
{
public:
    void prepare(double sampleRateToUse)
    {
        sampleRate = sampleRateToUse;
        reset();
    }

    void reset() { countdown = -1; }

    /** How many samples an off-beat step should be pushed back at this tempo. */
    int delaySamples(float amount01, float bpm) const
    {
        const double secondsPerSixteenth = 60.0 / (double) juce::jmax(1.0f, bpm) / 4.0;
        return (int) (amount01 * secondsPerSixteenth * sampleRate * 0.5);
    }

    /** Holds `payloadToUse` back by `delayInSamples` samples (0 fires on this sample). */
    void schedule(PayloadType payloadToUse, int delayInSamples)
    {
        payload = payloadToUse;
        countdown = juce::jmax(0, delayInSamples);
    }

    /** Call once per sample after handling ticks; true when the held hit is due now. */
    bool advance(PayloadType& firedPayload)
    {
        if (countdown < 0)
            return false;

        if (countdown == 0)
        {
            countdown = -1;
            firedPayload = payload;
            return true;
        }

        --countdown;
        return false;
    }

private:
    double sampleRate = 44100.0;
    int countdown = -1;
    PayloadType payload {};
};

} // namespace modulerack
