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

    auto global = std::make_unique<Group> ("global", "Global", "|");
    global->addChild (
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

    return { std::move (osc), std::move (env1), std::move (global) };
}
