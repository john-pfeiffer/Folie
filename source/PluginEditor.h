#pragma once

#include "PluginProcessor.h"
#include "params/ParameterIDs.h"
#include "ui/Knob.h"
#include "ui/ModuleStrip.h"
#include "ui/SampleLoader.h"
#include "ui/SectionPanel.h"

class FolieAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  public juce::FileDragAndDropTarget,
                                  private juce::ValueTree::Listener
{
public:
    explicit FolieAudioProcessorEditor (FolieAudioProcessor&);
    ~FolieAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override
    {
        for (const auto& f : files)
            if (f.endsWithIgnoreCase (".wav") || f.endsWithIgnoreCase (".aif")
                || f.endsWithIgnoreCase (".aiff") || f.endsWithIgnoreCase (".flac")
                || f.endsWithIgnoreCase (".ogg") || f.endsWithIgnoreCase (".mp3"))
                return true;
        return false;
    }

    void filesDropped (const juce::StringArray& files, int, int) override
    {
        for (const auto& f : files)
            if (processor.loadSampleFromFile (juce::File (f)))
                break;
        sampleLoader.refresh();
    }

private:
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier& property) override
    {
        if (property == juce::Identifier ("loopOrder"))
            resized();
        else if (property == juce::Identifier ("sampleName"))
            sampleLoader.refresh();
    }

    void valueTreeRedirected (juce::ValueTree&) override { resized(); }

    ModuleStrip* stripFor (juce::uint8 moduleID);

    FolieAudioProcessor& processor;

    SectionPanel oscSection { "OSCILLATOR" };
    Knob sawCount, detune, blend, width, octave;

    SectionPanel sourceSection { "SOURCE" };
    Knob sawLevel, noiseLevel, noiseType, sampleLevel, sampleRoot, sampleLoop;
    SampleLoader sampleLoader;

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
