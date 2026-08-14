#pragma once

#include "FolieVoice.h"

struct EngineParams
{
    VoiceParams voice;
    int   polyphony    = 8;      // 1..16
    float glideSeconds = 0.0f;
};

// Hand-rolled polyphonic engine over a fixed pool of 16 pre-allocated voices.
// No allocation after prepare(); the polyphony knob just limits how many pool
// voices are eligible, so it can move freely mid-performance.
class SynthEngine
{
public:
    static constexpr int maxVoices = 16;

    void prepare (double sampleRate)
    {
        for (int i = 0; i < maxVoices; ++i)
            voices[i].prepare (sampleRate, i);
        lastNoteHz = 0.0f;
    }

    void setParams (const EngineParams& p)
    {
        engineParams = p;
        for (auto& v : voices)
            v.setParams (p.voice);
    }

    void reset()
    {
        for (auto& v : voices)
            v.reset();
    }

    // Renders additively into the (pre-cleared) buffer, splitting at MIDI
    // event positions so note timing is sample-accurate.
    void renderBlock (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi)
    {
        int pos = 0;
        const int numSamples = buffer.getNumSamples();

        for (const auto metadata : midi)
        {
            const int eventTime = juce::jlimit (0, numSamples, metadata.samplePosition);
            if (eventTime > pos)
            {
                renderVoices (buffer, pos, eventTime - pos);
                pos = eventTime;
            }
            handleMidiEvent (metadata.getMessage());
        }

        if (pos < numSamples)
            renderVoices (buffer, pos, numSamples - pos);
    }

    int countActiveVoices() const
    {
        int n = 0;
        for (auto& v : voices)
            n += v.isActive() ? 1 : 0;
        return n;
    }

private:
    void handleMidiEvent (const juce::MidiMessage& m)
    {
        if (m.isNoteOn())
            noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())
            noteOff (m.getNoteNumber());
        else if (m.isPitchWheel())
        {
            const float semis = 2.0f * ((float) m.getPitchWheelValue() - 8192.0f) / 8192.0f;
            for (auto& v : voices)
                v.setPitchBend (semis);
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            for (auto& v : voices)
                v.stopNote (m.isAllNotesOff());
        }
    }

    void noteOn (int note, float velocity)
    {
        auto* voice = findVoiceFor (note);

        const float glideFrom = engineParams.glideSeconds > 0.0f ? lastNoteHz : 0.0f;
        voice->setHeld (true);
        voice->startNote (note, velocity, glideFrom, engineParams.glideSeconds);
        lastNoteHz = FolieVoice::noteHz (note);
    }

    void noteOff (int note)
    {
        for (auto& v : voices)
        {
            if (v.isActive() && v.currentNote() == note && v.isHeld())
            {
                v.setHeld (false);
                v.stopNote (true);
            }
        }
    }

    FolieVoice* findVoiceFor (int note)
    {
        const int limit = juce::jlimit (1, maxVoices, engineParams.polyphony);

        // Retrigger a voice already playing this note.
        for (int i = 0; i < limit; ++i)
            if (voices[i].isActive() && voices[i].currentNote() == note)
                return &voices[i];

        // Otherwise a free voice.
        for (int i = 0; i < limit; ++i)
            if (! voices[i].isActive())
                return &voices[i];

        // Otherwise steal: prefer the quietest releasing voice, else quietest.
        FolieVoice* best = &voices[0];
        float bestScore = 1.0e9f;
        for (int i = 0; i < limit; ++i)
        {
            float score = voices[i].envLevel() + (voices[i].isReleasing() ? 0.0f : 10.0f);
            if (score < bestScore)
            {
                bestScore = score;
                best = &voices[i];
            }
        }
        best->stopNote (false);
        return best;
    }

    void renderVoices (juce::AudioBuffer<float>& buffer, int start, int num)
    {
        for (auto& v : voices)
            v.renderNextBlock (buffer, start, num);
    }

    FolieVoice voices[maxVoices];
    EngineParams engineParams;
    float lastNoteHz = 0.0f;
};
