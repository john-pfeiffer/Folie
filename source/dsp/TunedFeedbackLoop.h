#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>

// The signature of Folie: a Karplus-Strong-style feedback loop whose delay is
// tuned to the played note, so cranked feedback resonates *in key*. Inside the
// loop: damping filter (LP/BP TPT SVF) -> tanh saturator -> DC blocker.
//
// Per sample:  loopOut = dcBlock(tanh(drive * svf(delay[t - N])))
//              y       = dry + fbGain * loopOut
//              delay.push(y)
//
// tanh bounds the recirculating signal for ANY input, which is what lets the
// feedback knob go past 1.0 (110%) without the loop blowing up — it settles
// into a bounded self-oscillation instead, like a bowed string.
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
        svf.prepare (spec);

        dcCoeff = 1.0f - juce::MathConstants<float>::twoPi * 20.0f / (float) sampleRate;
        reset();
    }

    void reset()
    {
        delay.reset();
        svf.reset();
        dcX1 = dcY1 = 0.0f;
    }

    // hz is the desired loop resonance; clamped so the delay stays within the
    // interpolator's happy range. True pitch sits slightly flat of hz (filter
    // phase delay is uncompensated in v1) — the Loop Tune knob covers it.
    void setLoopFrequency (float hz)
    {
        const float clamped = juce::jlimit (minLoopHz, maxLoopHz, hz);
        // -1: processSample pops before pushing, which adds one sample of
        // loop latency on top of the delay-line setting.
        delaySamples = juce::jmax (minDelaySamples, (float) sr / clamped - 1.0f);
    }

    void setFilter (bool bandpass, float cutoffHz, float q)
    {
        svf.setType (bandpass ? juce::dsp::StateVariableTPTFilterType::bandpass
                              : juce::dsp::StateVariableTPTFilterType::lowpass);
        setCutoff (cutoffHz);
        svf.setResonance (juce::jmax (0.1f, q));
    }

    // Cheap enough to call at chunk rate while ENV3 sweeps it.
    void setCutoff (float cutoffHz)
    {
        svf.setCutoffFrequency (juce::jlimit (20.0f, (float) sr * 0.45f, cutoffHz));
    }

    void setDrive (float driveDb)
    {
        driveLin = juce::Decibels::decibelsToGain (driveDb);
    }

    // Returns y = dry + fbGain * loopOut; y is also what recirculates.
    // fbGain is per-sample: the caller owns smoothing/enveloping (ENV2).
    forcedinline float processSample (float dry, float fbGain) noexcept
    {
        const float delayed = delay.popSample (0, delaySamples);
        const float filtered = svf.processSample (0, delayed);

        // Normalized tanh: small-signal loop gain stays unity as drive rises,
        // so Drive adds color/compression without re-calibrating the Feedback
        // knob's danger zone.
        const float shaped = std::tanh (driveLin * filtered) / driveLin;

        // In-loop DC blocker (~20 Hz one-pole HP). Mandatory: recirculated DC
        // from asymmetric saturation would walk the loop into tanh's rails.
        const float dcOut = shaped - dcX1 + dcCoeff * dcY1;
        dcX1 = shaped;
        dcY1 = dcOut;

        const float y = dry + fbGain * dcOut;
        delay.pushSample (0, y);
        return y;
    }

private:
    double sr = 44100.0;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delay { 4800 };
    juce::dsp::StateVariableTPTFilter<float> svf;

    float delaySamples = 100.0f;
    float driveLin = 1.0f;
    float dcCoeff = 0.997f;
    float dcX1 = 0.0f, dcY1 = 0.0f;
};
