#pragma once

#include "../dsp/LoopModules.h"

#include <juce_core/juce_core.h>

// The loop rack's module order. NOT a host parameter (see ParameterIDs.h):
// it lives in the state tree as the "loopOrder" property (message thread) and
// crosses to the audio thread as a packed atomic word (3 bits per slot).
// Everything here sanitizes: any input becomes a valid permutation of 0..4
// (first occurrence wins, missing modules appended in canonical order).
namespace LoopOrder
{
using Order = std::array<juce::uint8, numLoopModules>;

inline constexpr Order canonical { 0, 1, 2, 3, 4 };

inline Order sanitize (const Order& in)
{
    Order out {};
    bool seen[numLoopModules] {};
    int n = 0;

    for (auto v : in)
        if (v < numLoopModules && ! seen[v])
        {
            seen[v] = true;
            out[(size_t) n++] = v;
        }

    for (juce::uint8 m = 0; m < numLoopModules; ++m)
        if (! seen[m])
            out[(size_t) n++] = m;

    return out;
}

inline juce::uint32 pack (const Order& order)
{
    juce::uint32 word = 0;
    for (int i = 0; i < numLoopModules; ++i)
        word |= (juce::uint32) (order[(size_t) i] & 0x7) << (i * 3);
    return word;
}

inline Order unpack (juce::uint32 word)
{
    Order order {};
    for (int i = 0; i < numLoopModules; ++i)
        order[(size_t) i] = (juce::uint8) ((word >> (i * 3)) & 0x7);
    return sanitize (order);
}

inline juce::String toString (const Order& order)
{
    juce::String s;
    for (int i = 0; i < numLoopModules; ++i)
        s << (int) order[(size_t) i] << (i < numLoopModules - 1 ? "," : "");
    return s;
}

inline Order fromString (const juce::String& s)
{
    Order order = canonical;
    auto tokens = juce::StringArray::fromTokens (s, ",", "");
    for (int i = 0; i < juce::jmin (numLoopModules, tokens.size()); ++i)
        order[(size_t) i] = (juce::uint8) juce::jlimit (0, numLoopModules - 1,
                                                        tokens[i].getIntValue());
    return sanitize (order);
}
} // namespace LoopOrder
