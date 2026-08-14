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

// Feedback loop
inline constexpr auto fbGain       = "fbGain";
inline constexpr auto fbKeytrack   = "fbKeytrack";
inline constexpr auto fbTune       = "fbTune";
inline constexpr auto fbFilterMode = "fbFilterMode";
inline constexpr auto fbCutoff     = "fbCutoff";
inline constexpr auto fbReso       = "fbReso";
inline constexpr auto fbDrive      = "fbDrive";

// ENV1 — amp
inline constexpr auto env1Attack  = "env1Attack";
inline constexpr auto env1Decay   = "env1Decay";
inline constexpr auto env1Sustain = "env1Sustain";
inline constexpr auto env1Release = "env1Release";

// ENV2 — feedback gain
inline constexpr auto env2Attack  = "env2Attack";
inline constexpr auto env2Decay   = "env2Decay";
inline constexpr auto env2Sustain = "env2Sustain";
inline constexpr auto env2Release = "env2Release";
inline constexpr auto env2Amount  = "env2Amount";

// ENV3 — loop filter cutoff
inline constexpr auto env3Attack  = "env3Attack";
inline constexpr auto env3Decay   = "env3Decay";
inline constexpr auto env3Sustain = "env3Sustain";
inline constexpr auto env3Release = "env3Release";
inline constexpr auto env3Amount  = "env3Amount";

// Global
// (Zero post FX by design — the only bus stage is a fixed, parameterless
// safety soft-clip. See the post-initial-build handoff.)
inline constexpr auto voiceMode    = "voiceMode";
inline constexpr auto polyphony    = "polyphony";
inline constexpr auto glideTime    = "glideTime";
inline constexpr auto masterVolume = "masterVolume";
} // namespace ParamIDs
