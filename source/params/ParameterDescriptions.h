#pragma once

#include "ParameterIDs.h"

#include <juce_core/juce_core.h>

// Single source of truth for control tooltips, keyed by parameter ID. The
// coverage test iterates the parameter layout and fails if any registered
// parameter lacks a description here — adding a knob without explaining it
// is a build-breaking omission.
inline juce::String describeParam (const juce::String& id)
{
    using namespace ParamIDs;

    // Oscillator
    if (id == oscSawCount) return "How many detuned saws stack up per note (1 = a single clean saw, 16 = the full wall).";
    if (id == oscDetune)   return "How far the stacked saws spread apart in pitch. Low = thick unison, high = huge shimmering cloud.";
    if (id == oscBlend)    return "Center saw vs the detuned copies. 0% = only the clean center saw; 100% = everything at full level.";
    if (id == oscWidth)    return "Stereo spread of the saw stack. 0% = mono, 100% = saws panned hard alternating left/right.";
    if (id == oscOctave)   return "Coarse tune in octaves.";

    // Source
    if (id == srcSawLevel)    return "Level of the saw stack feeding the voice and its feedback loop.";
    if (id == srcNoiseLevel)  return "Noise into the voice and loop. Noise + high Feedback + a dark Loop Cutoff = plucked-string (Karplus-Strong) tones.";
    if (id == srcNoiseType)   return "White = bright hiss. Pink = darker, more even across octaves.";
    if (id == srcSampleLevel) return "Level of the loaded clip feeding the voice and loop. The clip is repitched to the note you play.";
    if (id == srcSampleRoot)  return "The note at which the clip plays back unchanged. Play above/below and it repitches accordingly.";
    if (id == srcSampleLoop)  return "On: the clip loops while the note holds. Off: it plays once per note (an exciter burst).";

    // Loop core
    if (id == fbGain)     return "THE knob. How much of the loop feeds back into itself. Past ~90% it sings on its own; past 100% it's held on the edge by the in-loop saturation.";
    if (id == fbKeytrack) return "How much the loop tunes to the note you play. 100% = feedback screams in key; lower = detuned, metallic, dissonant.";
    if (id == fbTune)     return "Offsets the loop's resonance from the note, in semitones. +12 = octave-up screech, +7 = fifth drones.";

    // Rack: filter
    if (id == fxFilterOn)   return "Damping filter inside the loop. Off = the loop keeps all its brightness.";
    if (id == fbFilterMode) return "LP darkens the feedback each pass. BP hollows it into a metallic band.";
    if (id == fbCutoff)     return "Where the loop's filter sits. Lower = feedback dies darker and faster; higher = harmonics survive and scream.";
    if (id == fbReso)       return "Resonant emphasis at the loop filter's cutoff — adds a whistling formant to the feedback.";

    // Rack: saturator
    if (id == fxSatOn)   return "Distortion inside the loop. This is also what tames feedback past 100% — off, a hidden safety limiter takes over.";
    if (id == fxSatMode) return "Tanh = smooth compression. Fold = wavefolder, chaotic shimmer when pushed. Clip = hard edge, harshest.";
    if (id == fbDrive)   return "How hard the loop hits its distortion each pass. More drive = the loop tips into screaming earlier and harder.";

    // Rack: echo
    if (id == fxEchoOn)   return "A second delay tap inside the loop — feedback develops rhythmic, canon-like patterns.";
    if (id == fxEchoSync) return "Free = set the time in ms. x2..x8 = the tap tracks the note's own loop length (adds under-tones that follow the key).";
    if (id == fxEchoTime) return "Echo tap time in Free mode.";
    if (id == fxEchoAmt)  return "Echo tap level. Negative flips its polarity for a hollower comb.";

    // Rack: diffuser
    if (id == fxDiffOn)   return "A tiny reverb inside the loop — smears the scream into bowed, shimmering textures. Detunes the resonance a little (Loop Tune compensates).";
    if (id == fxDiffSize) return "Size of the in-loop diffusion. Bigger = washier, more detuned.";
    if (id == fxDiffAmt)  return "How much diffusion each pass picks up.";

    // Rack: ring mod
    if (id == fxRingOn)    return "Multiplies the loop by a sine each pass — energy spirals through sidebands: barberpole and bell-metal tones.";
    if (id == fxRingMode)  return "Hz = fixed shift rate (slow = spirals). Track = the shift follows the note as a ratio (constant metallic interval).";
    if (id == fxRingHz)    return "Shift rate in Hz mode. Very low = slow evolving spirals; higher = clangorous.";
    if (id == fxRingRatio) return "Shift as a ratio of the note's frequency in Track mode. 0.5 = down an octave-ish color, 1.0 = octave metallic.";
    if (id == fxRingMix)   return "Dry/wet inside the loop. Small amounts spiral gently; 100% is full ring modulation every pass.";

    // Envelopes
    if (id == env1Attack)  return "Amp fade-in time per note.";
    if (id == env1Decay)   return "Time from the attack peak down to the sustain level.";
    if (id == env1Sustain) return "Held level while the key is down.";
    if (id == env1Release) return "Fade-out after release. Feedback tails live inside this — long screams need long release.";
    if (id == env2Attack)  return "How fast the feedback boost arrives each note.";
    if (id == env2Decay)   return "How fast the feedback boost falls to its sustain.";
    if (id == env2Sustain) return "Feedback boost level while the key is held.";
    if (id == env2Release) return "How the feedback boost lets go after release.";
    if (id == env2Amount)  return "ADDS to the Feedback knob per note: feedback = knob + this envelope. Knob at 0 + amount up = feedback that only exists where the envelope puts it.";
    if (id == env3Attack)  return "How fast the loop-brightness sweep arrives each note.";
    if (id == env3Decay)   return "How fast the sweep falls to its sustain.";
    if (id == env3Sustain) return "Sweep level while the key is held.";
    if (id == env3Release) return "How the sweep lets go after release.";
    if (id == env3Amount)  return "Sweeps the Loop Cutoff per note, up to +/-5 octaves (strongest at the envelope peak). Positive = starts bright, damps down (plucked-string decay). Negative = starts choked, opens up.";

    // Global
    if (id == voiceMode)  return "Poly = every note its own voice+loop. Mono = one voice, retriggers. Legato = one voice, overlapping notes glide WITHOUT retriggering - the scream bends with you.";
    if (id == polyphony)  return "Maximum simultaneous voices. Also the CPU throttle - each voice runs its own full loop rack.";
    if (id == glideTime)  return "Pitch slide time between notes. In Legato mode the tuned loop glides too (the mono-synth scream-bend).";
    if (id == masterVolume) return "Output level before the always-on safety clip.";

    return {};
}
