#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Rotary slider + name label + APVTS attachment in one bundle. The attachment
// is the only link between control and state (pure-function-UI rule).
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setMouseDragSensitivity (120);
        slider.setScrollWheelEnabled (true);
        slider.setSliderSnapsToMousePosition (false);
        addAndMakeVisible (slider);

        if (auto* param = apvts.getParameter (paramID))
            label.setText (param->getName (24), juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::FontOptions (13.0f));
        label.setMinimumHorizontalScale (0.7f);
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            apvts, paramID, slider);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        label.setBounds (area.removeFromTop (15));

        // Text box sized to what actually fits, so it can never swallow the
        // rotary's drag area (a fixed 68 px box used to eat narrow knobs whole).
        const int boxWidth = juce::jmin (area.getWidth() - 4, 64);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, boxWidth, 14);
        slider.setBounds (area);
    }

private:
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Knob)
};
