#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// ToggleButton + APVTS attachment bundle; attachment-only, like Knob.
class Toggle : public juce::Component
{
public:
    Toggle (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID)
    {
        if (auto* param = apvts.getParameter (paramID))
            button.setButtonText (param->getName (24));
        addAndMakeVisible (button);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            apvts, paramID, button);
    }

    void resized() override
    {
        button.setBounds (getLocalBounds().reduced (2));
    }

private:
    juce::ToggleButton button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Toggle)
};
