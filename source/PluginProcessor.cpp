#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
struct HelloSound : public juce::SynthesiserSound
{
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class HelloSineVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<HelloSound*> (sound) != nullptr;
    }

    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        phase = 0.0;
        level = velocity * 0.15f;
        auto freq = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        phaseDelta = juce::MathConstants<double>::twoPi * freq / getSampleRate();
        env.setSampleRate (getSampleRate());
        env.setParameters ({ 0.005f, 0.1f, 0.8f, 0.2f });
        env.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        if (allowTailOff)
            env.noteOff();
        else
        {
            env.reset();
            clearCurrentNote();
        }
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& output, int startSample, int numSamples) override
    {
        if (! env.isActive())
            return;

        for (int i = startSample; i < startSample + numSamples; ++i)
        {
            auto sample = level * env.getNextSample() * (float) std::sin (phase);
            phase += phaseDelta;

            for (int ch = 0; ch < output.getNumChannels(); ++ch)
                output.addSample (ch, i, sample);

            if (! env.isActive())
            {
                clearCurrentNote();
                break;
            }
        }
    }

private:
    double phase = 0.0, phaseDelta = 0.0;
    float level = 0.0f;
    juce::ADSR env;
};
} // namespace

FolieAudioProcessor::FolieAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    synth.addSound (new HelloSound());
    for (int i = 0; i < 8; ++i)
        synth.addVoice (new HelloSineVoice());
}

void FolieAudioProcessor::prepareToPlay (double sampleRate, int)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
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
    synth.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());
}

void FolieAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // No parameters yet — state becomes the APVTS tree in FOL-4.
    juce::MemoryOutputStream (destData, true).writeInt (1);
}

void FolieAudioProcessor::setStateInformation (const void*, int) {}

juce::AudioProcessorEditor* FolieAudioProcessor::createEditor()
{
    return new FolieAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FolieAudioProcessor();
}
