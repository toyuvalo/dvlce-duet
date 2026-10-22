// Dvlce Duet - editor.
// Copyright 2026 Yuval Olsha.
#include "PluginEditor.h"

namespace {
const juce::Colour kBg{0xff16181d}, kPanel{0xff20232a}, kText{0xffe6e6e6}, kDim{0xff9aa0a6}, kAccent{0xffd9a441},
    kLevel{0xff5fb3a1}, kDuck{0xffd9534f};
constexpr float kMeterFloorDb = -80.0f;
}  // namespace

DvlceDuetEditor::DvlceDuetEditor(DvlceDuetProcessor& p) : AudioProcessorEditor(&p), proc(p) {
    look.setColourScheme(juce::LookAndFeel_V4::getMidnightColourScheme());
    look.setColour(juce::Slider::rotarySliderFillColourId, kAccent);
    look.setColour(juce::Slider::thumbColourId, kAccent);
    look.setColour(juce::ToggleButton::tickColourId, kAccent);
    setLookAndFeel(&look);

    setupChoice(mode, "mode", "Mode",
                "Stereo pair: one stereo track, left = speaker A, right = speaker B (any host).\n"
                "Sidechain: this track = A, the sidechain input = B. Use one instance per speaker, each keyed from the other.");
    setupChoice(output, "output", "Output",
                "Stereo pair only. Separate keeps A left and B right. Mixed puts both gated voices in the centre.");
    setupChoice(overlap, "overlap", "Overlap",
                "Both talk at once: Both open lets the crosstalk through; Louder wins ducks the quieter one.");
    idleGate.setColour(juce::ToggleButton::textColourId, kText);
    idleGate.setTooltip("Nobody talking: duck both speakers too (an ordinary gate on top of the ducking).");
    addAndMakeVisible(idleGate);
    idleAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.params, "idle_gate", idleGate);

    setupKnob(thrA, "thr_a", "Threshold A", "Speaker A counts as talking above this detector level.");
    setupKnob(thrB, "thr_b", "Threshold B", "Speaker B counts as talking above this detector level.");
    setupKnob(depth, "depth", "Depth", "How far the silent speaker drops while the other talks.");
    setupKnob(bleed, "bleed", "Bleed reject",
              "A speaker this many dB under the other (each against its threshold) is mic bleed, not speech.");
    setupKnob(hysteresis, "hysteresis", "Hysteresis", "Talking ends only this far below the threshold. Stops chatter.");
    setupKnob(open, "open", "Open", "Time to open across the full depth.");
    setupKnob(hold, "hold", "Hold", "Talking state lasts this long after the level falls (pauses between words).");
    setupKnob(close, "close", "Close", "Time to duck across the full depth.");
    setupKnob(lookahead, "lookahead", "Lookahead", "Delays the audio so a voice opens before its first word. Adds latency.");
    setupKnob(hp, "hp", "Detector HP", "High-pass on the detector only: rumble and plosives do not trigger it.");

    status.setColour(juce::Label::textColourId, kDim);
    status.setFont(juce::FontOptions(13.0f));
    addAndMakeVisible(status);

    setResizable(true, true);
    setResizeLimits(760, 440, 1600, 900);
    setSize(860, 470);
    startTimerHz(30);
    timerCallback();
}

DvlceDuetEditor::~DvlceDuetEditor() { setLookAndFeel(nullptr); }

void DvlceDuetEditor::setupKnob(Knob& k, const char* id, const char* text, const char* tip) {
    k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 20);
    k.slider.setTooltip(tip);
    k.label.setText(text, juce::dontSendNotification);
    k.label.setJustificationType(juce::Justification::centred);
    k.label.setColour(juce::Label::textColourId, kText);
    addAndMakeVisible(k.slider);
    addAndMakeVisible(k.label);
    k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.params, id, k.slider);
    const juce::String unit = proc.params.getParameter(id)->getLabel();
    if (unit.isNotEmpty()) k.slider.setTextValueSuffix(" " + unit);
}

void DvlceDuetEditor::setupChoice(Choice& c, const char* id, const char* text, const char* tip) {
    c.box.addItemList(proc.params.getParameter(id)->getAllValueStrings(), 1);
    c.box.setTooltip(tip);
    c.label.setText(text, juce::dontSendNotification);
    c.label.setColour(juce::Label::textColourId, kText);
    addAndMakeVisible(c.box);
    addAndMakeVisible(c.label);
    c.attach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(proc.params, id, c.box);
}

void DvlceDuetEditor::timerCallback() {
    const bool keyed = mode.box.getSelectedItemIndex() == 1;
    output.box.setEnabled(!keyed);
    output.label.setAlpha(keyed ? 0.4f : 1.0f);

    juce::String s;
    if (keyed)
        s = proc.sidechainConnected.load() ? "Sidechain: this track = A, sidechain = B. Route the other speaker's track to the sidechain."
                                           : "Sidechain mode, but the host gives this plugin no sidechain input. Enable it, or use Stereo pair.";
    else
        s = proc.stereoInput.load() ? "Stereo pair: left = A, right = B. Mono tracks? Join them into one stereo track first."
                                    : "Stereo pair needs a stereo track (left = A, right = B). This track is mono, so B is silent.";
    status.setText(s, juce::dontSendNotification);
    repaint(meterArea);
}

