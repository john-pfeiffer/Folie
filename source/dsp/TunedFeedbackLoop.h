#pragma once

#include "LoopModules.h"

// The signature of Folie: a Karplus-Strong-style feedback loop whose delay is
// tuned to the played note, so cranked feedback resonates *in key*. Inside the
// loop sits the modular FX rack (LoopFxChain) — filter, saturator, echo,
// diffuser, ring mod, in user order — followed by two hidden, always-on
// safety stages:
//
//   - loop limiter: transparent below +/-1, soft knee to a +/-1.5 ceiling.
//     With the saturator bypassable, this is what keeps feedback > 100% a
//     bounded self-oscillation instead of an exponential blowup.
//   - DC blocker: recirculated DC from asymmetric nonlinearities would walk
//     the loop into the rails.
//
// Per sample:  x = dcBlock(limit(chain(delay[t - N])))
//              y = dry + fbGain * x;  delay.push(y)
class TunedFeedbackLoop
{
public:
    static constexpr float minLoopHz = 20.0f;
    static constexpr float maxLoopHz = 8000.0f; // top notes degrade gracefully
    static constexpr float minDelaySamples = 4.0f; // Lagrange3rd needs headroom

    void prepare (double sampleRate)
    {
        sr = sampleRate;

        const juce::dsp::ProcessSpec spec { sampleRate, 512, 1 };
        delay.prepare (spec);
        delay.setMaximumDelayInSamples ((int) std::ceil (sampleRate / (double) minLoopHz) + 8);
        chain.prepare (sampleRate);

        dcCoeff = 1.0f - juce::MathConstants<float>::twoPi * 20.0f / (float) sampleRate;
        reset();
    }

    void reset()
    {
        delay.reset();
        chain.reset();
        dcX1 = dcY1 = 0.0f;
    }

    // hz is the desired loop resonance; clamped so the delay stays within the
    // interpolator's happy range. True pitch sits slightly flat of hz (in-loop
    // phase delay is uncompensated) — the Loop Tune knob covers it.
    void setLoopFrequency (float hz)
    {
        const float clamped = juce::jlimit (minLoopHz, maxLoopHz, hz);
        // -1: processSample pops before pushing, which adds one sample of
        // loop latency on top of the delay-line setting.
        delaySamples = juce::jmax (minDelaySamples, (float) sr / clamped - 1.0f);
    }

    float getDelaySamples() const { return delaySamples; }
    double getSampleRate() const { return sr; }

    LoopFxChain& fx() { return chain; }

    // Returns y = dry + fbGain * loopOut; y is also what recirculates.
    // fbGain is per-sample: the caller owns smoothing/enveloping (ENV2).
    forcedinline float processSample (float dry, float fbGain) noexcept
    {
        const float delayed = delay.popSample (0, delaySamples);

        float x = chain.processSample (delayed);
        x = loopLimit (x);

        // In-loop DC blocker (~20 Hz one-pole HP). Mandatory.
        const float dcOut = x - dcX1 + dcCoeff * dcY1;
        dcX1 = x;
        dcY1 = dcOut;

        const float y = dry + fbGain * dcOut;
        delay.pushSample (0, y);
        return y;
    }

private:
    // Hidden safety: bit-exact below +/-1, tanh knee up to a +/-1.5 ceiling.
    static forcedinline float loopLimit (float x) noexcept
    {
        constexpr float knee = 1.0f;
        const float ax = std::abs (x);
        if (ax <= knee)
            return x;
        const float shaped = knee + 0.5f * std::tanh ((ax - knee) * 2.0f);
        return x > 0.0f ? shaped : -shaped;
    }

    double sr = 44100.0;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delay { 4800 };
    LoopFxChain chain;

    float delaySamples = 100.0f;
    float dcCoeff = 0.997f;
    float dcX1 = 0.0f, dcY1 = 0.0f;
};
