#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>

// Parameters fanned out from the APVTS once per block, same pattern as
// VoiceParams.
struct FxParams
{
    bool  chorusOn     = false;
    float chorusRateHz = 0.8f;
    float chorusDepth  = 0.3f;  // 0..1
    float chorusMix    = 0.5f;  // 0..1

    bool  delayOn       = false;
    float delayTimeMs   = 350.0f;
    float delayFeedback = 0.35f; // 0..0.95
    float delayMix      = 0.25f; // 0..1

    bool  reverbOn   = false;
    float reverbSize = 0.5f;    // 0..1
    float reverbDamp = 0.5f;    // 0..1
    float reverbMix  = 0.2f;    // 0..1
};

// Macro-style finishing chain: chorus -> delay -> reverb. Deliberately not a
// full FX rack. The always-on limiter lives at the very end of the processor's
// chain, after master gain. "On" toggles work by smoothing the wet level to
// zero, so switching never clicks and delay/reverb tails ring out silently.
class FxChain
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sr = spec.sampleRate;

        chorus.prepare (spec);
        chorus.setCentreDelay (7.0f);
        chorus.setFeedback (0.0f);

        const int maxDelaySamples = (int) std::ceil (2.1 * sr) + 8;
        for (auto* d : { &delayLineL, &delayLineR })
        {
            d->prepare ({ spec.sampleRate, spec.maximumBlockSize, 1 });
            d->setMaximumDelayInSamples (maxDelaySamples);
        }

        reverb.prepare (spec);

        chorusMixSmoothed.reset (sr, 0.02);
        delayWetSmoothed.reset (sr, 0.02);
        // Long ramp on delay time: knob moves slew the pitch tape-style
        // instead of zippering.
        delayTimeSmoothed.reset (sr, 0.1);
        reset();
    }

    void reset()
    {
        chorus.reset();
        delayLineL.reset();
        delayLineR.reset();
        reverb.reset();
    }

    void setParams (const FxParams& p)
    {
        params = p;

        chorus.setRate (p.chorusRateHz);
        chorus.setDepth (p.chorusDepth);
        chorusMixSmoothed.setTargetValue (p.chorusOn ? p.chorusMix : 0.0f);

        delayWetSmoothed.setTargetValue (p.delayOn ? p.delayMix : 0.0f);
        delayTimeSmoothed.setTargetValue (p.delayTimeMs * 0.001f * (float) sr);

        // juce::Reverb applies internal scale factors (dry x2, wet x3), so
        // dryLevel 0.5 == unity passthrough and wet is scaled to taste.
        juce::dsp::Reverb::Parameters rp;
        rp.roomSize = p.reverbSize;
        rp.damping = p.reverbDamp;
        rp.wetLevel = p.reverbOn ? p.reverbMix * 0.33f : 0.0f;
        rp.dryLevel = 0.5f;
        rp.width = 1.0f;
        reverb.setParameters (rp);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        jassert (buffer.getNumChannels() >= 1);

        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> context (block);

        // Chorus (mix 0 == exact dry passthrough inside juce::dsp::Chorus).
        chorus.setMix (chorusMixSmoothed.getNextValue());
        chorusMixSmoothed.skip (buffer.getNumSamples() - 1);
        chorus.process (context);

        // Stereo delay, per sample for smooth time/wet ramps.
        auto* l = buffer.getWritePointer (0);
        auto* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : l;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float delaySamples = delayTimeSmoothed.getNextValue();
            const float wet = delayWetSmoothed.getNextValue();

            const float dL = delayLineL.popSample (0, delaySamples);
            const float dR = delayLineR.popSample (0, delaySamples);

            delayLineL.pushSample (0, l[i] + dL * params.delayFeedback);
            delayLineR.pushSample (0, r[i] + dR * params.delayFeedback);

            l[i] += wet * dL;
            r[i] += wet * dR;
        }

        // Reverb (wet 0 when off; dry always 1).
        reverb.process (context);
    }

private:
    double sr = 44100.0;
    FxParams params;

    juce::dsp::Chorus<float> chorus;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLineL { 96000 },
                                                                                delayLineR { 96000 };
    juce::dsp::Reverb reverb;

    juce::SmoothedValue<float> chorusMixSmoothed { 0.0f };
    juce::SmoothedValue<float> delayWetSmoothed { 0.0f };
    juce::SmoothedValue<float> delayTimeSmoothed { 16800.0f };
};
