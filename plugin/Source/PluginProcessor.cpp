// Dvlce Duet - processor.
// Copyright 2026 Yuval Olsha.
#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace {
using Attr = juce::AudioParameterFloatAttributes;

std::unique_ptr<juce::AudioParameterFloat> dbParam(const char* id, const char* name, float lo, float hi, float def) {
    return std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name, juce::NormalisableRange<float>(lo, hi, 0.1f), def,
                                                       Attr().withLabel("dB"));
}

std::unique_ptr<juce::AudioParameterFloat> msParam(const char* id, const char* name, float lo, float hi, float def, float centre) {
    juce::NormalisableRange<float> r(lo, hi, 0.1f);
    r.setSkewForCentre(centre);
    return std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name, r, def, Attr().withLabel("ms"));
}
}  // namespace

DvlceDuetProcessor::DvlceDuetProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                         .withInput("Sidechain", juce::AudioChannelSet::stereo(), true)),
      params(*this, nullptr, "DvlceDuet", createLayout()) {}

juce::AudioProcessorValueTreeState::ParameterLayout DvlceDuetProcessor::createLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode", 1}, "Mode",
                                                       juce::StringArray{"Stereo pair", "Sidechain"}, 0));
    l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"output", 1}, "Output",
                                                       juce::StringArray{"Separate", "Mixed"}, 0));
    l.add(dbParam("thr_a", "Threshold A", -80.0f, 0.0f, -40.0f));
    l.add(dbParam("thr_b", "Threshold B", -80.0f, 0.0f, -40.0f));
    l.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"depth", 1}, "Depth", juce::NormalisableRange<float>(0.0f, duet::kMuteDb, 0.1f), 30.0f,
        Attr().withStringFromValueFunction([](float v, int) {
            return v >= duet::kMuteDb ? juce::String("Mute") : "-" + juce::String(v, 1) + " dB";
        }).withValueFromStringFunction([](const juce::String& s) {
            return s.trim().startsWithIgnoreCase("m") ? duet::kMuteDb : std::abs(s.getFloatValue());  // "-20 dB" or "20"
        })));
    l.add(dbParam("bleed", "Bleed reject", 0.0f, 40.0f, 10.0f));
    l.add(dbParam("hysteresis", "Hysteresis", 0.0f, 12.0f, 3.0f));
    l.add(msParam("open", "Open", 0.1f, 50.0f, 2.0f, 5.0f));
    l.add(msParam("hold", "Hold", 0.0f, 1000.0f, 200.0f, 200.0f));
    l.add(msParam("close", "Close", 5.0f, 2000.0f, 150.0f, 200.0f));
    l.add(msParam("lookahead", "Lookahead", 0.0f, duet::kMaxLookaheadMs, 0.0f, 5.0f));
    l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"hp", 1}, "Detector HP",
                                                      juce::NormalisableRange<float>(20.0f, 400.0f, 1.0f, 0.5f), 100.0f,
                                                      Attr().withLabel("Hz")));
    l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"overlap", 1}, "Overlap",
                                                       juce::StringArray{"Both open", "Louder wins"}, 0));
    l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"idle_gate", 1}, "Gate when idle", false));
    return l;
}

duet::Params DvlceDuetProcessor::readParams() const {
    const auto v = [this](const char* id) { return params.getRawParameterValue(id)->load(); };
    duet::Params p;
    p.thresholdADb = v("thr_a");
    p.thresholdBDb = v("thr_b");
    p.depthDb = v("depth");
    p.bleedDb = v("bleed");
    p.hysteresisDb = v("hysteresis");
    p.openMs = v("open");
    p.holdMs = v("hold");
    p.closeMs = v("close");
    p.lookaheadMs = v("lookahead");
    p.detectorHpHz = v("hp");
    p.overlap = v("overlap") > 0.5f ? duet::Overlap::LouderWins : duet::Overlap::BothOpen;
    p.gateWhenIdle = v("idle_gate") > 0.5f;
    return p;
}

bool DvlceDuetProcessor::isBusesLayoutSupported(const BusesLayout& layout) const {
    const auto in = layout.getMainInputChannelSet(), out = layout.getMainOutputChannelSet();
    if (in != out) return false;
    if (in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo()) return false;
    if (layout.inputBuses.size() > 1) {
        const auto side = layout.getChannelSet(true, 1);
        if (!side.isDisabled() && side != juce::AudioChannelSet::mono() && side != juce::AudioChannelSet::stereo()) return false;
    }
    return true;
}

void DvlceDuetProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    engine.setParams(readParams());
    engine.prepare(sampleRate);
    const int maxDelay = static_cast<int>(std::ceil(duet::kMaxLookaheadMs * 0.001 * sampleRate));
    for (auto& d : delay) {
        d.prepare(maxDelay);
        d.setDelay(engine.latencySamples());
        d.reset();
    }
    scratch.setSize(4, juce::jmax(samplesPerBlock, 512), false, true, true);
    reportedLatency = engine.latencySamples();
    setLatencySamples(reportedLatency);
}

void DvlceDuetProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    engine.setParams(readParams());
    const int lat = engine.latencySamples();
    if (lat != reportedLatency) {
        reportedLatency = lat;
        for (auto& d : delay) d.setDelay(lat);
        setLatencySamples(lat);
    }

    auto main = getBusBuffer(buffer, true, 0);
    juce::AudioBuffer<float> side;
    if (getBusCount(true) > 1 && getBus(true, 1)->isEnabled()) side = getBusBuffer(buffer, true, 1);
    const int mainCh = juce::jmin(main.getNumChannels(), 2), sideCh = side.getNumChannels();
    const bool keyed = params.getRawParameterValue("mode")->load() > 0.5f;
    const bool mixed = !keyed && mainCh == 2 && params.getRawParameterValue("output")->load() > 0.5f;
    sidechainConnected.store(sideCh > 0, std::memory_order_relaxed);
    stereoInput.store(mainCh == 2, std::memory_order_relaxed);

    const int total = buffer.getNumSamples(), cap = scratch.getNumSamples();
    for (int pos = 0; pos < total; pos += cap) {
        const int n = juce::jmin(cap, total - pos);
        float* detA = scratch.getWritePointer(0);
        float* detB = scratch.getWritePointer(1);
        float* gA = scratch.getWritePointer(2);
        float* gB = scratch.getWritePointer(3);
        const float* bIn = nullptr;

        // Detector inputs, copied before the audio is changed in place.
        if (keyed) {
            juce::FloatVectorOperations::copy(detA, main.getReadPointer(0, pos), n);
            if (mainCh == 2) {
                juce::FloatVectorOperations::add(detA, main.getReadPointer(1, pos), n);
                juce::FloatVectorOperations::multiply(detA, 0.5f, n);
            }
            if (sideCh > 0) {
                juce::FloatVectorOperations::copy(detB, side.getReadPointer(0, pos), n);
                if (sideCh == 2) {
                    juce::FloatVectorOperations::add(detB, side.getReadPointer(1, pos), n);
                    juce::FloatVectorOperations::multiply(detB, 0.5f, n);
                }
                bIn = detB;
            }
        } else {
            juce::FloatVectorOperations::copy(detA, main.getReadPointer(0, pos), n);
            if (mainCh == 2) {
                juce::FloatVectorOperations::copy(detB, main.getReadPointer(1, pos), n);
                bIn = detB;
            }
        }
        engine.process(detA, bIn, gA, gB, n);

        for (int c = 0; c < mainCh; ++c) {
            float* x = main.getWritePointer(c, pos);
            delay[c].process(x, n);
            // Stereo pair: left takes A's gain, right takes B's.  Sidechain: every channel is speaker A.
            juce::FloatVectorOperations::multiply(x, (!keyed && c == 1) ? gB : gA, n);
        }
        if (mixed) {
            float* l = main.getWritePointer(0, pos);
            float* r = main.getWritePointer(1, pos);
            juce::FloatVectorOperations::add(l, r, n);
            juce::FloatVectorOperations::copy(r, l, n);
        }
    }

    meterLevel[0].store(engine.levelADb(), std::memory_order_relaxed);
    meterLevel[1].store(engine.levelBDb(), std::memory_order_relaxed);
    meterGain[0].store(engine.gainA(), std::memory_order_relaxed);
    meterGain[1].store(engine.gainB(), std::memory_order_relaxed);
    meterTalking[0].store(engine.talkingA(), std::memory_order_relaxed);
    meterTalking[1].store(engine.talkingB(), std::memory_order_relaxed);
}

juce::AudioProcessorEditor* DvlceDuetProcessor::createEditor() { return new DvlceDuetEditor(*this); }

void DvlceDuetProcessor::getStateInformation(juce::MemoryBlock& dest) {
    if (auto xml = params.copyState().createXml()) copyXmlToBinary(*xml, dest);
}

void DvlceDuetProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(params.state.getType())) params.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DvlceDuetProcessor(); }
