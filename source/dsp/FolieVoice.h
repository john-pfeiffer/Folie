#pragma once

#include "NoiseSource.h"
#include "SampleData.h"
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

    // Source mixer
    float srcSawLevel   = 1.0f; // 0..1
    float srcNoiseLevel = 0.0f; // 0..1
    bool  srcNoisePink  = false;
    float srcSampleLevel = 0.0f; // 0..1
    int   srcSampleRoot  = 60;   // MIDI note the clip plays back unpitched at
    bool  srcSampleLoop  = false;
    // Snapshot taken once per block by the processor; lifetime guaranteed by
    // its publish/retire scheme.
    const SampleData* sample = nullptr;

    // Defaults mirror the parameter-layout defaults ("showcase" tuning).
    float fbGain      = 0.75f;  // 0..1.1 — past 1.0 is "past the edge"
    float fbKeytrack  = 1.0f;   // 0..1; 0 = loop pinned to fbBaseHz
    float fbTuneSemis = 0.0f;   // -24..+24
    bool  fbBandpass  = false;
    float fbCutoff    = 4000.0f;
    float fbReso      = 1.5f;
    float fbDriveDb   = 6.0f;

    // Loop FX rack
    bool fxFilterOn = true;
    bool fxSatOn    = true;
    int  fxSatMode  = 0;        // 0 tanh, 1 fold, 2 clip
    bool  fxEchoOn   = false;
    int   fxEchoSync = 0;       // 0 free; 1..5 = x2 x3 x4 x6 x8 of loop delay
    float fxEchoTimeMs = 120.0f;
    float fxEchoAmt  = 0.5f;    // -1..1
    bool  fxDiffOn   = false;
    float fxDiffSize = 0.5f;    // 0.1..1
    float fxDiffAmt  = 0.5f;    // 0..1
    bool  fxRingOn   = false;
    int   fxRingMode = 0;       // 0 Hz, 1 track
    float fxRingHz   = 8.0f;
    float fxRingRatio = 0.5f;
    float fxRingMix  = 0.5f;
    std::array<juce::uint8, numLoopModules> loopOrder { 0, 1, 2, 3, 4 };

    float env1AttackMs  = 5.0f;
    float env1DecayMs   = 200.0f;
    float env1Sustain   = 0.8f; // 0..1
    float env1ReleaseMs = 300.0f;

    // ENV2 -> feedback gain, additive per spec: fbEff = fbGain + amt * env2.
    // Defaults give every note a feedback bloom that settles (audible out of
    // the box; peaks ~105% clamped, relaxes to ~85%).
    float env2AttackMs  = 5.0f;
    float env2DecayMs   = 400.0f;
    float env2Sustain   = 0.35f;
    float env2ReleaseMs = 300.0f;
    float env2Amount    = 0.3f; // 0..1

    // ENV3 -> loop cutoff: cutEff = fbCutoff * 2^(5 * amt * env3), amt bipolar.
    // The shift is strongest at the envelope PEAK: positive amount = note
    // starts bright and damps toward sustain (plucked-string decay — the
    // default), negative = starts choked and opens up.
    float env3AttackMs  = 5.0f;
    float env3DecayMs   = 600.0f;
    float env3Sustain   = 0.25f;
    float env3ReleaseMs = 300.0f;
    float env3Amount    = 0.35f; // -1..+1

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
        noise.seed (0x9E3779B9u * (juce::uint32) (voiceIndex + 1));
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

        auto& fx = loop.fx();
        juce::uint8 mask = 0;
        if (p.fxFilterOn) mask |= LoopFxChain::bit (LoopModuleID::filter);
        if (p.fxSatOn)    mask |= LoopFxChain::bit (LoopModuleID::saturator);
        if (p.fxEchoOn)   mask |= LoopFxChain::bit (LoopModuleID::echo);
        if (p.fxDiffOn)   mask |= LoopFxChain::bit (LoopModuleID::diffuser);
        if (p.fxRingOn)   mask |= LoopFxChain::bit (LoopModuleID::ringmod);
        fx.setEnabled (mask);
        fx.setOrder (p.loopOrder);
        fx.filter.setShape (p.fbBandpass, p.fbReso);
        fx.filter.setCutoff (p.fbCutoff);
        fx.saturator.set ((Saturator::Mode) p.fxSatMode, p.fbDriveDb);
        fx.diffuser.set (p.fxDiffSize, p.fxDiffAmt);
        // Echo and ring mod are frequency-dependent (ratio/track modes) and
        // update at chunk rate in renderNextBlock.

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
        samplePos = 0.0;
        env1.noteOn();
        env2.noteOn();
        env3.noteOn();
    }

    // Legato pitch change: retune (and glide) without retriggering envelopes
    // or resetting phases/loop state. The tuned delay follows the glided
    // frequency automatically — this is the mono-synth scream-bend.
    void changeNote (int midiNote, float glideSeconds)
    {
        note = midiNote;
        const float current = smoothedFreq.getCurrentValue();
        smoothedFreq.reset (sr, glideSeconds);
        smoothedFreq.setCurrentAndTargetValue (current);
        smoothedFreq.setTargetValue (noteHz (midiNote));
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

            // Sample playback rate: repitched by the sounding frequency (so
            // it glides/bends with the voice) relative to the root note, and
            // corrected for the clip's own rate.
            const bool sampleActive = params.sample != nullptr && params.srcSampleLevel > 0.0f;
            double sampleRatio = 0.0;
            if (sampleActive)
                sampleRatio = (params.sample->sourceRate / sr)
                              * (double) (freq / noteHz (params.srcSampleRoot));

            // Loop tuning tracks the sounding pitch (post-glide/octave/bend),
            // interpolated in the log domain toward the fixed anchor at 0%
            // keytrack, plus the tune offset.
            const float loopHz = std::exp2 (params.fbKeytrack * std::log2 (freq)
                                            + (1.0f - params.fbKeytrack)
                                                  * std::log2 (VoiceParams::fbBaseHz)
                                            + params.fbTuneSemis / 12.0f);
            loop.setLoopFrequency (loopHz);

            if (params.fxEchoOn)
            {
                static constexpr float ratios[] = { 0.0f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f };
                const int sync = juce::jlimit (0, 5, params.fxEchoSync);
                const float echoSamples = sync == 0
                    ? params.fxEchoTimeMs * 0.001f * (float) sr
                    : ratios[sync] * loop.getDelaySamples();
                loop.fx().echo.set (echoSamples, params.fxEchoAmt);
            }

            if (params.fxRingOn)
            {
                const float shiftHz = params.fxRingMode == 0
                    ? params.fxRingHz
                    : juce::jlimit (0.05f, 4000.0f, params.fxRingRatio * freq);
                loop.fx().ringmod.set (shiftHz, params.fxRingMix, sr);
            }

            // ENV3 sweeps the damping cutoff. The envelope advances per
            // sample (below); the filter coefficient updates at chunk rate,
            // which the TPT structure tolerates and keeps tan() off the
            // per-sample path.
            if (std::abs (params.env3Amount) > 1.0e-4f)
                loop.fx().filter.setCutoff (params.fbCutoff
                                            * std::exp2 (5.0f * params.env3Amount * env3Level));

            for (int i = 0; i < n; ++i)
            {
                float l, r;
                osc.processSample (l, r);
                l *= params.srcSawLevel;
                r *= params.srcSawLevel;

                // Mono noise into both channels BEFORE the loop's mono sum,
                // so it excites the loop like the saws do (noise + high
                // feedback + damping = the classic Karplus-Strong pluck).
                // 0.4: rough loudness match against the normalized saw stack.
                if (params.srcNoiseLevel > 0.0f)
                {
                    const float nz = noise.process (params.srcNoisePink)
                                     * params.srcNoiseLevel * 0.4f;
                    l += nz;
                    r += nz;
                }

                // Sample exciter: mono into both channels pre-loop, like noise.
                if (sampleActive)
                {
                    const int total = params.sample->mono.getNumSamples();
                    const bool ended = ! params.srcSampleLoop && samplePos >= (double) total;
                    if (! ended)
                    {
                        const double pos = params.srcSampleLoop
                                               ? std::fmod (samplePos, (double) total)
                                               : samplePos;
                        const float sv = params.sample->read (pos, params.srcSampleLoop)
                                         * params.srcSampleLevel * 0.8f;
                        l += sv;
                        r += sv;
                        samplePos += sampleRatio;
                    }
                }

                // ENV2 adds on top of the knob (spec: base + amount * ADSR),
                // clamped to the knob's own 110% ceiling.
                const float fbBase = fbBaseSmoothed.getNextValue();
                const float env2Sample = env2.getNextSample();
                const float fbEff = juce::jlimit (
                    0.0f, 1.1f, fbBase + params.env2Amount * env2Sample);
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
    NoiseSource noise;
    TunedFeedbackLoop loop;
    juce::ADSR env1, env2, env3;
    juce::SmoothedValue<float> fbBaseSmoothed { 0.0f };
    float env3Level = 0.0f;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedFreq { 440.0f };

    VoiceParams params;
    double samplePos = 0.0;
    int note = -1;
    bool heldDown = false;
    float level = 0.0f;
    float bendSemis = 0.0f;
    float lastEnvLevel = 0.0f;
};
