#pragma once

#include "SupersawOscillator.h"
#include "TunedFeedbackLoop.h"

// Parameters fanned out from the APVTS once per block (plain values — the
// audio thread never touches parameter objects directly).
struct VoiceParams
{
    int   sawCount   = 7;
    float detune     = 0.25f;   // 0..1
    float blend      = 0.6f;    // 0..1
    float width      = 0.8f;    // 0..1
    int   octave     = 0;       // -2..+2

    float fbGain      = 0.4f;   // 0..1.1 — past 1.0 is "past the edge"
    float fbKeytrack  = 1.0f;   // 0..1; 0 = loop pinned to fbBaseHz
    float fbTuneSemis = 0.0f;   // -24..+24
    bool  fbBandpass  = false;
    float fbCutoff    = 4000.0f;
    float fbReso      = 0.71f;
    float fbDriveDb   = 0.0f;

    float env1AttackMs  = 5.0f;
    float env1DecayMs   = 200.0f;
    float env1Sustain   = 0.8f; // 0..1
    float env1ReleaseMs = 300.0f;

    // ENV2 -> feedback gain: fbEff = fbGain * ((1 - amt) + amt * env2)
    float env2AttackMs  = 5.0f;
    float env2DecayMs   = 400.0f;
    float env2Sustain   = 1.0f;
    float env2ReleaseMs = 300.0f;
    float env2Amount    = 0.0f; // 0..1

    // ENV3 -> loop cutoff: cutEff = fbCutoff * 2^(5 * amt * env3), amt bipolar
    float env3AttackMs  = 5.0f;
    float env3DecayMs   = 600.0f;
    float env3Sustain   = 1.0f;
    float env3ReleaseMs = 300.0f;
    float env3Amount    = 0.0f; // -1..+1

    static constexpr float fbBaseHz = 261.63f; // loop anchor at 0% keytrack
};

// One synth voice: supersaw stack + amp ADSR + glide. The tuned feedback loop
// (FOL-5) and ENV2/ENV3 (FOL-6) slot in here. Not a juce::SynthesiserVoice —
// voice management is hand-rolled in SynthEngine so stealing and (later)
// feedback-tail keepalive stay under our control with zero audio-thread
// allocation.
class FolieVoice
{
public:
    void prepare (double sampleRate, int voiceIndex)
    {
        sr = sampleRate;
        rng.setSeed ((juce::int64) 0x466f6c69 + voiceIndex);
        osc.prepare (sampleRate);
        loop.prepare (sampleRate);
        env1.setSampleRate (sampleRate);
        env2.setSampleRate (sampleRate);
        env3.setSampleRate (sampleRate);
        fbBaseSmoothed.reset (sampleRate, 0.01);
        smoothedFreq.reset (sampleRate, 0.0);
        reset();
    }

    void reset()
    {
        env1.reset();
        env2.reset();
        env3.reset();
        loop.reset();
        note = -1;
        lastEnvLevel = 0.0f;
        fbBaseSmoothed.setCurrentAndTargetValue (fbBaseSmoothed.getTargetValue());
    }

    void setParams (const VoiceParams& p)
    {
        params = p;
        osc.setParams (p.sawCount, p.detune, p.blend, p.width);
        loop.setFilter (p.fbBandpass, p.fbCutoff, p.fbReso);
        loop.setDrive (p.fbDriveDb);
        fbBaseSmoothed.setTargetValue (p.fbGain);
        env1.setParameters ({ p.env1AttackMs * 0.001f,
                              p.env1DecayMs * 0.001f,
                              p.env1Sustain,
                              p.env1ReleaseMs * 0.001f });
        env2.setParameters ({ p.env2AttackMs * 0.001f,
                              p.env2DecayMs * 0.001f,
                              p.env2Sustain,
                              p.env2ReleaseMs * 0.001f });
        env3.setParameters ({ p.env3AttackMs * 0.001f,
                              p.env3DecayMs * 0.001f,
                              p.env3Sustain,
                              p.env3ReleaseMs * 0.001f });
    }

