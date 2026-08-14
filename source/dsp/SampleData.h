#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

// The user's exciter clip: mono audio + its source rate, plus the FLAC/Base64
// encoding cached at load time so getStateInformation stays a plain tree copy.
// Immutable after construction; ownership lives on the message thread with a
// publish/retire scheme (see PluginProcessor) — the audio thread only ever
// reads a raw pointer snapshot taken once per block.
struct SampleData
{
    juce::AudioBuffer<float> mono; // 1 channel
    double sourceRate = 48000.0;
    juce::String name;
    juce::String base64; // 24-bit FLAC, Base64-encoded

    static constexpr double maxSeconds = 10.0;

    // Message thread. Returns null on failure.
    static std::unique_ptr<SampleData> fromBuffer (juce::AudioBuffer<float>&& monoBuffer,
                                                   double rate, const juce::String& name)
    {
        if (monoBuffer.getNumSamples() == 0 || rate <= 0.0)
            return nullptr;

        auto data = std::make_unique<SampleData>();
        data->mono = std::move (monoBuffer);
        data->sourceRate = rate;
        data->name = name;

        juce::MemoryBlock flacData;
        {
            juce::FlacAudioFormat flac;
            // Writer takes ownership of the stream; the stream writes into
            // flacData, which outlives both.
            std::unique_ptr<juce::AudioFormatWriter> writer (flac.createWriterFor (
                new juce::MemoryOutputStream (flacData, false), rate, 1, 24, {}, 0));
            if (writer == nullptr)
                return nullptr;
            if (! writer->writeFromAudioSampleBuffer (data->mono, 0, data->mono.getNumSamples()))
                return nullptr;
        } // writer destroyed -> stream flushed and closed

        data->base64 = juce::Base64::toBase64 (flacData.getData(), flacData.getSize());
        return data;
    }

    // Message thread (or host state-restore thread). Returns null on failure.
    static std::unique_ptr<SampleData> fromBase64 (const juce::String& base64,
                                                   const juce::String& name)
    {
        juce::MemoryOutputStream decoded;
        if (! juce::Base64::convertFromBase64 (decoded, base64))
            return nullptr;

        juce::FlacAudioFormat flac;
        std::unique_ptr<juce::AudioFormatReader> reader (flac.createReaderFor (
            new juce::MemoryInputStream (decoded.getData(), decoded.getDataSize(), false), true));
        if (reader == nullptr)
            return nullptr;

        auto data = std::make_unique<SampleData>();
        data->sourceRate = reader->sampleRate;
        data->name = name;
        data->base64 = base64;
        data->mono.setSize (1, (int) reader->lengthInSamples);
        reader->read (&data->mono, 0, (int) reader->lengthInSamples, 0, true, false);
        return data;
    }

    // 4-point Catmull-Rom read; loop mode wraps, one-shot clamps edges.
    forcedinline float read (double pos, bool loopMode) const noexcept
    {
        const int n = mono.getNumSamples();
        const float* d = mono.getReadPointer (0);

        const int i = (int) pos;
        const float frac = (float) (pos - (double) i);

        auto at = [d, n, loopMode] (int idx) -> float
        {
            if (loopMode)
            {
                idx %= n;
                return d[idx < 0 ? idx + n : idx];
            }
            return idx < 0 || idx >= n ? 0.0f : d[idx];
        };

        const float x0 = at (i - 1), x1 = at (i), x2 = at (i + 1), x3 = at (i + 2);

        const float a = 0.5f * (3.0f * (x1 - x2) - x0 + x3);
        const float b = x2 + x2 + x0 - (5.0f * x1 + x3) * 0.5f;
        const float c = 0.5f * (x2 - x0);
        return ((a * frac + b) * frac + c) * frac + x1;
    }
};
