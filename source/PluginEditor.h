#pragma once

#include "PluginProcessor.h"
#include "params/ParameterIDs.h"
#include "ui/Knob.h"
#include "ui/ModuleStrip.h"
#include "ui/SectionPanel.h"

class FolieAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  private juce::ValueTree::Listener
{
public:
    explicit FolieAudioProcessorEditor (FolieAudioProcessor&);
    ~FolieAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier& property) override
    {
        if (property == juce::Identifier ("loopOrder"))
            resized();
    }

    void valueTreeRedirected (juce::ValueTree&) override { resized(); }

    ModuleStrip* stripFor (juce::uint8 moduleID);

    FolieAudioProcessor& processor;

    SectionPanel oscSection { "OSCILLATOR" };
    Knob sawCount, detune, blend, width, octave;

    SectionPanel sourceSection { "SOURCE" };
    Knob sawLevel, noiseLevel, noiseType;

    SectionPanel loopSection { "FEEDBACK LOOP" };
    Knob fbGain, fbKeytrack, fbTune;

    // Rack strips (drawn in loopOrder order)
    ModuleStrip filterStrip, satStrip, echoStrip, diffStrip, ringStrip;

    SectionPanel envSection { "AMP ENV" };
    Knob attack, decay, sustain, release;

    SectionPanel env2Section { "FEEDBACK ENV" };
    Knob fbAttack, fbDecay, fbSustain, fbRelease, fbAmount;

    SectionPanel env3Section { "CUTOFF ENV" };
    Knob cutAttack, cutDecay, cutSustain, cutRelease, cutAmount;

    SectionPanel globalSection { "GLOBAL" };
    Knob voiceMode, polyphony, glide, master;

    juce::Rectangle<int> rackLabelArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FolieAudioProcessorEditor)
};
