#pragma once

#include <juce_core/juce_core.h>

// Per-voice noise source: branchless xorshift32 white, Paul Kellet economy
// 3-pole pink. Mono — it feeds the voice's mono loop sum at full strength
// (stereo-decorrelated noise is a tracked later idea).
struct NoiseSource
{
    void seed (juce::uint32 s) { state = s | 1u; }

    void reset()
    {
        b0 = b1 = b2 = 0.0f;
    }

    forcedinline float white() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        // uint32 -> [-1, 1)
        return (float) (juce::int32) state * (1.0f / 2147483648.0f);
    }

    forcedinline float process (bool pink) noexcept
    {
        const float w = white();
        if (! pink)
            return w;

        b0 = 0.99765f * b0 + w * 0.0990460f;
        b1 = 0.96300f * b1 + w * 0.2965164f;
        b2 = 0.57000f * b2 + w * 1.0526913f;
        return (b0 + b1 + b2 + w * 0.1848f) * 0.25f;
    }

    juce::uint32 state = 0x466f6c69;
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
};
