#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>

// The modular in-loop FX rack: five concrete per-voice modules processed in a
// user-defined order inside the feedback loop's recursion. No virtual dispatch
// — a switch over a uint8 id inlines every module body. Bypassed modules are
// skipped entirely (zero cost) and reset on their enabled->disabled edge.
//
// Every module keeps its output bounded (<= |1|-ish) or is gain-neutral, so
// the loop's stability guarantee holds for any combination — backstopped by
// the hidden always-on limiter in TunedFeedbackLoop.

enum class LoopModuleID : juce::uint8
{
    filter = 0,
    saturator,
    echo,
    diffuser,
    ringmod,
};

inline constexpr int numLoopModules = 5;

// ---------------------------------------------------------------------------
struct DampingFilter
{
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        svf.prepare ({ sampleRate, 512, 1 });
    }

    void reset() { svf.reset(); }

    void setShape (bool bandpass, float q)
    {
        svf.setType (bandpass ? juce::dsp::StateVariableTPTFilterType::bandpass
                              : juce::dsp::StateVariableTPTFilterType::lowpass);
        svf.setResonance (juce::jmax (0.1f, q));
    }

    // Chunk-rate (ENV3 sweeps it).
    void setCutoff (float hz)
    {
        svf.setCutoffFrequency (juce::jlimit (20.0f, (float) sr * 0.45f, hz));
    }

    forcedinline float processSample (float x) noexcept
    {
        return svf.processSample (0, x);
    }

    double sr = 44100.0;
    juce::dsp::StateVariableTPTFilter<float> svf;
};

// ---------------------------------------------------------------------------
struct Saturator
{
    enum class Mode : int { tanhMode = 0, fold, clip };

    void reset() {}

    void set (Mode m, float driveDb)
    {
        mode = m;
        driveLin = juce::Decibels::decibelsToGain (driveDb);
    }

    // All three shapes have output ceiling +/-1 for any input => the loop's
    // recirculation stays bounded in every mode.
    forcedinline float processSample (float x) noexcept
    {
        const float d = driveLin * x;
        switch (mode)
        {
            case Mode::fold:
                // Sine fold: bounded, smooth, and each recirculation folds
                // again — chaotic-but-stable self-oscillation when cranked.
                return std::sin (juce::MathConstants<float>::halfPi * d);
            case Mode::clip:
                return juce::jlimit (-1.0f, 1.0f, d);
            case Mode::tanhMode:
            default:
                return std::tanh (d);
        }
    }

    Mode mode = Mode::tanhMode;
    float driveLin = 2.0f;
};

// ---------------------------------------------------------------------------
// Feedforward comb: y = (x + g*x[n-M]) / (1 + |g|). Normalized so max gain is
// exactly 1 — the loop's feedback calibration is untouched. (A series delay
// would lengthen the loop and shift its pitch; a feedback comb would multiply
// loop gain. Both rejected.)
struct LoopEcho
{
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        const juce::dsp::ProcessSpec spec { sampleRate, 512, 1 };
        delay.prepare (spec);
        delay.setMaximumDelayInSamples ((int) std::ceil (0.5 * sampleRate) + 8);
        maxDelay = (float) delay.getMaximumDelayInSamples() - 2.0f;
    }

    void reset() { delay.reset(); }

    void set (float delaySamples, float amount) // amount in [-1, 1]
    {
        m = juce::jlimit (1.0f, maxDelay, delaySamples);
        g = juce::jlimit (-1.0f, 1.0f, amount);
        norm = 1.0f / (1.0f + std::abs (g));
    }

    forcedinline float processSample (float x) noexcept
    {
        delay.pushSample (0, x);
        const float delayed = delay.popSample (0, m);
        return (x + g * delayed) * norm;
    }

    double sr = 44100.0;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay { 24008 };
    float m = 4800.0f, g = 0.5f, norm = 1.0f / 1.5f, maxDelay = 24000.0f;
};

// ---------------------------------------------------------------------------
// Four series Schroeder allpasses, prime base lengths (1.9-7.9 ms @48k).
// Allpasses are unity-magnitude at all frequencies, so the module is
// stability-neutral inside the outer loop; the phase delay it adds smears the
// loop resonance into inharmonic "reverb in the loop" modes (that's the
// point — Loop Tune compensates the detune by ear).
struct Diffuser
{
    static constexpr int numStages = 4;

    void prepare (double sampleRate)
    {
        static constexpr int baseLen48k[numStages] = { 89, 149, 233, 379 };
        const double scale = sampleRate / 48000.0;
        for (int s = 0; s < numStages; ++s)
        {
            maxLen[s] = juce::jmax (8, (int) std::lround (baseLen48k[s] * scale));
            buffers[s].assign ((size_t) maxLen[s], 0.0f);
        }
        reset();
    }

    void reset()
    {
        for (auto& b : buffers)
            std::fill (b.begin(), b.end(), 0.0f);
        for (auto& w : writeIdx)
            w = 0;
    }

