// Offline DSP render tests. Plain assert-style checks, no framework.
// Each test renders audio headlessly through the real SynthEngine and exits
// non-zero on failure.

#include <juce_dsp/juce_dsp.h>

#include "dsp/SynthEngine.h"

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
} // namespace

int main()
{
    testPolyphonicSanity();
    testTailDecay();
    testOscillatorPitch();

    std::printf (failures == 0 ? "All tests passed.\n" : "%d test(s) FAILED.\n", failures);
    return failures == 0 ? 0 : 1;
}
