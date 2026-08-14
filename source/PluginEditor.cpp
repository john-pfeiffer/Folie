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
      chorusOn (p.apvts, ParamIDs::chorusOn),
      chorusRate (p.apvts, ParamIDs::chorusRate),
      chorusDepth (p.apvts, ParamIDs::chorusDepth),
      chorusMix (p.apvts, ParamIDs::chorusMix),
      delayOn (p.apvts, ParamIDs::delayOn),
      delayTime (p.apvts, ParamIDs::delayTime),
      delayFeedback (p.apvts, ParamIDs::delayFeedback),
      delayMix (p.apvts, ParamIDs::delayMix),
      reverbOn (p.apvts, ParamIDs::reverbOn),
      reverbSize (p.apvts, ParamIDs::reverbSize),
      reverbDamp (p.apvts, ParamIDs::reverbDamp),
      reverbMix (p.apvts, ParamIDs::reverbMix),
      ceiling (p.apvts, ParamIDs::limiterCeiling),
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

    chorusSection.addItem (chorusOn);
    chorusSection.addItem (chorusRate);
    chorusSection.addItem (chorusDepth);
    chorusSection.addItem (chorusMix);
    addAndMakeVisible (chorusSection);

    delaySection.addItem (delayOn);
    delaySection.addItem (delayTime);
    delaySection.addItem (delayFeedback);
    delaySection.addItem (delayMix);
    addAndMakeVisible (delaySection);

    reverbSection.addItem (reverbOn);
    reverbSection.addItem (reverbSize);
    reverbSection.addItem (reverbDamp);
    reverbSection.addItem (reverbMix);
    addAndMakeVisible (reverbSection);

    globalSection.addItem (ceiling);
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

    const int rowHeight = area.getHeight() / 5;
    oscSection.setBounds (area.removeFromTop (rowHeight));
    loopSection.setBounds (area.removeFromTop (rowHeight));

    // Three envelopes share one row: AMP (4 knobs) | FEEDBACK (5) | CUTOFF (5).
    auto envRow = area.removeFromTop (rowHeight);
    envSection.setBounds (envRow.removeFromLeft (envRow.getWidth() * 4 / 14));
    env2Section.setBounds (envRow.removeFromLeft (envRow.getWidth() / 2));
    env3Section.setBounds (envRow);

    // FX row: CHORUS | DELAY | REVERB, 4 slots each.
    auto fxRow = area.removeFromTop (rowHeight);
    const int fxWidth = fxRow.getWidth() / 3;
    chorusSection.setBounds (fxRow.removeFromLeft (fxWidth));
    delaySection.setBounds (fxRow.removeFromLeft (fxWidth));
    reverbSection.setBounds (fxRow);

    globalSection.setBounds (area);
}
