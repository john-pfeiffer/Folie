#pragma once

// Single source of truth for parameter IDs. Every parameter is registered in
// createParameterLayout() and nowhere else; never inline these strings.
namespace ParamIDs
{
// Oscillator
inline constexpr auto oscSawCount = "oscSawCount";
inline constexpr auto oscDetune   = "oscDetune";
inline constexpr auto oscBlend    = "oscBlend";
inline constexpr auto oscWidth    = "oscWidth";
inline constexpr auto oscOctave   = "oscOctave";

// ENV1 — amp
inline constexpr auto env1Attack  = "env1Attack";
inline constexpr auto env1Decay   = "env1Decay";
inline constexpr auto env1Sustain = "env1Sustain";
inline constexpr auto env1Release = "env1Release";

// Global
inline constexpr auto polyphony    = "polyphony";
inline constexpr auto glideTime    = "glideTime";
inline constexpr auto masterVolume = "masterVolume";
} // namespace ParamIDs
