#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

namespace modulerack
{

/**
 * Compact resonant state-variable filter shared by Bass, Lead, Percussion and
 * Fx1 so each module doesn't reimplement its own. Not aiming for pristine
 * analog modelling -- just cheap and stable.
 *
 * The core is the topology-preserving (ZDF) form rather than the classic
 * Chamberlin one: Chamberlin's feedback coefficient is 2*sin(pi*fc/fs), which
 * passes 1.0 at a sixth of the sample rate and makes the filter blow up to
 * infinity above that. Fx1's cutoff knob alone reaches 12 kHz, so at 44.1 kHz
 * that limit is comfortably inside the range the knobs can ask for. The TPT
 * form below stays stable all the way to Nyquist at any resonance setting.
 */
class SimpleFilter
{
public:
    void prepare(double sampleRateIn)
    {
        sampleRate = sampleRateIn;
        updateCoefficients();
        reset();
    }

    void reset()
    {
        state1 = 0.0f;
        state2 = 0.0f;
    }

    void setCutoffHz(float hz)
    {
        const float clamped = juce::jlimit(20.0f, (float) (sampleRate * 0.45), hz);

        if (! juce::approximatelyEqual(clamped, cutoff))
        {
            cutoff = clamped;
            updateCoefficients();
        }
    }

    void setResonance01(float res01)
    {
        const float clamped = juce::jlimit(0.0f, 0.95f, res01);

        if (! juce::approximatelyEqual(clamped, resonance))
        {
            resonance = clamped;
            updateCoefficients();
        }
    }

    float processLowpass(float input) { return process(input).low; }

    /** Same state-variable core as processLowpass(), but returns the highpass output instead. */
    float processHighpass(float input) { return process(input).high; }

private:
    struct Outputs { float low, high, band; };

    void updateCoefficients()
    {
        g = std::tan(juce::MathConstants<float>::pi * cutoff / (float) sampleRate);
        k = juce::jmax(0.1f, 2.0f * (1.0f - resonance));
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    Outputs process(float input)
    {
        const float v3 = input - state2;
        const float v1 = a1 * state1 + a2 * v3;
        const float v2 = state2 + a2 * state1 + a3 * v3;

        state1 = 2.0f * v1 - state1;
        state2 = 2.0f * v2 - state2;

        return { v2, input - k * v1 - v2, v1 };
    }

    double sampleRate = 44100.0;
    float cutoff = 1000.0f;
    float resonance = 0.2f;

    float g = 0.0f, k = 2.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    float state1 = 0.0f;
    float state2 = 0.0f;
};

} // namespace modulerack
