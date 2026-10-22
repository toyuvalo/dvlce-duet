// Dvlce Duet - editor: speaker meters, mode controls, knobs.
// Copyright 2026 Yuval Olsha.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

class DvlceDuetEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit DvlceDuetEditor(DvlceDuetProcessor&);
    ~DvlceDuetEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

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
    juce::LookAndFeel_V4 look;
    juce::TooltipWindow tooltips{this, 600};
    Knob thrA, thrB, depth, bleed, hysteresis, open, hold, close, lookahead, hp;
    Choice mode, output, overlap;
    juce::ToggleButton idleGate{"Gate when idle"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> idleAttach;
    juce::Label status;
    juce::Rectangle<int> meterArea, panelArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DvlceDuetEditor)
};
