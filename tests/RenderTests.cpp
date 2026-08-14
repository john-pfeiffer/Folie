// Offline DSP render tests. Plain assert-style checks, no framework.
// Each test renders audio headlessly through the real SynthEngine and exits
// non-zero on failure.

#include <juce_dsp/juce_dsp.h>

#include "dsp/SoftClip.h"
#include "dsp/SynthEngine.h"
#include "dsp/TunedFeedbackLoop.h"

#include <chrono>
#include <cstdio>

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;

int failures = 0;

void check (bool condition, const char* what)
{
    if (condition)
        std::printf ("PASS  %s\n", what);
    else
    {
        std::printf ("FAIL  %s\n", what);
        ++failures;
    }
}

// Renders `seconds` of audio, feeding the given MIDI events (sample-stamped
// relative to the start of the render) into the engine block by block.
juce::AudioBuffer<float> render (SynthEngine& engine,
                                 const std::vector<std::pair<int, juce::MidiMessage>>& events,
                                 double seconds)
{
    const int totalSamples = (int) (seconds * kSampleRate);
    juce::AudioBuffer<float> out (2, totalSamples);
    out.clear();

    juce::AudioBuffer<float> block (2, kBlockSize);

    for (int start = 0; start < totalSamples; start += kBlockSize)
    {
        const int n = juce::jmin (kBlockSize, totalSamples - start);
        juce::MidiBuffer midi;

        for (const auto& [time, msg] : events)
            if (time >= start && time < start + n)
                midi.addEvent (msg, time - start);

        block.setSize (2, n, false, false, true);
        block.clear();
        engine.renderBlock (block, midi);

        for (int ch = 0; ch < 2; ++ch)
            out.copyFrom (ch, start, block, ch, 0, n);
    }

    return out;
}

bool allFinite (const juce::AudioBuffer<float>& b, float maxAbs)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
    {
        const float* data = b.getReadPointer (ch);
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (! std::isfinite (data[i]) || std::abs (data[i]) > maxAbs)
                return false;
    }
    return true;
}

float rmsOfTail (const juce::AudioBuffer<float>& b, double tailSeconds)
{
    const int tailSamples = juce::jmin (b.getNumSamples(), (int) (tailSeconds * kSampleRate));
    const int start = b.getNumSamples() - tailSamples;
    float sum = 0.0f;
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        sum += b.getRMSLevel (ch, start, tailSamples);
    return sum / (float) b.getNumChannels();
}

// Frequency of the strongest FFT bin over the last `windowSeconds` of channel 0.
float dominantFrequency (const juce::AudioBuffer<float>& b, double windowSeconds)
{
    constexpr int order = 15; // 32768 points
    constexpr int fftSize = 1 << order;

    juce::dsp::FFT fft (order);
    std::vector<float> data ((size_t) fftSize * 2, 0.0f);

    const int available = juce::jmin (b.getNumSamples(), (int) (windowSeconds * kSampleRate));
    const int start = b.getNumSamples() - available;
    const int n = juce::jmin (available, fftSize);
    const float* src = b.getReadPointer (0);

    for (int i = 0; i < n; ++i)
    {
        // Hann window
        const float w = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) n);
        data[(size_t) i] = src[start + i] * w;
    }

    fft.performFrequencyOnlyForwardTransform (data.data());

    int bestBin = 1;
    float bestMag = 0.0f;
    for (int bin = 1; bin < fftSize / 2; ++bin)
    {
        if (data[(size_t) bin] > bestMag)
        {
            bestMag = data[(size_t) bin];
            bestBin = bin;
        }
    }

    return (float) bestBin * (float) kSampleRate / (float) fftSize;
}

float centsBetween (float f1, float f2)
{
    return 1200.0f * std::log2 (f1 / f2);
}

EngineParams defaultParams()
{
    EngineParams p;
    return p;
}