    void startNote (int midiNote, float velocity, float glideFromHz, float glideSeconds)
    {
        note = midiNote;
        level = 0.25f * (0.3f + 0.7f * velocity);

        const auto target = noteHz (midiNote);
        smoothedFreq.reset (sr, glideFromHz > 0.0f ? glideSeconds : 0.0);
        smoothedFreq.setCurrentAndTargetValue (glideFromHz > 0.0f ? glideFromHz : target);
        smoothedFreq.setTargetValue (target);

        osc.noteOn (rng);
        env1.noteOn();
        env2.noteOn();
        env3.noteOn();
    }

    void stopNote (bool allowTailOff)
    {
        if (allowTailOff)
        {
            env1.noteOff();
            env2.noteOff();
            env3.noteOff();
        }
        else
        {
            reset();
        }
    }

    void setPitchBend (float semitones) { bendSemis = semitones; }

    bool isActive() const { return note >= 0 && env1.isActive(); }
    bool isReleasing() const { return isActive() && ! heldDown; }
    int currentNote() const { return note; }
    float envLevel() const { return lastEnvLevel; }

    void setHeld (bool h) { heldDown = h; }
    bool isHeld() const { return heldDown; }

    // Adds into the buffer. Frequency-derived state updates per sub-chunk
    // (~0.7 ms) — cheap and inaudible for glide; envelope runs per sample.
    void renderNextBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
    {
        if (! isActive())
            return;

        auto* left = buffer.getWritePointer (0);
        auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : left;

        constexpr int chunkSize = 32;
        int remaining = numSamples;
        int pos = startSample;

        while (remaining > 0 && isActive())
        {
            const int n = juce::jmin (chunkSize, remaining);

            smoothedFreq.skip (n - 1);
            const float freq = smoothedFreq.getNextValue()
                               * std::exp2 ((float) params.octave + bendSemis / 12.0f);
            osc.setFrequency (freq);
            osc.updateDerived();

            // Loop tuning tracks the sounding pitch (post-glide/octave/bend),
            // interpolated in the log domain toward the fixed anchor at 0%
            // keytrack, plus the tune offset.
            const float loopHz = std::exp2 (params.fbKeytrack * std::log2 (freq)
                                            + (1.0f - params.fbKeytrack)
                                                  * std::log2 (VoiceParams::fbBaseHz)
                                            + params.fbTuneSemis / 12.0f);
            loop.setLoopFrequency (loopHz);

            // ENV3 sweeps the damping cutoff. The envelope advances per
            // sample (below); the filter coefficient updates at chunk rate,
            // which the TPT structure tolerates and keeps tan() off the
            // per-sample path.
            if (std::abs (params.env3Amount) > 1.0e-4f)
                loop.setCutoff (params.fbCutoff
                                * std::exp2 (5.0f * params.env3Amount * env3Level));

            for (int i = 0; i < n; ++i)
            {
                float l, r;
                osc.processSample (l, r);

                // ENV2 blends the feedback gain between its static knob value
                // and the fully-enveloped value.
                const float fbBase = fbBaseSmoothed.getNextValue();
                const float env2Sample = env2.getNextSample();
                const float fbEff = fbBase * ((1.0f - params.env2Amount)
                                              + params.env2Amount * env2Sample);
                env3Level = env3.getNextSample();

                // The loop runs on the mono sum; its return is added equally
                // L/R (mono loop per voice — dual stereo loops is a tracked
                // v2 idea).
                const float mono = 0.5f * (l + r);
                const float fbComponent = loop.processSample (mono, fbEff) - mono;

                const float amp = level * env1.getNextSample();
                lastEnvLevel = amp;

                left[pos + i] += (l + fbComponent) * amp;
                right[pos + i] += (r + fbComponent) * amp;
            }

            if (! env1.isActive())
            {
                reset();
                break;
            }

            pos += n;
            remaining -= n;
        }
    }

    static float noteHz (int midiNote)
    {
        return 440.0f * std::exp2 ((float) (midiNote - 69) / 12.0f);
    }

private:
    double sr = 44100.0;
    juce::Random rng;

    SupersawOscillator osc;
    TunedFeedbackLoop loop;
    juce::ADSR env1, env2, env3;
    juce::SmoothedValue<float> fbBaseSmoothed { 0.0f };
    float env3Level = 0.0f;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedFreq { 440.0f };

    VoiceParams params;
    int note = -1;
    bool heldDown = false;
    float level = 0.0f;
    float bendSemis = 0.0f;
    float lastEnvLevel = 0.0f;
};
