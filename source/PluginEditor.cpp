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
      fbGain (p.apvts, ParamIDs::fbGain),
      fbKeytrack (p.apvts, ParamIDs::fbKeytrack),
      fbTune (p.apvts, ParamIDs::fbTune),
      fbFilterMode (p.apvts, ParamIDs::fbFilterMode),
      fbCutoff (p.apvts, ParamIDs::fbCutoff),
      fbReso (p.apvts, ParamIDs::fbReso),
      fbDrive (p.apvts, ParamIDs::fbDrive),
      attack (p.apvts, ParamIDs::env1Attack),
      decay (p.apvts, ParamIDs::env1Decay),
      sustain (p.apvts, ParamIDs::env1Sustain),
      release (p.apvts, ParamIDs::env1Release),
      fbAttack (p.apvts, ParamIDs::env2Attack),
      fbDecay (p.apvts, ParamIDs::env2Decay),
      fbSustain (p.apvts, ParamIDs::env2Sustain),
      fbRelease (p.apvts, ParamIDs::env2Release),
      fbAmount (p.apvts, ParamIDs::env2Amount),
      cutAttack (p.apvts, ParamIDs::env3Attack),
      cutDecay (p.apvts, ParamIDs::env3Decay),
      cutSustain (p.apvts, ParamIDs::env3Sustain),
      cutRelease (p.apvts, ParamIDs::env3Release),
      cutAmount (p.apvts, ParamIDs::env3Amount),
      voiceMode (p.apvts, ParamIDs::voiceMode),
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

    loopSection.addItem (fbGain);
    loopSection.addItem (fbKeytrack);
    loopSection.addItem (fbTune);
    loopSection.addItem (fbFilterMode);
    loopSection.addItem (fbCutoff);
    loopSection.addItem (fbReso);
    loopSection.addItem (fbDrive);
    addAndMakeVisible (loopSection);

    envSection.addItem (attack);
    envSection.addItem (decay);
    envSection.addItem (sustain);
    envSection.addItem (release);
    addAndMakeVisible (envSection);

    env2Section.addItem (fbAttack);
    env2Section.addItem (fbDecay);
    env2Section.addItem (fbSustain);
    env2Section.addItem (fbRelease);
    env2Section.addItem (fbAmount);
    addAndMakeVisible (env2Section);

    env3Section.addItem (cutAttack);
    env3Section.addItem (cutDecay);
    env3Section.addItem (cutSustain);
    env3Section.addItem (cutRelease);
    env3Section.addItem (cutAmount);
    addAndMakeVisible (env3Section);

    globalSection.addItem (voiceMode);
    globalSection.addItem (polyphony);
    globalSection.addItem (glide);
    globalSection.addItem (master);
    addAndMakeVisible (globalSection);

    setResizable (true, true);
    setResizeLimits (900, 620, 1800, 1240);

    const auto saved = processor.getSavedEditorSize();
    setSize (juce::jmax (900, saved.x), juce::jmax (620, saved.y));
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

    const int rowHeight = area.getHeight() / 4;
    oscSection.setBounds (area.removeFromTop (rowHeight));
    loopSection.setBounds (area.removeFromTop (rowHeight));

    // Three envelopes share one row: AMP (4 knobs) | FEEDBACK (5) | CUTOFF (5).
    auto envRow = area.removeFromTop (rowHeight);
    envSection.setBounds (envRow.removeFromLeft (envRow.getWidth() * 4 / 14));
    env2Section.setBounds (envRow.removeFromLeft (envRow.getWidth() / 2));
    env3Section.setBounds (envRow);

    globalSection.setBounds (area);
}
