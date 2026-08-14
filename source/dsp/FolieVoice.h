#pragma once

#include "SupersawOscillator.h"

// Parameters fanned out from the APVTS once per block (plain values — the
// audio thread never touches parameter objects directly).
struct VoiceParams
{
    int   sawCount   = 7;
    float detune     = 0.25f;   // 0..1
    float blend      = 0.6f;    // 0..1
    float width      = 0.8f;    // 0..1
    int   octave     = 0;       // -2..+2
    float env1AttackMs  = 5.0f;
    float env1DecayMs   = 200.0f;
    float env1Sustain   = 0.8f; // 0..1
    float env1ReleaseMs = 300.0f;
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
        env1.setSampleRate (sampleRate);
        smoothedFreq.reset (sampleRate, 0.0);
        reset();
    }

    void reset()
    {
        env1.reset();
        note = -1;
        lastEnvLevel = 0.0f;
    }

    void setParams (const VoiceParams& p)
    {
        params = p;
        osc.setParams (p.sawCount, p.detune, p.blend, p.width);
        env1.setParameters ({ p.env1AttackMs * 0.001f,
                              p.env1DecayMs * 0.001f,
                              p.env1Sustain,
                              p.env1ReleaseMs * 0.001f });
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
    }

    void stopNote (bool allowTailOff)
    {
        if (allowTailOff)
            env1.noteOff();
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

            for (int i = 0; i < n; ++i)
            {
                float l, r;
                osc.processSample (l, r);

                const float amp = level * env1.getNextSample();
                lastEnvLevel = amp;

                left[pos + i] += l * amp;
                right[pos + i] += r * amp;
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
    juce::ADSR env1;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedFreq { 440.0f };

    VoiceParams params;
    int note = -1;
    bool heldDown = false;
    float level = 0.0f;
    float bendSemis = 0.0f;
    float lastEnvLevel = 0.0f;
};
