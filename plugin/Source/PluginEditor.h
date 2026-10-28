// Dvlce Duet - editor: speaker meters, mode controls, knobs.
// Copyright 2026 Yuval Olsha.  PolyForm Noncommercial 1.0.0 (see LICENSE).
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "ui/QuantumHeader.h"

class DvlceDuetEditor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget, private juce::Timer {
public:
    explicit DvlceDuetEditor(DvlceDuetProcessor&);
    ~DvlceDuetEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Drop a stereo file anywhere on the plugin to split it (the built-in mono-izer).
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void startSplit(const juce::File& file);

private:
    struct Knob {
        juce::Slider slider{juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow};
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };
    struct Choice {
        juce::ComboBox box;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attach;
    };

    void timerCallback() override;
    void setupKnob(Knob&, const char* id, const char* text, const char* tip);
    void setupChoice(Choice&, const char* id, const char* text, const char* tip);
    void paintMeter(juce::Graphics&, juce::Rectangle<int> r, int speaker, const juce::String& name) const;

    DvlceDuetProcessor& proc;
    dvlce::ui::DvlceLookAndFeel look{dvlce::ui::ThemeId::Quantum};
    dvlce::ui::QuantumHeader header;
    juce::Image background;
    juce::TooltipWindow tooltips{this, 600};
    Knob thrA, thrB, depth, bleed, hysteresis, open, hold, close, lookahead, hp;
    Choice mode, output, overlap;
    juce::ToggleButton idleGate{"Gate when idle"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> idleAttach;
    juce::Label status;
    juce::Rectangle<int> meterArea, panelArea;

    juce::Label splitLabel, leftLabel, rightLabel, splitStatus;
    juce::TextEditor leftPrefix, rightPrefix;
    juce::TextButton splitButton{"Split a stereo file..."};
    std::unique_ptr<juce::FileChooser> chooser;
    bool splitting = false;
    juce::ThreadPool splitPool{1};  // declared last: destroyed first, waits for a running split

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DvlceDuetEditor)
};
