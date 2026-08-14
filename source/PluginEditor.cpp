#include "PluginEditor.h"

FolieAudioProcessorEditor::FolieAudioProcessorEditor (FolieAudioProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      sawCount (p.apvts, ParamIDs::oscSawCount),
      detune (p.apvts, ParamIDs::oscDetune),
      blend (p.apvts, ParamIDs::oscBlend),
      width (p.apvts, ParamIDs::oscWidth),
      octave (p.apvts, ParamIDs::oscOctave),
      sawLevel (p.apvts, ParamIDs::srcSawLevel),
      noiseLevel (p.apvts, ParamIDs::srcNoiseLevel),
      noiseType (p.apvts, ParamIDs::srcNoiseType),
      fbGain (p.apvts, ParamIDs::fbGain),
      fbKeytrack (p.apvts, ParamIDs::fbKeytrack),
      fbTune (p.apvts, ParamIDs::fbTune),
      filterStrip (p, LoopModuleID::filter, "FILTER", ParamIDs::fxFilterOn,
                   { ParamIDs::fbFilterMode, ParamIDs::fbCutoff, ParamIDs::fbReso }),
      satStrip (p, LoopModuleID::saturator, "SATURATOR", ParamIDs::fxSatOn,
                { ParamIDs::fxSatMode, ParamIDs::fbDrive }),
      echoStrip (p, LoopModuleID::echo, "ECHO", ParamIDs::fxEchoOn,
                 { ParamIDs::fxEchoSync, ParamIDs::fxEchoTime, ParamIDs::fxEchoAmt }),
      diffStrip (p, LoopModuleID::diffuser, "DIFFUSER", ParamIDs::fxDiffOn,
                 { ParamIDs::fxDiffSize, ParamIDs::fxDiffAmt }),
      ringStrip (p, LoopModuleID::ringmod, "RING MOD", ParamIDs::fxRingOn,
                 { ParamIDs::fxRingMode, ParamIDs::fxRingHz, ParamIDs::fxRingRatio,
                   ParamIDs::fxRingMix }),
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

    sourceSection.addItem (sawLevel);
    sourceSection.addItem (noiseLevel);
    sourceSection.addItem (noiseType);
    addAndMakeVisible (sourceSection);

    loopSection.addItem (fbGain);
    loopSection.addItem (fbKeytrack);
    loopSection.addItem (fbTune);
    addAndMakeVisible (loopSection);

    for (auto* strip : { &filterStrip, &satStrip, &echoStrip, &diffStrip, &ringStrip })
        addAndMakeVisible (*strip);

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

    processor.apvts.state.addListener (this);

    setResizable (true, true);
    setResizeLimits (900, 620, 1800, 1240);

    const auto saved = processor.getSavedEditorSize();
    setSize (juce::jmax (900, saved.x), juce::jmax (620, saved.y));
}

FolieAudioProcessorEditor::~FolieAudioProcessorEditor()
{
    processor.apvts.state.removeListener (this);
}

ModuleStrip* FolieAudioProcessorEditor::stripFor (juce::uint8 moduleID)
{
    switch ((LoopModuleID) moduleID)
    {
        case LoopModuleID::filter:    return &filterStrip;
        case LoopModuleID::saturator: return &satStrip;
        case LoopModuleID::echo:      return &echoStrip;
        case LoopModuleID::diffuser:  return &diffStrip;
        case LoopModuleID::ringmod:   return &ringStrip;
        default:                      return nullptr;
    }
}

void FolieAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff141218));
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("FOLIE", getLocalBounds().removeFromTop (32).reduced (12, 4),
                juce::Justification::centredLeft);

    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("LOOP FX RACK  (< > moves a module through the loop)",
                rackLabelArea, juce::Justification::centredLeft);
}

void FolieAudioProcessorEditor::resized()
{
    processor.setSavedEditorSize (getWidth(), getHeight());

    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (28);

    const int rowHeight = (area.getHeight() - 16) / 4;

    // Row 1: oscillator + source mixer + loop core side by side.
    auto row1 = area.removeFromTop (rowHeight);
    oscSection.setBounds (row1.removeFromLeft (row1.getWidth() * 5 / 11));
    sourceSection.setBounds (row1.removeFromLeft (row1.getWidth() * 3 / 6));
    loopSection.setBounds (row1);

    // Row 2: the rack, in loopOrder order.
    rackLabelArea = area.removeFromTop (16).reduced (4, 0);
    auto rackRow = area.removeFromTop (rowHeight);
    const auto order = processor.getLoopOrder();

    // Widths proportional to knob counts (3,2,3,2,4 + header space).
    int totalKnobs = 0;
    for (auto m : order)
        if (auto* s = stripFor (m))
            totalKnobs += juce::jmax (2, s->knobCount());

    for (auto m : order)
    {
        if (auto* s = stripFor (m))
        {
            const int w = rackRow.getWidth() * juce::jmax (2, s->knobCount()) / juce::jmax (1, totalKnobs);
            s->setBounds (rackRow.removeFromLeft (w));
            totalKnobs -= juce::jmax (2, s->knobCount());
        }
    }

    // Row 3: envelopes.
    auto envRow = area.removeFromTop (rowHeight);
    envSection.setBounds (envRow.removeFromLeft (envRow.getWidth() * 4 / 14));
    env2Section.setBounds (envRow.removeFromLeft (envRow.getWidth() / 2));
    env3Section.setBounds (envRow);

    globalSection.setBounds (area);
}
