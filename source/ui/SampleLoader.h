#pragma once

#include "../PluginProcessor.h"

// Load button + current-sample name. The name is read from the state tree
// (pure-function-UI: the tree is the truth; refresh() re-reads it).
class SampleLoader : public juce::Component
{
public:
    explicit SampleLoader (FolieAudioProcessor& p) : processor (p)
    {
        loadButton.setButtonText ("Load...");
        loadButton.onClick = [this]
        {
            chooser = std::make_unique<juce::FileChooser> (
                "Load exciter sample", juce::File(),
                "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
            chooser->launchAsync (juce::FileBrowserComponent::openMode
                                      | juce::FileBrowserComponent::canSelectFiles,
                                  [this] (const juce::FileChooser& fc)
                                  {
                                      const auto file = fc.getResult();
                                      if (file.existsAsFile())
                                          processor.loadSampleFromFile (file);
                                      refresh();
                                  });
        };
        addAndMakeVisible (loadButton);

        nameLabel.setJustificationType (juce::Justification::centred);
        nameLabel.setFont (juce::FontOptions (11.0f));
        nameLabel.setMinimumHorizontalScale (0.6f);
        addAndMakeVisible (nameLabel);
        refresh();
    }

    void refresh()
    {
        const auto name = processor.getSampleName();
        nameLabel.setText (name.isEmpty() ? "(drop audio here)" : name,
                           juce::dontSendNotification);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (2);
        area.removeFromTop (14); // align with knob labels
        loadButton.setBounds (area.removeFromTop (22));
        nameLabel.setBounds (area);
    }

private:
    FolieAudioProcessor& processor;
    juce::TextButton loadButton;
    juce::Label nameLabel;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleLoader)
};
