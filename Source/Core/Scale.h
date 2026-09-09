#pragma once

#include <array>
#include <cmath>

namespace modulerack
{

/** Minimal scale/quantiser helper shared by the generative modules (Bass, Lead, Pad, Arp). */
struct Scale
{
    enum class Type
    {
        minorPentatonic,
        naturalMinor,
        dorian
    };

    static const std::array<int, 5>& pentatonicIntervals()
    {
        static constexpr std::array<int, 5> v { 0, 3, 5, 7, 10 };
        return v;
    }

    static const std::array<int, 7>& naturalMinorIntervals()
    {
        static constexpr std::array<int, 7> v { 0, 2, 3, 5, 7, 8, 10 };
        return v;
    }

    static const std::array<int, 7>& dorianIntervals()
    {
        static constexpr std::array<int, 7> v { 0, 2, 3, 5, 7, 9, 10 };
        return v;
    }

    /** Maps an unbounded scale-degree index (can be negative) to a MIDI note number. */
    static int degreeToMidiNote(int degree, int rootNote, Type type)
    {
        switch (type)
        {
            case Type::naturalMinor: return degreeToMidiNoteImpl(degree, rootNote, naturalMinorIntervals());
            case Type::dorian:       return degreeToMidiNoteImpl(degree, rootNote, dorianIntervals());
            case Type::minorPentatonic:
            default:                 return degreeToMidiNoteImpl(degree, rootNote, pentatonicIntervals());
        }
    }

private:
    template <size_t N>
    static int degreeToMidiNoteImpl(int degree, int rootNote, const std::array<int, N>& intervals)
    {
        const int size = (int) N;
        int octave = degree >= 0 ? degree / size : (degree - size + 1) / size;
        int index = degree - octave * size;
        return rootNote + octave * 12 + intervals[(size_t) index];
    }
};

} // namespace modulerack
