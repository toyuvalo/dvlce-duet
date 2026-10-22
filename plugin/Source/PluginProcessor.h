// Dvlce Duet - processor: bus layouts, parameters, and the engine.
// Copyright 2026 Yuval Olsha.  PolyForm Noncommercial 1.0.0 (see LICENSE).
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>

#include "duet/duet.h"

class DvlceDuetProcessor : public juce::AudioProcessor {
public:
    // Mode: how the two speakers reach the plugin.
    //   StereoPair: one stereo track, left = speaker A, right = speaker B.  Works in every host.
    //   Sidechain:  this track = speaker A, the sidechain input = speaker B.  Put one instance on each
    //               speaker's track and key each from the other.
    enum Mode { StereoPair = 0, Sidechain = 1 };
    enum Output { Separate = 0, Mixed = 1 };

    DvlceDuetProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Dvlce Duet"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState params;

    // Meters (written on the audio thread, read by the editor).
    std::atomic<float> meterLevel[2]{{-200.0f}, {-200.0f}}, meterGain[2]{{1.0f}, {1.0f}};
    std::atomic<bool> meterTalking[2]{{false}, {false}};
    std::atomic<bool> sidechainConnected{false};  // the host gave the sidechain bus channels
    std::atomic<bool> stereoInput{true};          // the main bus has two channels

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    duet::Params readParams() const;

    duet::Engine engine;
    duet::Delay delay[2];
    juce::AudioBuffer<float> scratch;  // detector A, detector B, gain A, gain B
    int reportedLatency = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DvlceDuetProcessor)
};
