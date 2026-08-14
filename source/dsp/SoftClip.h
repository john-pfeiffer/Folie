#pragma once

#include <cmath>

// The only bus stage: a fixed, parameterless safety soft-clip. Pure speaker
// protection per the handoff — a feedback synth can spike +20 dB faster than
// a hand reaches a fader. Transparent (bit-exact) below the -1 dBFS knee,
// tanh knee above, hard ceiling at 0 dBFS. No character, no controls.
namespace SafetyClip
{
inline constexpr float knee = 0.891f; // -1 dBFS

inline float process (float x) noexcept
{
    const float ax = std::abs (x);
    if (ax <= knee)
        return x;

    const float sign = x > 0.0f ? 1.0f : -1.0f;
    return sign * (knee + (1.0f - knee) * std::tanh ((ax - knee) / (1.0f - knee)));
}
} // namespace SafetyClip
