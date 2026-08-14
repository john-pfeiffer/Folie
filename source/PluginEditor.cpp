#include "PluginEditor.h"
#include "params/ParameterIDs.h"

FolieAudioProcessorEditor::FolieAudioProcessorEditor (FolieAudioProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      sawCount (p.apvts, ParamIDs::oscSawCount),
      detune (p.apvts, ParamIDs::oscDetune),
      blend (p.apvts, ParamIDs::oscBlend),
      width (p.apvts, ParamIDs::oscWidth),
      octave (p.apvts, ParamIDs::oscOctave),
      attack (p.apvts, ParamIDs::env1Attack),
      decay (p.apvts, ParamIDs::env1Decay),
      sustain (p.apvts, ParamIDs::env1Sustain),
      release (p.apvts, ParamIDs::env1Release),
      polyphony (p.apvts, ParamIDs::polyphony),
      glide (p.apvts, ParamIDs::glideTime),
      master (p.apvts, ParamIDs::masterVolume)
{
    oscSection.addItem (sawCount);
    oscSection.addItem (detune);
    oscSection.addItem (blend);
    oscSection.addItem (width);
    oscSection.addItem (octave);
    addAndMakeVisible (oscSection);

    envSection.addItem (attack);
    envSection.addItem (decay);
    envSection.addItem (sustain);
    envSection.addItem (release);
    addAndMakeVisible (envSection);

    globalSection.addItem (polyphony);
    globalSection.addItem (glide);
    globalSection.addItem (master);
    addAndMakeVisible (globalSection);

    setResizable (true, true);
    setResizeLimits (520, 360, 1560, 1080);

    const auto saved = processor.getSavedEditorSize();
    setSize (saved.x, saved.y);
}

void FolieAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff141218));
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("FOLIE", getLocalBounds().removeFromTop (36).reduced (12, 4),
                juce::Justification::centredLeft);
}

void FolieAudioProcessorEditor::resized()
{
    processor.setSavedEditorSize (getWidth(), getHeight());

    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (32);

    const int rowHeight = area.getHeight() / 3;
    oscSection.setBounds (area.removeFromTop (rowHeight));
    envSection.setBounds (area.removeFromTop (rowHeight));
    globalSection.setBounds (area);
}
