#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace modulerack
{

/**
 * Shared sample-accurate 16th-note clock. SignalGraph advances it once per audio
 * block, before any module processes; every generative module (Kick, Bass, Lead,
 * Pad, Arp, Percussion) only ever reads ticksThisBlock() to know when to fire.
 * This is what makes all 8 modules play together as layers of one composition
 * instead of needing to be triggered individually from pad presses.
 *
 * Tick positions are accumulated in double precision and never rounded to whole
 * samples, so the grid does not drift away from the nominal tempo however many
 * blocks have gone by, and a tick that lands on the very last sample of a block
 * is still reported inside that block rather than being dropped.
 */
class Clock
{
public:
    struct Tick
    {
        int sampleOffset;   // always within [0, numSamples) of the block just advanced
        int sixteenthIndex; // 0-15, wraps every bar (4/4, 16th-note resolution)
    };

    /** maximumBlockSize is only used to size the tick list up front, so that
        advanceBlock() never allocates on the audio thread. */
    void prepare(double sampleRateIn, int maximumBlockSize)
    {
        sampleRate = sampleRateIn;
        recalcSamplesPerTick();

        // Worst case: the fastest tempo at the lowest sample rate still gives well
        // over 100 samples per 16th, but reserve generously - this is startup code.
        ticks.reserve((size_t) juce::jmax(8, maximumBlockSize / 16 + 8));

        samplesUntilNextTick = 0.0;
        tickIndex = 0;
    }

    void setBpm(float newBpm)
    {
        bpm = juce::jlimit(20.0f, 300.0f, newBpm);
        recalcSamplesPerTick();

        // A tempo increase must not leave a longer wait pending than the new
        // tick length, or the next tick would arrive late by up to one old tick.
        samplesUntilNextTick = juce::jmin(samplesUntilNextTick, samplesPerTick);
    }

    float getBpm() const { return bpm; }

    /** Call once per processBlock, before iterating modules. */
    void advanceBlock(int numSamples)
    {
        ticks.clear();

        if (samplesPerTick <= 0.0 || numSamples <= 0)
            return;

        double position = samplesUntilNextTick;

        while (position < (double) numSamples)
        {
            ticks.push_back({ (int) position, tickIndex });
            tickIndex = (tickIndex + 1) % 16;
            position += samplesPerTick;
        }

        samplesUntilNextTick = position - (double) numSamples;
    }

    const std::vector<Tick>& ticksThisBlock() const { return ticks; }

    /** Index of the next 16th to fire, useful outside of a tick (e.g. UI). */
    int getCurrentSixteenthIndex() const { return tickIndex; }

private:
    void recalcSamplesPerTick()
    {
        const double secondsPerSixteenth = 60.0 / (double) bpm / 4.0;
        samplesPerTick = secondsPerSixteenth * sampleRate;
    }

    double sampleRate = 44100.0;
    float bpm = 120.0f;
    double samplesPerTick = 0.0;
    double samplesUntilNextTick = 0.0;
    int tickIndex = 0;
    std::vector<Tick> ticks;
};

} // namespace modulerack