void testPolyphonicSanity()
{
    SynthEngine engine;
    engine.prepare (kSampleRate);
    engine.setParams (defaultParams());

    std::vector<std::pair<int, juce::MidiMessage>> events;
    const int chord[] = { 48, 52, 55, 59 };
    for (int i = 0; i < 4; ++i)
    {
        events.emplace_back (i * 2000, juce::MidiMessage::noteOn (1, chord[i], 0.8f));
        events.emplace_back (80000 + i * 2000, juce::MidiMessage::noteOff (1, chord[i]));
    }

    auto out = render (engine, events, 3.0);
    check (allFinite (out, 4.0f), "poly render: all samples finite, |x| <= 4");
    check (out.getMagnitude (0, out.getNumSamples()) > 0.01f, "poly render: produces signal");
}

void testTailDecay()
{
    SynthEngine engine;
    engine.prepare (kSampleRate);
    engine.setParams (defaultParams());

    std::vector<std::pair<int, juce::MidiMessage>> events {
        { 0, juce::MidiMessage::noteOn (1, 60, 0.9f) },
        { (int) kSampleRate, juce::MidiMessage::noteOff (1, 60) },
    };

    // 1 s note + 6 s after release (default release is 300 ms).
    auto out = render (engine, events, 7.0);
    const float tailRms = rmsOfTail (out, 1.0);
    check (tailRms < juce::Decibels::decibelsToGain (-60.0f),
           "tail decay: silent (< -60 dBFS) 6 s after release");
    check (engine.countActiveVoices() == 0, "tail decay: voice freed after release");
}

void testOscillatorPitch()
{
    SynthEngine engine;
    engine.prepare (kSampleRate);

    auto p = defaultParams();
    p.voice.sawCount = 1;   // single saw, no detune: fundamental should dominate
    p.voice.detune = 0.0f;
    p.voice.fbGain = 0.0f;  // loop off for a clean oscillator measurement
    p.voice.env1AttackMs = 1.0f;
    p.voice.env1Sustain = 1.0f;
    engine.setParams (p);

    std::vector<std::pair<int, juce::MidiMessage>> events {
        { 0, juce::MidiMessage::noteOn (1, 45, 0.9f) }, // A2 = 110 Hz
    };

    auto out = render (engine, events, 2.0);
    const float peak = dominantFrequency (out, 1.0);
    const float cents = centsBetween (peak, 110.0f);
    std::printf ("      osc pitch: dominant %.2f Hz (%.1f cents from A2)\n", peak, cents);
    check (std::abs (cents) < 20.0f, "osc pitch: single saw fundamental within 20 cents of A2");
}

// The core promise of the instrument: excite the loop with a burst, let it
// ring at high feedback, and the ringing pitch must be the note you asked for
// (Karplus-Strong behavior). Tests the tuning math + fractional delay directly.
void testLoopPitch()
{
    for (const float target : { 110.0f, 880.0f }) // A2 and A5 (A5 nears min-delay clamp territory at high keytrack offsets)
    {
        TunedFeedbackLoop feedbackLoop;
        feedbackLoop.prepare (kSampleRate);
        feedbackLoop.setFilter (false, 8000.0f, 0.71f);
        feedbackLoop.setDrive (0.0f);
        feedbackLoop.setLoopFrequency (target);

        const int totalSamples = (int) kSampleRate * 2;
        juce::AudioBuffer<float> out (1, totalSamples);

        // 50 ms sine burst at the target pitch (like the saw fundamental
        // feeding the loop in real use), then free ringing. A noise burst
        // would excite every comb mode equally and the FFT could legitimately
        // pick a high harmonic as "dominant".
        const int burst = (int) (0.05 * kSampleRate);
        for (int i = 0; i < totalSamples; ++i)
        {
            const float dry = i < burst
                                  ? 0.5f * std::sin (juce::MathConstants<float>::twoPi
                                                     * target * (float) i / (float) kSampleRate)
                                  : 0.0f;
            out.setSample (0, i, feedbackLoop.processSample (dry, 0.98f));
        }

        const float peak = dominantFrequency (out, 1.0);
        const float cents = centsBetween (peak, target);
        std::printf ("      loop pitch: target %.0f Hz -> rings at %.2f Hz (%.1f cents)\n",
                     target, peak, cents);
        check (std::abs (cents) < 50.0f, "loop pitch: rings within 50 cents of target");
    }
}