void DvlceDuetEditor::paintMeter(juce::Graphics& g, juce::Rectangle<int> r, int speaker, const juce::String& name) const {
    const bool talking = proc.meterTalking[speaker].load();
    const float level = proc.meterLevel[speaker].load();
    const float gainDb = juce::Decibels::gainToDecibels(proc.meterGain[speaker].load(), -duet::kMuteDb);
    const float thr = proc.params.getRawParameterValue(speaker == 0 ? "thr_a" : "thr_b")->load();

    g.setColour(kText);
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText(name, r.removeFromTop(22), juce::Justification::centred);
    auto lamp = r.removeFromBottom(22);
    g.setColour(talking ? kAccent : kPanel.brighter(0.15f));
    g.fillRoundedRectangle(lamp.reduced(4, 3).toFloat(), 4.0f);
    g.setColour(talking ? kBg : kDim);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText(talking ? "TALKING" : "quiet", lamp, juce::Justification::centred);
    r.removeFromBottom(6);

    const auto toY = [&](float db, juce::Rectangle<int> bar) {
        const float t = juce::jlimit(0.0f, 1.0f, (db - kMeterFloorDb) / -kMeterFloorDb);
        return bar.getBottom() - t * static_cast<float>(bar.getHeight());
    };
    auto levelBar = r.removeFromLeft(r.getWidth() / 2).reduced(4, 0);
    auto gainBar = r.reduced(4, 0);
    g.setColour(kBg);
    g.fillRect(levelBar);
    g.fillRect(gainBar);
    g.setColour(kLevel);
    const float ly = toY(level, levelBar);
    g.fillRect(juce::Rectangle<float>(float(levelBar.getX()), ly, float(levelBar.getWidth()), float(levelBar.getBottom()) - ly));
    g.setColour(kAccent);
    const float ty = toY(thr, levelBar);
    g.fillRect(juce::Rectangle<float>(float(levelBar.getX()) - 3.0f, ty - 1.0f, float(levelBar.getWidth()) + 6.0f, 2.0f));
    // gain reduction hangs down from the top: 0 dB = empty, full depth = the whole bar
    g.setColour(kDuck);
    const float gy = toY(kMeterFloorDb - gainDb, gainBar);  // -30 dB of gain fills 30 dB of scale from the top
    g.fillRect(juce::Rectangle<float>(float(gainBar.getX()), float(gainBar.getY()), float(gainBar.getWidth()),
                                      float(gainBar.getBottom()) - gy));
    g.setColour(kDim);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("level", levelBar.withY(levelBar.getBottom() - 14).withHeight(14), juce::Justification::centred);
    g.drawText("duck", gainBar.withY(gainBar.getBottom() - 14).withHeight(14), juce::Justification::centred);
}

void DvlceDuetEditor::paint(juce::Graphics& g) {
    g.fillAll(kBg);
    auto r = getLocalBounds().reduced(12);
    auto head = r.removeFromTop(36);
    g.setColour(kText);
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.drawText("Dvlce Duet", head.removeFromLeft(150), juce::Justification::centredLeft);
    g.setColour(kDim);
    g.setFont(juce::FontOptions(13.0f));
    g.drawText("v" DVLCE_DUET_VERSION "   mutual ducking gate: when one speaker talks, the other is ducked", head,
               juce::Justification::centredLeft);
    g.setColour(kPanel);
    g.fillRoundedRectangle(meterArea.toFloat().expanded(6.0f), 6.0f);
    g.fillRoundedRectangle(panelArea.toFloat(), 6.0f);

    const bool keyed = mode.box.getSelectedItemIndex() == 1;
    auto m = meterArea;
    const int w = m.getWidth() / 2;
    paintMeter(g, m.removeFromLeft(w).reduced(4, 0), 0, keyed ? "A (this track)" : "A (left)");
    paintMeter(g, m.reduced(4, 0), 1, keyed ? "B (sidechain)" : "B (right)");
}

void DvlceDuetEditor::resized() {
    auto r = getLocalBounds().reduced(12);
    r.removeFromTop(44);
    status.setBounds(r.removeFromBottom(22));
    r.removeFromBottom(6);
    meterArea = r.removeFromLeft(220).reduced(6);
    r.removeFromLeft(12);
    panelArea = r;
    auto panel = r.reduced(10);

    auto row = panel.removeFromTop(48);
    const int cw = row.getWidth() / 4;
    for (auto* c : {&mode, &output, &overlap}) {
        auto cell = row.removeFromLeft(cw).reduced(4, 0);
        c->label.setBounds(cell.removeFromTop(20));
        c->box.setBounds(cell.removeFromTop(24));
    }
    idleGate.setBounds(row.reduced(4, 0).withTrimmedTop(20).withHeight(24));
    panel.removeFromTop(8);

    const int rowH = panel.getHeight() / 2;
    Knob* rows[2][5] = {{&thrA, &thrB, &depth, &bleed, &hysteresis}, {&open, &hold, &close, &lookahead, &hp}};
    for (auto& knobRow : rows) {
        auto line = panel.removeFromTop(rowH);
        const int kw = line.getWidth() / 5;
        for (auto* k : knobRow) {
            auto cell = line.removeFromLeft(kw).reduced(4);
            k->label.setBounds(cell.removeFromTop(18));
            k->slider.setBounds(cell);
        }
    }
}
