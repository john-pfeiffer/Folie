#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Titled section that lays out its child components in a row.
class SectionPanel : public juce::Component
{
public:
    explicit SectionPanel (const juce::String& titleIn) : title (titleIn) {}

    void addItem (juce::Component& c)
    {
        items.push_back (&c);
        addAndMakeVisible (c);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (juce::Colour (0x18ffffff));
        g.fillRoundedRectangle (bounds, 8.0f);
        g.setColour (juce::Colour (0x30ffffff));
        g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

        g.setColour (juce::Colours::white.withAlpha (0.7f));
        g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
        g.drawText (title, getLocalBounds().removeFromTop (22).reduced (10, 2),
                    juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (8);
        area.removeFromTop (20);

        if (items.empty())
            return;

        const int w = area.getWidth() / (int) items.size();
        for (auto* item : items)
            item->setBounds (area.removeFromLeft (w).reduced (2));
    }

private:
    juce::String title;
    std::vector<juce::Component*> items;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SectionPanel)
};