// Worst case everything: 110% feedback, +24 dB drive, wide-open filter, high
// resonance, 16 voices. Must stay finite and bounded (tanh + DC blocker are
// the guarantees under test). Doubles as the spec's CPU sanity check: the
// render must run comfortably faster than realtime.
void testLoopStabilityAndPerfAtExtremes()
{
    SynthEngine engine;
    engine.prepare (kSampleRate);

    auto p = defaultParams();
    p.voice.fbGain = 1.1f;
    p.voice.fbDriveDb = 24.0f;
    p.voice.fbCutoff = 20000.0f;
    p.voice.fbReso = 8.0f;
    p.voice.fbTuneSemis = 12.0f;
    p.voice.sawCount = 16;
    p.voice.env1Sustain = 1.0f;
    p.polyphony = 16;
    engine.setParams (p);

    std::vector<std::pair<int, juce::MidiMessage>> events;
    for (int i = 0; i < 16; ++i)
        events.emplace_back (i * 400, juce::MidiMessage::noteOn (1, 36 + i * 3, 1.0f));

    const auto t0 = std::chrono::steady_clock::now();
    auto out = render (engine, events, 10.0);
    const auto t1 = std::chrono::steady_clock::now();
    const double renderSeconds = std::chrono::duration<double> (t1 - t0).count();
    const double realtimeFactor = renderSeconds / 10.0;

    check (allFinite (out, 20.0f), "loop stability: 16 voices at 110% fb / +24 dB drive stay finite & bounded");
    check (out.getMagnitude (0, out.getNumSamples()) > 0.01f, "loop stability: still producing signal");

    std::printf ("      perf: 10 s @ 16 voices x 16 saws rendered in %.2f s (%.2fx realtime budget)\n",
                 renderSeconds, realtimeFactor);
    check (realtimeFactor < 1.0, "perf: max-polyphony extreme patch renders faster than realtime");
}

// After note-off + release the voice must go silent even with the loop pushed
// past unity — ENV1 gates the loop, and the voice must actually free itself.
void testFeedbackTailGated()
{
    SynthEngine engine;
    engine.prepare (kSampleRate);

    auto p = defaultParams();
    p.voice.fbGain = 1.1f;
    p.voice.fbDriveDb = 12.0f;
    engine.setParams (p);

    std::vector<std::pair<int, juce::MidiMessage>> events {
        { 0, juce::MidiMessage::noteOn (1, 48, 0.9f) },
        { (int) kSampleRate, juce::MidiMessage::noteOff (1, 48) },
    };

    auto out = render (engine, events, 7.0);
    check (rmsOfTail (out, 1.0) < juce::Decibels::decibelsToGain (-60.0f),
           "feedback tail: gated silent by amp env after release");
    check (engine.countActiveVoices() == 0, "feedback tail: voice freed");
}

