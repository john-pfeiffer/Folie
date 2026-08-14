#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "dsp/SynthEngine.h"
#include "params/LoopOrder.h"

class FolieAudioProcessor : public juce::AudioProcessor
{
public:
    FolieAudioProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Editor size persistence (pure-function-UI rule: the state tree is the
    // single home for everything, including UI size).
    void setSavedEditorSize (int w, int h);
    juce::Point<int> getSavedEditorSize() const;

    // Loop rack order: state property (message thread) bridged to the audio
    // thread via a packed atomic. Message thread only.
    LoopOrder::Order getLoopOrder() const;
    void setLoopOrder (const LoopOrder::Order& order);
    void moveLoopModule (LoopModuleID id, int delta);

private:
    EngineParams gatherParams() const;

    SynthEngine engine;
    juce::SmoothedValue<float> masterGain { 0.5f };
    std::atomic<juce::uint32> packedOrder { LoopOrder::pack (LoopOrder::canonical) };

    struct RawParams
    {
        std::atomic<float>* sawCount = nullptr;
        std::atomic<float>* detune = nullptr;
        std::atomic<float>* blend = nullptr;
        std::atomic<float>* width = nullptr;
        std::atomic<float>* octave = nullptr;
        std::atomic<float>* fbGain = nullptr;
        std::atomic<float>* fbKeytrack = nullptr;
        std::atomic<float>* fbTune = nullptr;
        std::atomic<float>* fbFilterMode = nullptr;
        std::atomic<float>* fbCutoff = nullptr;
        std::atomic<float>* fbReso = nullptr;
        std::atomic<float>* fbDrive = nullptr;
        std::atomic<float>* fxFilterOn = nullptr;
        std::atomic<float>* fxSatOn = nullptr;
        std::atomic<float>* fxSatMode = nullptr;
        std::atomic<float>* fxEchoOn = nullptr;
        std::atomic<float>* fxEchoSync = nullptr;
        std::atomic<float>* fxEchoTime = nullptr;
        std::atomic<float>* fxEchoAmt = nullptr;
        std::atomic<float>* fxDiffOn = nullptr;
        std::atomic<float>* fxDiffSize = nullptr;
        std::atomic<float>* fxDiffAmt = nullptr;
        std::atomic<float>* fxRingOn = nullptr;
        std::atomic<float>* fxRingMode = nullptr;
        std::atomic<float>* fxRingHz = nullptr;
        std::atomic<float>* fxRingRatio = nullptr;
        std::atomic<float>* fxRingMix = nullptr;
        std::atomic<float>* env1A = nullptr;
        std::atomic<float>* env1D = nullptr;
        std::atomic<float>* env1S = nullptr;
        std::atomic<float>* env1R = nullptr;
        std::atomic<float>* env2A = nullptr;
        std::atomic<float>* env2D = nullptr;
        std::atomic<float>* env2S = nullptr;
        std::atomic<float>* env2R = nullptr;
        std::atomic<float>* env2Amt = nullptr;
        std::atomic<float>* env3A = nullptr;
        std::atomic<float>* env3D = nullptr;
        std::atomic<float>* env3S = nullptr;
        std::atomic<float>* env3R = nullptr;
        std::atomic<float>* env3Amt = nullptr;
        std::atomic<float>* voiceMode = nullptr;
        std::atomic<float>* polyphony = nullptr;
        std::atomic<float>* glide = nullptr;
        std::atomic<float>* master = nullptr;
    } raw;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FolieAudioProcessor)
};
