#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

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

    SynthEngine engine;
    juce::SmoothedValue<float> masterGain { 0.5f };

    struct RawParams
    {
        std::atomic<float>* sawCount = nullptr;
        std::atomic<float>* detune = nullptr;
        std::atomic<float>* blend = nullptr;
        std::atomic<float>* width = nullptr;
        std::atomic<float>* octave = nullptr;
        std::atomic<float>* env1A = nullptr;
        std::atomic<float>* env1D = nullptr;
        std::atomic<float>* env1S = nullptr;
        std::atomic<float>* env1R = nullptr;
        std::atomic<float>* polyphony = nullptr;
        std::atomic<float>* glide = nullptr;
        std::atomic<float>* master = nullptr;
    } raw;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FolieAudioProcessor)
};
