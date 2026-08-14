#pragma once

#include "../PluginProcessor.h"
#include "Knob.h"

// One rack slot: enable toggle (doubles as the module name), chain-order
// buttons, and the module's own knobs. The order buttons write through the
// processor to the loopOrder state property — the editor re-lays out from
// state, staying a pure function of it.
class ModuleStrip : public juce::Component
{
public:
    ModuleStrip (FolieAudioProcessor& proc, LoopModuleID id, const juce::String& title,
                 const char* enableParamID, std::initializer_list<const char*> knobParamIDs)
        : processor (proc), moduleID (id)
    {
        enable.setButtonText (title);
        enable.setTooltip (describeParam (enableParamID));
        addAndMakeVisible (enable);
        enableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            proc.apvts, enableParamID, enable);

        moveLeft.setButtonText ("<");
        moveRight.setButtonText (">");
        moveLeft.onClick = [this] { processor.moveLoopModule (moduleID, -1); };
        moveRight.onClick = [this] { processor.moveLoopModule (moduleID, +1); };
        addAndMakeVisible (moveLeft);
        addAndMakeVisible (moveRight);

        for (auto* pid : knobParamIDs)
        {
            knobs.push_back (std::make_unique<Knob> (proc.apvts, pid));
            addAndMakeVisible (*knobs.back());
        }
    }

    int knobCount() const { return (int) knobs.size(); }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (juce::Colour (0x14ffffff));
        g.fillRoundedRectangle (bounds, 6.0f);
        g.setColour (juce::Colour (0x28ffffff));
        g.drawRoundedRectangle (bounds, 6.0f, 1.0f);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (5);

        auto header = area.removeFromTop (20);
        moveLeft.setBounds (header.removeFromLeft (20));
        moveRight.setBounds (header.removeFromRight (20));
        enable.setBounds (header);

        if (knobs.empty())
            return;

        const int w = area.getWidth() / (int) knobs.size();
        for (auto& k : knobs)
            k->setBounds (area.removeFromLeft (w));
    }

private:
    FolieAudioProcessor& processor;
    LoopModuleID moduleID;

    juce::ToggleButton enable;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableAttachment;
    juce::TextButton moveLeft, moveRight;
    std::vector<std::unique_ptr<Knob>> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleStrip)
};
