#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/SoftClip.h"
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
    raw.srcSawLevel   = apvts.getRawParameterValue (ParamIDs::srcSawLevel);
    raw.srcNoiseLevel = apvts.getRawParameterValue (ParamIDs::srcNoiseLevel);
    raw.srcNoiseType  = apvts.getRawParameterValue (ParamIDs::srcNoiseType);
    raw.fbGain       = apvts.getRawParameterValue (ParamIDs::fbGain);
    raw.fbKeytrack   = apvts.getRawParameterValue (ParamIDs::fbKeytrack);
    raw.fbTune       = apvts.getRawParameterValue (ParamIDs::fbTune);
    raw.fbFilterMode = apvts.getRawParameterValue (ParamIDs::fbFilterMode);
    raw.fbCutoff     = apvts.getRawParameterValue (ParamIDs::fbCutoff);
    raw.fbReso       = apvts.getRawParameterValue (ParamIDs::fbReso);
    raw.fbDrive      = apvts.getRawParameterValue (ParamIDs::fbDrive);
    raw.fxFilterOn   = apvts.getRawParameterValue (ParamIDs::fxFilterOn);
    raw.fxSatOn      = apvts.getRawParameterValue (ParamIDs::fxSatOn);
    raw.fxSatMode    = apvts.getRawParameterValue (ParamIDs::fxSatMode);
    raw.fxEchoOn     = apvts.getRawParameterValue (ParamIDs::fxEchoOn);
    raw.fxEchoSync   = apvts.getRawParameterValue (ParamIDs::fxEchoSync);
    raw.fxEchoTime   = apvts.getRawParameterValue (ParamIDs::fxEchoTime);
    raw.fxEchoAmt    = apvts.getRawParameterValue (ParamIDs::fxEchoAmt);
    raw.fxDiffOn     = apvts.getRawParameterValue (ParamIDs::fxDiffOn);
    raw.fxDiffSize   = apvts.getRawParameterValue (ParamIDs::fxDiffSize);
    raw.fxDiffAmt    = apvts.getRawParameterValue (ParamIDs::fxDiffAmt);
    raw.fxRingOn     = apvts.getRawParameterValue (ParamIDs::fxRingOn);
    raw.fxRingMode   = apvts.getRawParameterValue (ParamIDs::fxRingMode);
    raw.fxRingHz     = apvts.getRawParameterValue (ParamIDs::fxRingHz);
    raw.fxRingRatio  = apvts.getRawParameterValue (ParamIDs::fxRingRatio);
    raw.fxRingMix    = apvts.getRawParameterValue (ParamIDs::fxRingMix);
    raw.env1A     = apvts.getRawParameterValue (ParamIDs::env1Attack);
    raw.env1D     = apvts.getRawParameterValue (ParamIDs::env1Decay);
    raw.env1S     = apvts.getRawParameterValue (ParamIDs::env1Sustain);
    raw.env1R     = apvts.getRawParameterValue (ParamIDs::env1Release);
    raw.env2A     = apvts.getRawParameterValue (ParamIDs::env2Attack);
    raw.env2D     = apvts.getRawParameterValue (ParamIDs::env2Decay);
    raw.env2S     = apvts.getRawParameterValue (ParamIDs::env2Sustain);
    raw.env2R     = apvts.getRawParameterValue (ParamIDs::env2Release);
    raw.env2Amt   = apvts.getRawParameterValue (ParamIDs::env2Amount);
    raw.env3A     = apvts.getRawParameterValue (ParamIDs::env3Attack);
    raw.env3D     = apvts.getRawParameterValue (ParamIDs::env3Decay);
    raw.env3S     = apvts.getRawParameterValue (ParamIDs::env3Sustain);
    raw.env3R     = apvts.getRawParameterValue (ParamIDs::env3Release);
    raw.env3Amt   = apvts.getRawParameterValue (ParamIDs::env3Amount);
    raw.voiceMode = apvts.getRawParameterValue (ParamIDs::voiceMode);
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
    p.voice.srcSawLevel   = raw.srcSawLevel->load() * 0.01f;
    p.voice.srcNoiseLevel = raw.srcNoiseLevel->load() * 0.01f;
    p.voice.srcNoisePink  = raw.srcNoiseType->load() > 0.5f;
    p.voice.fbGain        = raw.fbGain->load() * 0.01f;
    p.voice.fbKeytrack    = raw.fbKeytrack->load() * 0.01f;
    p.voice.fbTuneSemis   = raw.fbTune->load();
    p.voice.fbBandpass    = raw.fbFilterMode->load() > 0.5f;
    p.voice.fbCutoff      = raw.fbCutoff->load();
    p.voice.fbReso        = raw.fbReso->load();
    p.voice.fbDriveDb     = raw.fbDrive->load();
    p.voice.fxFilterOn    = raw.fxFilterOn->load() > 0.5f;
    p.voice.fxSatOn       = raw.fxSatOn->load() > 0.5f;
    p.voice.fxSatMode     = (int) raw.fxSatMode->load();
    p.voice.fxEchoOn      = raw.fxEchoOn->load() > 0.5f;
    p.voice.fxEchoSync    = (int) raw.fxEchoSync->load();
    p.voice.fxEchoTimeMs  = raw.fxEchoTime->load();
    p.voice.fxEchoAmt     = raw.fxEchoAmt->load() * 0.01f;
    p.voice.fxDiffOn      = raw.fxDiffOn->load() > 0.5f;
    p.voice.fxDiffSize    = raw.fxDiffSize->load() * 0.01f;
    p.voice.fxDiffAmt     = raw.fxDiffAmt->load() * 0.01f;
    p.voice.fxRingOn      = raw.fxRingOn->load() > 0.5f;
    p.voice.fxRingMode    = (int) raw.fxRingMode->load();
    p.voice.fxRingHz      = raw.fxRingHz->load();
    p.voice.fxRingRatio   = raw.fxRingRatio->load();
    p.voice.fxRingMix     = raw.fxRingMix->load() * 0.01f;
    p.voice.loopOrder     = LoopOrder::unpack (packedOrder.load (std::memory_order_acquire));
    p.voice.env1AttackMs  = raw.env1A->load();
    p.voice.env1DecayMs   = raw.env1D->load();
    p.voice.env1Sustain   = raw.env1S->load() * 0.01f;
    p.voice.env1ReleaseMs = raw.env1R->load();
    p.voice.env2AttackMs  = raw.env2A->load();
    p.voice.env2DecayMs   = raw.env2D->load();
    p.voice.env2Sustain   = raw.env2S->load() * 0.01f;
    p.voice.env2ReleaseMs = raw.env2R->load();
    p.voice.env2Amount    = raw.env2Amt->load() * 0.01f;
    p.voice.env3AttackMs  = raw.env3A->load();
    p.voice.env3DecayMs   = raw.env3D->load();
    p.voice.env3Sustain   = raw.env3S->load() * 0.01f;
    p.voice.env3ReleaseMs = raw.env3R->load();
    p.voice.env3Amount    = raw.env3Amt->load() * 0.01f;
    const int mode        = (int) raw.voiceMode->load();
    p.mode                = mode == 1 ? VoiceMode::mono
                          : mode == 2 ? VoiceMode::legato
                                      : VoiceMode::poly;
    p.polyphony           = (int) raw.polyphony->load();
    p.glideSeconds        = raw.glide->load() * 0.001f;
    return p;
}

void FolieAudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    engine.setParams (gatherParams());
    masterGain.reset (sampleRate, 0.02);
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

    // Fixed safety soft-clip — the only bus stage (zero post FX by design).
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            data[i] = SafetyClip::process (data[i]);
    }
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
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));

            // replaceState swaps the whole tree, so non-parameter state must
            // be re-bridged to the audio thread explicitly.
            const auto order = LoopOrder::fromString (
                apvts.state.getProperty ("loopOrder", LoopOrder::toString (LoopOrder::canonical)));
            packedOrder.store (LoopOrder::pack (order), std::memory_order_release);
        }
    }
}

LoopOrder::Order FolieAudioProcessor::getLoopOrder() const
{
    return LoopOrder::fromString (
        apvts.state.getProperty ("loopOrder", LoopOrder::toString (LoopOrder::canonical)));
}

void FolieAudioProcessor::setLoopOrder (const LoopOrder::Order& orderIn)
{
    const auto order = LoopOrder::sanitize (orderIn);
    apvts.state.setProperty ("loopOrder", LoopOrder::toString (order), nullptr);
    packedOrder.store (LoopOrder::pack (order), std::memory_order_release);
}

void FolieAudioProcessor::moveLoopModule (LoopModuleID id, int delta)
{
    auto order = getLoopOrder();
    for (int i = 0; i < numLoopModules; ++i)
    {
        if (order[(size_t) i] == (juce::uint8) id)
        {
            const int j = juce::jlimit (0, numLoopModules - 1, i + delta);
            std::swap (order[(size_t) i], order[(size_t) j]);
            setLoopOrder (order);
            return;
        }
    }
}

void FolieAudioProcessor::setSavedEditorSize (int w, int h)
{
    apvts.state.setProperty ("uiWidth", w, nullptr);
    apvts.state.setProperty ("uiHeight", h, nullptr);
}

juce::Point<int> FolieAudioProcessor::getSavedEditorSize() const
{
    return { (int) apvts.state.getProperty ("uiWidth", 1100),
             (int) apvts.state.getProperty ("uiHeight", 740) };
}

juce::AudioProcessorEditor* FolieAudioProcessor::createEditor()
{
    return new FolieAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FolieAudioProcessor();
}