    void set (float size01, float diffuse01)
    {
        for (int s = 0; s < numStages; ++s)
            len[s] = juce::jlimit (2, maxLen[s], (int) std::lround (size01 * (float) maxLen[s]));
        a = 0.7f * juce::jlimit (0.0f, 1.0f, diffuse01);
    }

    forcedinline float processSample (float x) noexcept
    {
        for (int s = 0; s < numStages; ++s)
        {
            auto& buf = buffers[s];
            const int w = writeIdx[s];
            int r = w - len[s];
            if (r < 0)
                r += maxLen[s];

            const float z = buf[(size_t) r];
            const float v = x - a * z;
            buf[(size_t) w] = v;
            x = z + a * v;

            writeIdx[s] = w + 1 >= maxLen[s] ? 0 : w + 1;
        }
        return x;
    }

    std::vector<float> buffers[numStages]; // sized in prepare, never after
    int maxLen[numStages] { 8, 8, 8, 8 };
    int len[numStages] { 4, 4, 4, 4 };
    int writeIdx[numStages] {};
    float a = 0.35f;
};

// ---------------------------------------------------------------------------
// Ring mod via complex-phasor rotation. Inside a recirculating loop, repeated
// double-sideband multiplication walks energy outward through +/-k*f sidebands
// — the barberpole/metallic spiral. (True SSB Hilbert shifter is a tracked v2.)
struct RingMod
{
    void reset()
    {
        c = 1.0f;
        s = 0.0f;
    }

    // Chunk-rate: rotation increment + renormalize the phasor.
    void set (float shiftHz, float mix01, double sampleRate)
    {
        const float w = juce::MathConstants<float>::twoPi * shiftHz / (float) sampleRate;
        dc = std::cos (w);
        ds = std::sin (w);
        mix = juce::jlimit (0.0f, 1.0f, mix01);

        const float mag = c * c + s * s;
        if (mag > 0.0f)
        {
            const float inv = 1.0f / std::sqrt (mag);
            c *= inv;
            s *= inv;
        }
    }

    forcedinline float processSample (float x) noexcept
    {
        const float out = (1.0f - mix) * x + mix * x * c;
        const float nc = c * dc - s * ds;
        s = c * ds + s * dc;
        c = nc;
        return out;
    }

    float c = 1.0f, s = 0.0f;
    float dc = 1.0f, ds = 0.0f;
    float mix = 0.5f;
};

// ---------------------------------------------------------------------------
class LoopFxChain
{
public:
    void prepare (double sampleRate)
    {
        filter.prepare (sampleRate);
        echo.prepare (sampleRate);
        diffuser.prepare (sampleRate);
        reset();
    }

    void reset()
    {
        filter.reset();
        saturator.reset();
        echo.reset();
        diffuser.reset();
        ringmod.reset();
    }

    // Block-rate. Resets a module when it transitions enabled -> disabled so
    // re-enabling never replays stale state.
    void setEnabled (juce::uint8 mask)
    {
        const juce::uint8 turnedOff = (juce::uint8) (enabledMask & ~mask);
        if (turnedOff & bit (LoopModuleID::filter))    filter.reset();
        if (turnedOff & bit (LoopModuleID::saturator)) saturator.reset();
        if (turnedOff & bit (LoopModuleID::echo))      echo.reset();
        if (turnedOff & bit (LoopModuleID::diffuser))  diffuser.reset();
        if (turnedOff & bit (LoopModuleID::ringmod))   ringmod.reset();
        enabledMask = mask;
    }

    void setOrder (const std::array<juce::uint8, numLoopModules>& newOrder)
    {
        order = newOrder;
    }

    static constexpr juce::uint8 bit (LoopModuleID id)
    {
        return (juce::uint8) (1u << (int) id);
    }

    forcedinline float processSample (float x) noexcept
    {
        for (int slot = 0; slot < numLoopModules; ++slot)
        {
            const auto m = order[(size_t) slot];
            if ((enabledMask & (1u << m)) == 0)
                continue;

            switch ((LoopModuleID) m)
            {
                case LoopModuleID::filter:    x = filter.processSample (x); break;
                case LoopModuleID::saturator: x = saturator.processSample (x); break;
                case LoopModuleID::echo:      x = echo.processSample (x); break;
                case LoopModuleID::diffuser:  x = diffuser.processSample (x); break;
                case LoopModuleID::ringmod:   x = ringmod.processSample (x); break;
                default: break;
            }
        }
        return x;
    }

    DampingFilter filter;
    Saturator saturator;
    LoopEcho echo;
    Diffuser diffuser;
    RingMod ringmod;

private:
    std::array<juce::uint8, numLoopModules> order { 0, 1, 2, 3, 4 };
    juce::uint8 enabledMask = bit (LoopModuleID::filter) | bit (LoopModuleID::saturator);
};
