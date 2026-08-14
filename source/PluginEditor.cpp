#include "PluginEditor.h"

FolieAudioProcessorEditor::FolieAudioProcessorEditor (FolieAudioProcessor& p)
    : AudioProcessorEditor (p)
{
    setResizable (true, true);
    setResizeLimits (520, 360, 1560, 1080);
    setSize (780, 540);
}

void FolieAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff141218));
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (28.0f));
    g.drawFittedText ("FOLIE — hello world (sine)", getLocalBounds(), juce::Justification::centred, 1);
}