// ENV2 is additive per spec (fbEff = base + amount * env2). With base 0 and
// amount 100%, the envelope alone must drive the loop (impossible under the
// old multiplicative blend); with sustain 0 the scream must die back to plain
// saws late in a held note.
void testFeedbackEnvelopeAdditive()
{
    auto renderCase = [] (float base, float amount)
    {
        SynthEngine engine;
        engine.prepare (kSampleRate);

        auto p = defaultParams();
        p.voice.sawCount = 1;
        p.voice.fbGain = base;
        p.voice.fbDriveDb = 6.0f;
        p.voice.env1Sustain = 1.0f;
        p.voice.env2Amount = amount;
        p.voice.env2DecayMs = 200.0f;
        p.voice.env2Sustain = 0.0f;
        engine.setParams (p);

        std::vector<std::pair<int, juce::MidiMessage>> events {
            { 0, juce::MidiMessage::noteOn (1, 48, 0.9f) },
        };
        return render (engine, events, 4.0);
    };

    const auto held = renderCase (1.0f, 0.0f);      // static screaming feedback
    const auto enveloped = renderCase (0.0f, 1.0f); // env-only feedback, decays to 0

    const float heldLate = rmsOfTail (held, 1.0);
    const float envelopedLate = rmsOfTail (enveloped, 1.0);
    std::printf ("      env2: late RMS static fb %.4f vs env-only fb %.4f\n",
                 heldLate, envelopedLate);
    check (envelopedLate < heldLate * 0.7f,
           "env2 additive: env-only feedback decays back to saws while static fb screams on");

    const float envelopedEarly = enveloped.getRMSLevel (0, 0, (int) (0.2 * kSampleRate));
    check (envelopedEarly > envelopedLate * 1.3f,
           "env2 additive: base-0 patch is driven by the envelope early in the note");
    check (allFinite (enveloped, 20.0f), "env2 additive: enveloped render finite");
}

// ENV3 sweep at extreme settings must stay stable (per-sample env, chunk-rate
// cutoff updates into the TPT filter).
void testCutoffEnvelopeStability()
{
    SynthEngine engine;
    engine.prepare (kSampleRate);

    auto p = defaultParams();
    p.voice.fbGain = 1.1f;
    p.voice.fbDriveDb = 24.0f;
    p.voice.fbReso = 8.0f;
    p.voice.env3Amount = 1.0f;   // +5 octaves from 4 kHz, clamped internally
    p.voice.env3DecayMs = 100.0f;
    p.voice.env3Sustain = 0.0f;  // full sweep down every note
    p.voice.env2Amount = 1.0f;
    p.polyphony = 8;
    engine.setParams (p);

    std::vector<std::pair<int, juce::MidiMessage>> events;
    for (int i = 0; i < 8; ++i)
        events.emplace_back (i * 6000, juce::MidiMessage::noteOn (1, 40 + i * 4, 1.0f));

    auto out = render (engine, events, 6.0);
    check (allFinite (out, 20.0f), "env3: extreme swept-cutoff render stays finite");
}

// Regression guard for the "no feedback loop at all" field report: the same
// held-note phrase with hot feedback must carry substantially more energy
// than with the loop off.
void testFeedbackIsAudible()
{
    auto renderWithFeedback = [] (float fbGain, float driveDb)
    {
        SynthEngine engine;
        engine.prepare (kSampleRate);

        auto p = defaultParams();
        p.voice.fbGain = fbGain;
        p.voice.fbDriveDb = driveDb;
        p.voice.env1Sustain = 1.0f;
        engine.setParams (p);

        std::vector<std::pair<int, juce::MidiMessage>> events {
            { 0, juce::MidiMessage::noteOn (1, 45, 0.9f) },
        };
        return render (engine, events, 3.0);
    };

    const auto dry = renderWithFeedback (0.0f, 0.0f);
    const auto hot = renderWithFeedback (0.9f, 6.0f);

    const float dryRms = dry.getRMSLevel (0, (int) kSampleRate, (int) kSampleRate);
    const float hotRms = hot.getRMSLevel (0, (int) kSampleRate, (int) kSampleRate);
    std::printf ("      fb audibility: RMS fb-off %.4f vs fb-hot %.4f (%.2fx)\n",
                 dryRms, hotRms, hotRms / juce::jmax (1.0e-9f, dryRms));
    check (hotRms > dryRms * 1.25f, "fb audibility: hot loop adds >1.25x RMS over dry saws");
}

