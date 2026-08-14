#include "ParameterLayout.h"
#include "ParameterIDs.h"

namespace
{
using APF = juce::AudioParameterFloat;
using API = juce::AudioParameterInt;
using Group = juce::AudioProcessorParameterGroup;

juce::NormalisableRange<float> percentRange()
{
    return { 0.0f, 100.0f, 0.0f };
}

// Envelope time range: 1 ms .. 10 s, log-ish skew so the knob is musical.
juce::NormalisableRange<float> envTimeRange()
{
    return { 1.0f, 10000.0f, 0.0f, 0.3f };
}

auto msAttributes()
{
    return juce::AudioParameterFloatAttributes{}.withLabel ("ms");
}

auto percentAttributes()
{
    return juce::AudioParameterFloatAttributes{}.withLabel ("%");
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    auto osc = std::make_unique<Group> ("osc", "Oscillator", "|");
    osc->addChild (
        std::make_unique<API> (juce::ParameterID { ParamIDs::oscSawCount, 1 }, "Saws", 1, 16, 7),
        std::make_unique<APF> (juce::ParameterID { ParamIDs::oscDetune, 1 }, "Detune",
                               percentRange(), 25.0f, percentAttributes()),
        std::make_unique<APF> (juce::ParameterID { ParamIDs::oscBlend, 1 }, "Blend",
                               percentRange(), 60.0f, percentAttributes()),
        std::make_unique<APF> (juce::ParameterID { ParamIDs::oscWidth, 1 }, "Width",
                               percentRange(), 80.0f, percentAttributes()),
        std::make_unique<API> (juce::ParameterID { ParamIDs::oscOctave, 1 }, "Octave", -2, 2, 0));

    auto loop = std::make_unique<Group> ("loop", "Feedback Loop", "|");
    {
        // Log-ish cutoff knob: midpoint of knob travel lands at 640 Hz.
        juce::NormalisableRange<float> cutoffRange { 20.0f, 20000.0f };
        cutoffRange.setSkewForCentre (640.0f);

        loop->addChild (
            // The star knob. >100% is intentional: tanh in the loop self-limits.
            std::make_unique<APF> (juce::ParameterID { ParamIDs::fbGain, 1 }, "Feedback",
                                   juce::NormalisableRange<float> { 0.0f, 110.0f, 0.0f, 0.7f },
                                   40.0f, percentAttributes()),
            std::make_unique<APF> (juce::ParameterID { ParamIDs::fbKeytrack, 1 }, "Key Track",
                                   percentRange(), 100.0f, percentAttributes()),
            std::make_unique<APF> (juce::ParameterID { ParamIDs::fbTune, 1 }, "Loop Tune",
                                   juce::NormalisableRange<float> { -24.0f, 24.0f, 0.1f }, 0.0f,
                                   juce::AudioParameterFloatAttributes{}.withLabel ("st")),
            std::make_unique<juce::AudioParameterChoice> (
                juce::ParameterID { ParamIDs::fbFilterMode, 1 }, "Loop Filter",
                juce::StringArray { "LP", "BP" }, 0),
            std::make_unique<APF> (juce::ParameterID { ParamIDs::fbCutoff, 1 }, "Loop Cutoff",
                                   cutoffRange, 4000.0f,
                                   juce::AudioParameterFloatAttributes{}.withLabel ("Hz")),
            std::make_unique<APF> (juce::ParameterID { ParamIDs::fbReso, 1 }, "Loop Reso",
                                   juce::NormalisableRange<float> { 0.5f, 8.0f, 0.0f, 0.5f },
                                   0.71f),
            std::make_unique<APF> (juce::ParameterID { ParamIDs::fbDrive, 1 }, "Loop Drive",
                                   juce::NormalisableRange<float> { 0.0f, 24.0f, 0.0f }, 0.0f,
                                   juce::AudioParameterFloatAttributes{}.withLabel ("dB")));
    }

    auto env1 = std::make_unique<Group> ("env1", "Amp Envelope", "|");
    env1->addChild (
        std::make_unique<APF> (juce::ParameterID { ParamIDs::env1Attack, 1 }, "Amp Attack",
                               envTimeRange(), 5.0f, msAttributes()),
        std::make_unique<APF> (juce::ParameterID { ParamIDs::env1Decay, 1 }, "Amp Decay",
                               envTimeRange(), 200.0f, msAttributes()),
        std::make_unique<APF> (juce::ParameterID { ParamIDs::env1Sustain, 1 }, "Amp Sustain",
                               percentRange(), 80.0f, percentAttributes()),
        std::make_unique<APF> (juce::ParameterID { ParamIDs::env1Release, 1 }, "Amp Release",
                               envTimeRange(), 300.0f, msAttributes()));

    auto env2 = std::make_unique<Group> ("env2", "Feedback Envelope", "|");
    env2->addChild (
        std::make_unique<APF> (juce::ParameterID { ParamIDs::env2Attack, 1 }, "FB Attack",
                               envTimeRange(), 5.0f, msAttributes()),
        std::make_unique<APF> (juce::ParameterID { ParamIDs::env2Decay, 1 }, "FB Decay",
                               envTimeRange(), 400.0f, msAttributes()),
        std::make_unique<APF> (juce::ParameterID { ParamIDs::env2Sustain, 1 }, "FB Sustain",
                               percentRange(), 100.0f, percentAttributes()),
        std::make_unique<APF> (juce::ParameterID { ParamIDs::env2Release, 1 }, "FB Release",
                               envTimeRange(), 300.0f, msAttributes()),
        // 0% = feedback stays at the knob; 100% = fully shaped by this ADSR.
        std::make_unique<APF> (juce::ParameterID { ParamIDs::env2Amount, 1 }, "FB Env Amt",
                               percentRange(), 0.0f, percentAttributes()));

    auto env3 = std::make_unique<Group> ("env3", "Loop Filter Envelope", "|");
    env3->addChild (
        std::make_unique<APF> (juce::ParameterID { ParamIDs::env3Attack, 1 }, "Cut Attack",
                               envTimeRange(), 5.0f, msAttributes()),
        std::make_unique<APF> (juce::ParameterID { ParamIDs::env3Decay, 1 }, "Cut Decay",
                               envTimeRange(), 600.0f, msAttributes()),
        std::make_unique<APF> (juce::ParameterID { ParamIDs::env3Sustain, 1 }, "Cut Sustain",
                               percentRange(), 100.0f, percentAttributes()),
        std::make_unique<APF> (juce::ParameterID { ParamIDs::env3Release, 1 }, "Cut Release",
                               envTimeRange(), 300.0f, msAttributes()),
        // Bipolar: +/-100% sweeps the loop cutoff up to +/-5 octaves.
        std::make_unique<APF> (juce::ParameterID { ParamIDs::env3Amount, 1 }, "Cut Env Amt",
                               juce::NormalisableRange<float> { -100.0f, 100.0f, 0.0f }, 0.0f,
                               percentAttributes()));

    auto global = std::make_unique<Group> ("global", "Global", "|");
    global->addChild (
        // The limiter itself is always in-circuit (feedback synth safety net);
        // only its ceiling is a parameter.
        std::make_unique<APF> (juce::ParameterID { ParamIDs::limiterCeiling, 1 }, "Ceiling",
                               juce::NormalisableRange<float> { -12.0f, 0.0f, 0.0f }, -0.3f,
                               juce::AudioParameterFloatAttributes{}.withLabel ("dB")),
        std::make_unique<API> (juce::ParameterID { ParamIDs::polyphony, 1 }, "Voices", 1, 16, 8),
        std::make_unique<APF> (juce::ParameterID { ParamIDs::glideTime, 1 }, "Glide",
                               juce::NormalisableRange<float> { 0.0f, 2000.0f, 0.0f, 0.3f }, 0.0f,
                               msAttributes()),
        std::make_unique<APF> (juce::ParameterID { ParamIDs::masterVolume, 1 }, "Master",
                               juce::NormalisableRange<float> { -60.0f, 6.0f, 0.0f }, -6.0f,
                               juce::AudioParameterFloatAttributes{}
                                   .withLabel ("dB")
                                   .withStringFromValueFunction ([] (float v, int)
                                   {
                                       return v <= -59.9f ? juce::String ("-inf")
                                                          : juce::String (v, 1) + " dB";
                                   })));

    return { std::move (osc), std::move (loop), std::move (env1),
             std::move (env2), std::move (env3), std::move (global) };
}
