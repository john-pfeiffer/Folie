#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

// Stack of 1..16 detuned polyBLEP saws with blend (center vs sides), stereo
// width (alternating pan), and a superlinear detune curve in the JP-8000
// spirit. polyBLEP keeps the whole voice at 1x rate, which matters once the
// tuned feedback loop runs per-sample in the same voice.
class SupersawOscillator
{
public:
    static constexpr int maxSaws = 16;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        updateDerived();
    }

    // Safe to call per block; derived state is recomputed lazily per chunk.
    void setParams (int numSawsIn, float detune01, float blend01, float width01)
    {
        numSaws = juce::jlimit (1, maxSaws, numSawsIn);
        detune  = juce::jlimit (0.0f, 1.0f, detune01);
        blend   = juce::jlimit (0.0f, 1.0f, blend01);
        width   = juce::jlimit (0.0f, 1.0f, width01);
        derivedDirty = true;
    }

    void setFrequency (float hz)
    {
        baseFreq = hz;
        derivedDirty = true;
    }

    void noteOn (juce::Random& rng)
    {
        // Random initial phases: the classic supersaw "ensemble" thickness,
        // and no two note-ons phase-cancel the same way twice.
        for (auto& p : phase)
            p = rng.nextFloat();
        derivedDirty = true;
    }

    // Call at chunk boundaries (every ~32 samples) after frequency changes.
    void updateDerived()
    {
        if (! derivedDirty && juce::approximatelyEqual (lastDerivedFreq, baseFreq))
            return;

        // Superlinear detune curve: gentle at the bottom of the knob, steep at
        // the top (JP-8000-style feel; exact Szabo polynomial fit is a tracked
        // later refinement). Max spread ~= +/-11% at full detune.
        const float curved = 0.2f * detune + 0.8f * detune * detune * detune;
        const float spread = 0.11f * curved;

        float sumSquares = 0.0f;
        for (int i = 0; i < numSaws; ++i)
        {
            // Positions in [-1, 1]; single saw sits at center.
            const float pos = numSaws == 1
                                  ? 0.0f
                                  : -1.0f + 2.0f * (float) i / (float) (numSaws - 1);

            const float freq = baseFreq * (1.0f + pos * spread);
            increment[i] = juce::jlimit (0.0f, 0.45f, (float) (freq / sr));

            // Center saw(s) full level, outer saws scaled by blend.
            const float centerness = 1.0f - std::abs (pos);
            const float gain = juce::jmap (blend, centerness, 1.0f);

            // Equal-power pan by position * width.
            const float pan = pos * width; // [-1, 1]
            const float angle = juce::MathConstants<float>::halfPi * 0.5f * (pan + 1.0f);
            gainL[i] = gain * std::cos (angle);
            gainR[i] = gain * std::sin (angle);

            sumSquares += gain * gain;
        }

        // Keep perceived level roughly constant as saw count / blend change.
        const float norm = 1.0f / std::sqrt (juce::jmax (1.0f, sumSquares));
        for (int i = 0; i < numSaws; ++i)
        {
            gainL[i] *= norm;
            gainR[i] *= norm;
        }

        lastDerivedFreq = baseFreq;
        derivedDirty = false;
    }

    forcedinline void processSample (float& outL, float& outR) noexcept
    {
        float l = 0.0f, r = 0.0f;

        for (int i = 0; i < numSaws; ++i)
        {
            const float dt = increment[i];
            float t = phase[i];

            float saw = 2.0f * t - 1.0f - polyBlep (t, dt);

            l += saw * gainL[i];
            r += saw * gainR[i];

            t += dt;
            if (t >= 1.0f)
                t -= 1.0f;
            phase[i] = t;
        }

        outL = l;
        outR = r;
    }

private:
    static forcedinline float polyBlep (float t, float dt) noexcept
    {
        if (dt <= 0.0f)
            return 0.0f;

        if (t < dt)
        {
            const float x = t / dt;
            return x + x - x * x - 1.0f;
        }

        if (t > 1.0f - dt)
        {
            const float x = (t - 1.0f) / dt;
            return x * x + x + x + 1.0f;
        }

        return 0.0f;
    }

    double sr = 44100.0;
    float baseFreq = 440.0f, lastDerivedFreq = 0.0f;
    int numSaws = 7;
    float detune = 0.25f, blend = 0.6f, width = 0.8f;
    bool derivedDirty = true;

    float phase[maxSaws] {};
    float increment[maxSaws] {};
    float gainL[maxSaws] {};
    float gainR[maxSaws] {};
};