// Voice modes: Mono holds one voice with last-note priority; Legato changes
// pitch without retriggering and the whole voice (including the tuned loop)
// glides to the new note.
void testVoiceModes()
{
    // Mono: a held chord collapses to one voice; releasing the last note
    // falls back to the previous held note.
    {
        SynthEngine engine;
        engine.prepare (kSampleRate);
        auto p = defaultParams();
        p.mode = VoiceMode::mono;
        p.voice.sawCount = 1;
        p.voice.detune = 0.0f;
        p.voice.fbGain = 0.0f;
        p.voice.env1Sustain = 1.0f;
        engine.setParams (p);

        std::vector<std::pair<int, juce::MidiMessage>> events {
            { 0, juce::MidiMessage::noteOn (1, 45, 0.9f) },     // A2
            { 4000, juce::MidiMessage::noteOn (1, 52, 0.9f) },  // E3 (takes over)
            { (int) kSampleRate, juce::MidiMessage::noteOff (1, 52) }, // back to A2
        };

        auto out = render (engine, events, 3.0);
        check (engine.countActiveVoices() == 1, "mono: overlapping notes use a single voice");

        const float late = dominantFrequency (out, 1.0);
        std::printf ("      mono fallback: late dominant %.2f Hz (expect A2 110)\n", late);
        check (std::abs (centsBetween (late, 110.0f)) < 30.0f,
               "mono: releasing top note falls back to the held note");
        check (allFinite (out, 4.0f), "mono: finite");
    }

    // Legato: second overlapping note takes the pitch without retrigger;
    // the sounding pitch ends on the new note.
    {
        SynthEngine engine;
        engine.prepare (kSampleRate);
        auto p = defaultParams();
        p.mode = VoiceMode::legato;
        p.voice.sawCount = 1;
        p.voice.detune = 0.0f;
        p.voice.fbGain = 0.0f;
        p.voice.env1AttackMs = 1.0f;
        p.voice.env1Sustain = 1.0f;
        p.glideSeconds = 0.1f;
        engine.setParams (p);

        std::vector<std::pair<int, juce::MidiMessage>> events {
            { 0, juce::MidiMessage::noteOn (1, 45, 0.9f) },    // A2
            { 24000, juce::MidiMessage::noteOn (1, 57, 0.9f) } // A3, legato glide up
        };

        auto out = render (engine, events, 3.0);
        check (engine.countActiveVoices() == 1, "legato: overlapping notes use a single voice");

        const float late = dominantFrequency (out, 1.0);
        std::printf ("      legato glide: late dominant %.2f Hz (expect A3 220)\n", late);
        check (std::abs (centsBetween (late, 220.0f)) < 30.0f,
               "legato: pitch glides to the new note without retrigger");
        check (allFinite (out, 4.0f), "legato: finite");
    }
}

// The fixed output soft-clip: bit-exact below the -1 dBFS knee, bounded below
// 0 dBFS for any input.
void testSafetyClip()
{
    bool transparent = true;
    for (float x = -0.85f; x <= 0.85f; x += 0.05f)
        transparent = transparent && juce::exactlyEqual (SafetyClip::process (x), x);
    check (transparent, "safety clip: bit-exact below the knee");

    bool bounded = true;
    for (float x : { 1.5f, 4.0f, 100.0f, -100.0f, 1.0e9f })
        bounded = bounded && std::abs (SafetyClip::process (x)) <= 1.0f;
    check (bounded, "safety clip: ceiling at 0 dBFS for any input");
}
} // namespace

int main()
{
    testPolyphonicSanity();
    testTailDecay();
    testOscillatorPitch();
    testLoopPitch();
    testLoopStabilityAndPerfAtExtremes();
    testFeedbackTailGated();
    testFeedbackEnvelopeAdditive();
    testCutoffEnvelopeStability();
    testFeedbackIsAudible();
    testVoiceModes();
    testSafetyClip();

    std::printf (failures == 0 ? "All tests passed.\n" : "%d test(s) FAILED.\n", failures);
    return failures == 0 ? 0 : 1;
}
