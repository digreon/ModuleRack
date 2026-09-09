#pragma once

#include <array>

namespace modulerack
{

/**
 * Bjorklund-style Euclidean rhythm generator, fixed at a 16-step (one bar of
 * 16th notes) resolution. Used by Kick and Percussion to turn a single
 * "density" knob into an evenly-spread hit pattern, the way Euclidean
 * sequencers on Eurorack modules (e.g. Pamela's New Workout) typically work.
 */
inline std::array<bool, 16> computeEuclideanPattern(int hits)
{
    std::array<bool, 16> pattern {};
    hits = hits < 0 ? 0 : (hits > 16 ? 16 : hits);

    if (hits == 0)
        return pattern;

    // Evenly distribute `hits` onto 16 steps: step i is a hit when crossing a
    // hits/16 boundary. This is a simple, well-known approximation of the
    // full Bjorklund algorithm that is more than adequate for a 16-step grid.
    int previousBucket = -1;
    for (int i = 0; i < 16; ++i)
    {
        const int bucket = (i * hits) / 16;
        if (bucket != previousBucket)
        {
            pattern[(size_t) i] = true;
            previousBucket = bucket;
        }
    }

    return pattern;
}

} // namespace modulerack
