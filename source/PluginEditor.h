#pragma once

#include "PluginProcessor.h"

class FolieAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit FolieAudioProcessorEditor (FolieAudioProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override {}

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FolieAudioProcessorEditor)
};
