#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "dsp/FxChain.h"
#include "dsp/SynthEngine.h"

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

private:
    EngineParams gatherParams() const;
    FxParams gatherFxParams() const;

    SynthEngine engine;
    FxChain fx;
    juce::SmoothedValue<float> masterGain { 0.5f };
    juce::dsp::Limiter<float> limiter; // always in-circuit — feedback safety net

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
        std::atomic<float>* limiterCeiling = nullptr;
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
        std::atomic<float>* chorusOn = nullptr;
        std::atomic<float>* chorusRate = nullptr;
        std::atomic<float>* chorusDepth = nullptr;
        std::atomic<float>* chorusMix = nullptr;
        std::atomic<float>* delayOn = nullptr;
        std::atomic<float>* delayTime = nullptr;
        std::atomic<float>* delayFeedback = nullptr;
        std::atomic<float>* delayMix = nullptr;
        std::atomic<float>* reverbOn = nullptr;
        std::atomic<float>* reverbSize = nullptr;
        std::atomic<float>* reverbDamp = nullptr;
        std::atomic<float>* reverbMix = nullptr;
        std::atomic<float>* polyphony = nullptr;
        std::atomic<float>* glide = nullptr;
        std::atomic<float>* master = nullptr;
    } raw;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FolieAudioProcessor)
};
