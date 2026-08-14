// Offline DSP render tests. Plain assert-style checks, no framework.
// Each test renders audio headlessly through the real SynthEngine and exits
// non-zero on failure.

#include <juce_dsp/juce_dsp.h>

#include "dsp/FxChain.h"
#include "dsp/SynthEngine.h"
#include "dsp/TunedFeedbackLoop.h"

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
// the guarantees under test).
void testLoopStabilityAtExtremes()
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

    auto out = render (engine, events, 10.0);
    check (allFinite (out, 20.0f), "loop stability: 16 voices at 110% fb / +24 dB drive stay finite & bounded");
    check (out.getMagnitude (0, out.getNumSamples()) > 0.01f, "loop stability: still producing signal");
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
// ENV2 must actually shape the feedback: with amount=100% and sustain=0, the
// loop's contribution dies after the decay even though the note is held —
// versus amount=0 where fb>1 keeps the loop screaming. Compare late-window
// energy of the two renders.
void testFeedbackEnvelopeModulates()
{
    auto renderWithAmount = [] (float amount)
    {
        SynthEngine engine;
        engine.prepare (kSampleRate);

        auto p = defaultParams();
        p.voice.sawCount = 1;
        p.voice.fbGain = 1.05f;
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

    const auto held = renderWithAmount (0.0f);
    const auto enveloped = renderWithAmount (1.0f);

    const float heldLate = rmsOfTail (held, 1.0);
    const float envelopedLate = rmsOfTail (enveloped, 1.0);
    std::printf ("      env2: late RMS static fb %.4f vs enveloped fb %.4f\n",
                 heldLate, envelopedLate);
    check (envelopedLate < heldLate * 0.7f,
           "env2: enveloped feedback is quieter late in the note than static feedback");

    // And the enveloped render must stay finite while modulating per-sample.
    check (allFinite (enveloped, 20.0f), "env2: enveloped render finite");
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
// Renders a short poly phrase and returns it (shared input for FX tests).
juce::AudioBuffer<float> renderDryPhrase (double seconds)
{
    SynthEngine engine;
    engine.prepare (kSampleRate);
    engine.setParams (defaultParams());

    std::vector<std::pair<int, juce::MidiMessage>> events {
        { 0, juce::MidiMessage::noteOn (1, 48, 0.8f) },
        { 2000, juce::MidiMessage::noteOn (1, 55, 0.8f) },
        { 40000, juce::MidiMessage::noteOff (1, 48) },
        { 40000, juce::MidiMessage::noteOff (1, 55) },
    };
    return render (engine, events, seconds);
}

void processThroughFx (juce::AudioBuffer<float>& buffer, const FxParams& p)
{
    FxChain fxChain;
    fxChain.prepare ({ kSampleRate, (juce::uint32) kBlockSize, 2 });
    fxChain.setParams (p);

    juce::AudioBuffer<float> block (2, kBlockSize);
    for (int start = 0; start < buffer.getNumSamples(); start += kBlockSize)
    {
        const int n = juce::jmin (kBlockSize, buffer.getNumSamples() - start);
        block.setSize (2, n, false, false, true);
        for (int ch = 0; ch < 2; ++ch)
            block.copyFrom (ch, 0, buffer, ch, start, n);
        fxChain.setParams (p);
        fxChain.process (block);
        for (int ch = 0; ch < 2; ++ch)
            buffer.copyFrom (ch, start, block, ch, 0, n);
    }
}

// All FX off must be a clean passthrough (bit-for-bit is too strict across the
// chorus's internal mixer, but RMS must match within ~1 dB and stay finite).
void testFxBypassPassthrough()
{
    auto dry = renderDryPhrase (2.0);
    auto processed = renderDryPhrase (2.0);

    FxParams p; // all off by default
    processThroughFx (processed, p);

    check (allFinite (processed, 4.0f), "fx bypass: finite");
    const float dryRms = dry.getRMSLevel (0, 0, dry.getNumSamples());
    const float wetRms = processed.getRMSLevel (0, 0, processed.getNumSamples());
    const float ratioDb = std::abs (juce::Decibels::gainToDecibels (wetRms / juce::jmax (1.0e-9f, dryRms)));
    std::printf ("      fx bypass: RMS delta %.3f dB\n", ratioDb);
    check (ratioDb < 1.0f, "fx bypass: all-off chain passes signal within 1 dB");
}

// Everything on at extreme settings must stay finite and bounded.
void testFxExtremes()
{
    auto buffer = renderDryPhrase (6.0);

    FxParams p;
    p.chorusOn = true;  p.chorusRateHz = 8.0f; p.chorusDepth = 1.0f; p.chorusMix = 1.0f;
    p.delayOn = true;   p.delayTimeMs = 2000.0f; p.delayFeedback = 0.95f; p.delayMix = 1.0f;
    p.reverbOn = true;  p.reverbSize = 1.0f; p.reverbDamp = 0.0f; p.reverbMix = 1.0f;
    processThroughFx (buffer, p);

    check (allFinite (buffer, 20.0f), "fx extremes: all-on maxed chain stays finite & bounded");
}

// The delay must actually delay: with a 500 ms delay and the dry phrase ending
// before the render does, the late window must carry echo energy that the dry
// render doesn't have.
void testDelayProducesTail()
{
    auto dry = renderDryPhrase (4.0);
    auto wet = renderDryPhrase (4.0);

    FxParams p;
    p.delayOn = true;
    p.delayTimeMs = 500.0f;
    p.delayFeedback = 0.6f;
    p.delayMix = 1.0f;
    processThroughFx (wet, p);

    const float dryLate = rmsOfTail (dry, 1.0);
    const float wetLate = rmsOfTail (wet, 1.0);
    std::printf ("      delay tail: late RMS dry %.5f vs delayed %.5f\n", dryLate, wetLate);
    check (wetLate > dryLate * 2.0f + 1.0e-5f, "delay: echoes persist after the dry phrase ends");
}
} // namespace

int main()
{
    testPolyphonicSanity();
    testTailDecay();
    testOscillatorPitch();
    testLoopPitch();
    testLoopStabilityAtExtremes();
    testFeedbackTailGated();
    testFeedbackEnvelopeModulates();
    testCutoffEnvelopeStability();
    testFxBypassPassthrough();
    testFxExtremes();
    testDelayProducesTail();

    std::printf (failures == 0 ? "All tests passed.\n" : "%d test(s) FAILED.\n", failures);
    return failures == 0 ? 0 : 1;
}
