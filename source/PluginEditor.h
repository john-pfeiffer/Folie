#pragma once

#include "PluginProcessor.h"
#include "ui/Knob.h"
#include "ui/SectionPanel.h"

class FolieAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit FolieAudioProcessorEditor (FolieAudioProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FolieAudioProcessor& processor;

    SectionPanel oscSection { "OSCILLATOR" };
    Knob sawCount, detune, blend, width, octave;

    SectionPanel loopSection { "FEEDBACK LOOP" };
    Knob fbGain, fbKeytrack, fbTune, fbFilterMode, fbCutoff, fbReso, fbDrive;

    SectionPanel envSection { "AMP ENV" };
    Knob attack, decay, sustain, release;

    SectionPanel env2Section { "FEEDBACK ENV" };
    Knob fbAttack, fbDecay, fbSustain, fbRelease, fbAmount;

    SectionPanel env3Section { "CUTOFF ENV" };
    Knob cutAttack, cutDecay, cutSustain, cutRelease, cutAmount;

    SectionPanel globalSection { "GLOBAL" };
    Knob ceiling, polyphony, glide, master;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FolieAudioProcessorEditor)
};
