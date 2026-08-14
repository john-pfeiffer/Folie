#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "params/ParameterIDs.h"
#include "params/ParameterLayout.h"

FolieAudioProcessor::FolieAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "FOLIE", createParameterLayout())
{
    raw.sawCount  = apvts.getRawParameterValue (ParamIDs::oscSawCount);
    raw.detune    = apvts.getRawParameterValue (ParamIDs::oscDetune);
    raw.blend     = apvts.getRawParameterValue (ParamIDs::oscBlend);
    raw.width     = apvts.getRawParameterValue (ParamIDs::oscWidth);
    raw.octave    = apvts.getRawParameterValue (ParamIDs::oscOctave);
    raw.fbGain       = apvts.getRawParameterValue (ParamIDs::fbGain);
    raw.fbKeytrack   = apvts.getRawParameterValue (ParamIDs::fbKeytrack);
    raw.fbTune       = apvts.getRawParameterValue (ParamIDs::fbTune);
    raw.fbFilterMode = apvts.getRawParameterValue (ParamIDs::fbFilterMode);
    raw.fbCutoff     = apvts.getRawParameterValue (ParamIDs::fbCutoff);
    raw.fbReso       = apvts.getRawParameterValue (ParamIDs::fbReso);
    raw.fbDrive      = apvts.getRawParameterValue (ParamIDs::fbDrive);
    raw.limiterCeiling = apvts.getRawParameterValue (ParamIDs::limiterCeiling);
    raw.env1A     = apvts.getRawParameterValue (ParamIDs::env1Attack);
    raw.env1D     = apvts.getRawParameterValue (ParamIDs::env1Decay);
    raw.env1S     = apvts.getRawParameterValue (ParamIDs::env1Sustain);
    raw.env1R     = apvts.getRawParameterValue (ParamIDs::env1Release);
    raw.polyphony = apvts.getRawParameterValue (ParamIDs::polyphony);
    raw.glide     = apvts.getRawParameterValue (ParamIDs::glideTime);
    raw.master    = apvts.getRawParameterValue (ParamIDs::masterVolume);
}

EngineParams FolieAudioProcessor::gatherParams() const
{
    EngineParams p;
    p.voice.sawCount      = (int) raw.sawCount->load();
    p.voice.detune        = raw.detune->load() * 0.01f;
    p.voice.blend         = raw.blend->load() * 0.01f;
    p.voice.width         = raw.width->load() * 0.01f;
    p.voice.octave        = (int) raw.octave->load();
    p.voice.fbGain        = raw.fbGain->load() * 0.01f;
    p.voice.fbKeytrack    = raw.fbKeytrack->load() * 0.01f;
    p.voice.fbTuneSemis   = raw.fbTune->load();
    p.voice.fbBandpass    = raw.fbFilterMode->load() > 0.5f;
    p.voice.fbCutoff      = raw.fbCutoff->load();
    p.voice.fbReso        = raw.fbReso->load();
    p.voice.fbDriveDb     = raw.fbDrive->load();
    p.voice.env1AttackMs  = raw.env1A->load();
    p.voice.env1DecayMs   = raw.env1D->load();
    p.voice.env1Sustain   = raw.env1S->load() * 0.01f;
    p.voice.env1ReleaseMs = raw.env1R->load();
    p.polyphony           = (int) raw.polyphony->load();
    p.glideSeconds        = raw.glide->load() * 0.001f;
    return p;
}

void FolieAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    engine.setParams (gatherParams());
    masterGain.reset (sampleRate, 0.02);

    limiter.prepare ({ sampleRate, (juce::uint32) samplesPerBlock, 2 });
    limiter.setRelease (100.0f);
}

bool FolieAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void FolieAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();
    engine.setParams (gatherParams());
    engine.renderBlock (buffer, midi);

    const float masterDb = raw.master->load();
    masterGain.setTargetValue (masterDb <= -59.9f ? 0.0f
                                                  : juce::Decibels::decibelsToGain (masterDb));
    masterGain.applyGain (buffer, buffer.getNumSamples());

    limiter.setThreshold (raw.limiterCeiling->load());
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    limiter.process (context);
}

void FolieAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void FolieAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

void FolieAudioProcessor::setSavedEditorSize (int w, int h)
{
    apvts.state.setProperty ("uiWidth", w, nullptr);
    apvts.state.setProperty ("uiHeight", h, nullptr);
}

juce::Point<int> FolieAudioProcessor::getSavedEditorSize() const
{
    return { (int) apvts.state.getProperty ("uiWidth", 780),
             (int) apvts.state.getProperty ("uiHeight", 540) };
}

juce::AudioProcessorEditor* FolieAudioProcessor::createEditor()
{
    return new FolieAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FolieAudioProcessor();
}
