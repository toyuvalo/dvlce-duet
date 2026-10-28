// Quantum header, adapted from the DVLCE UI kit. Copyright 2026 Yuval Olsha.
#pragma once
#include "DvlceLookAndFeel.h"
#include "Presets.h"

namespace dvlce::ui {
class QuantumWave : public juce::Component, private juce::Timer {
public:
    void setRunning(bool on) {
        if (on == isTimerRunning()) return;
        if (on) startTimerHz(30); else stopTimer();
        repaint();
    }
    void paint(juce::Graphics& g) override {
        Theme::Scope scope(*this);
        auto r = getLocalBounds().toFloat().reduced(1, 4);
        Theme::field(g, r);
        juce::Path wave;
        const float amplitude = (r.getHeight() * .5f - 4) * (.5f + level * .5f);
        for (int i = 0; i <= 60; ++i) {
            const float t = float(i) / 60.f;
            const float envelope = std::pow(juce::jmax(0.f, std::sin(juce::MathConstants<float>::pi * t)), 1.5f);
            const float tau = juce::MathConstants<float>::twoPi;
            const float value = .55f * std::sin(tau * (5*t + phase)) + .3f * std::sin(tau * (13*t - 1.7f*phase));
            const auto p = juce::Point<float>(r.getX() + 6 + t * (r.getWidth()-12), r.getCentreY()-amplitude*envelope*value);
            if (i == 0) wave.startNewSubPath(p); else wave.lineTo(p);
        }
        g.setColour(Theme::light());
        g.strokePath(wave, juce::PathStrokeType(1.4f));
    }
private:
    void timerCallback() override {
        if (!isShowing() && getTopLevelComponent()->isOnDesktop()) return;
        if (auto* p = dynamic_cast<ThemeProvider*>(&getLookAndFeel())) level += (p->themeActivity()-level)*.2f;
        phase += .006f + .03f*level;
        if (phase > 1000) phase -= 1000;
        repaint();
    }
    float phase = 0, level = 0;
};

class QuantumHeader : public juce::Component, private juce::Timer {
public:
    QuantumHeader(juce::AudioProcessor& p, juce::AudioProcessorValueTreeState& s,
                  DvlceLookAndFeel& l, juce::String product, juce::String id,
                  juce::String v, juce::String description)
        : bank(p,s,id,product), state(s), look(l), title(product.toUpperCase()),
          subtitle("v"+v+"   //   "+description) {
        for (auto* b : {&previous, &name, &next, &save, &motion}) addAndMakeVisible(b);
        addAndMakeVisible(wave);
        look.setAccent(Accent::Amber);
        motion.getProperties().set("dvlceLed", true);
        previous.setTooltip("Previous preset");
        next.setTooltip("Next preset");
        name.setTooltip("Factory presets by category; user presets");
        save.setTooltip("Save current settings as JSON in "+bank.directory().getFullPathName());
        previous.onClick = [this] { step(-1); };
        next.onClick = [this] { step(1); };
        name.onClick = [this] { showMenu(); };
        save.onClick = [this] { saveDialog(); };
        motion.onClick = [this] {
            const bool reduce = !look.choice().reduceMotion;
            state.state.setProperty("uiReduceMotion", reduce, nullptr);
            Theme::rememberChoice({ThemeId::Quantum, false, reduce});
            sync();
        };
        sync();
        startTimerHz(4);
    }
    ~QuantumHeader() override { stopTimer(); }
    void paint(juce::Graphics& g) override {
        Theme::Scope scope(*this);
        auto r = getLocalBounds().reduced(4,0);
        auto row = r.removeFromTop(44);
        g.setFont(Theme::type(23, true, true));
        g.setColour(Theme::colour(Theme::textDim));
        g.drawText("DVLCE //", row.removeFromLeft(102), juce::Justification::centredLeft);
        g.setColour(Theme::light());
        g.drawText(title, row.removeFromLeft(150), juce::Justification::centredLeft);
        g.setFont(Theme::font(12));
        g.setColour(Theme::colour(Theme::textDim));
        g.drawText(subtitle, r.removeFromTop(24), juce::Justification::centredLeft);
        g.setColour(Theme::colour(Theme::line));
        g.drawHorizontalLine(getHeight()-1, 0.f, float(getWidth()));
    }
    void resized() override {
        auto r = getLocalBounds().reduced(4,7).withHeight(30);
        motion.setBounds(r.removeFromRight(84));
        r.removeFromRight(12);
        save.setBounds(r.removeFromRight(54));
        r.removeFromRight(5);
        next.setBounds(r.removeFromRight(28));
        r.removeFromRight(5);
        name.setBounds(r.removeFromRight(220));
        r.removeFromRight(5);
        previous.setBounds(r.removeFromRight(28));
        r.removeFromLeft(266);
        r.reduce(12,0);
        wave.setVisible(r.getWidth() >= 100);
        wave.setBounds(r);
    }
    PresetBank bank;
private:
    void timerCallback() override { sync(); }
    void sync() {
        const bool reduce = static_cast<bool>(state.state.getProperty("uiReduceMotion", defaultReduceMotion));
        const ThemeChoice desired{ThemeId::Quantum,false,reduce};
        if (look.choice() != desired) look.setChoice(desired);
        motion.setToggleState(!reduce, juce::dontSendNotification);
        motion.setTooltip(reduce ? "Motion is off. Click to animate." : "Motion is on. Click to stop animation.");
        wave.setRunning(!reduce);
        const int selected = bank.selected();
        const auto label = selected < 0 ? juce::String("INIT / CUSTOM") :
            bank.entries[static_cast<size_t>(selected)].name + (bank.matches(selected) ? "" : " *");
        name.setButtonText(label + "  v");
    }
    void step(int delta) {
        const int n = static_cast<int>(bank.entries.size());
        if (!n) return;
        const int current = bank.selected();
        const int index = current < 0 ? (delta > 0 ? 0 : n-1) : (current+delta+n)%n;
        bank.apply(index);
        sync();
    }
    void showMenu() {
        bank.reload();
        juce::PopupMenu menu;
        juce::StringArray categories;
        for (const auto& e : bank.entries) categories.addIfNotAlreadyThere(e.category);
        for (const auto& category : categories) {
            juce::PopupMenu group;
            for (size_t i=0;i<bank.entries.size();++i)
                if (bank.entries[i].category == category)
                    group.addItem(static_cast<int>(i)+1, bank.entries[i].name, true, static_cast<int>(i)==bank.selected());
            menu.addSubMenu(category, group);
        }
        juce::Component::SafePointer<QuantumHeader> safe(this);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(name),
                          [safe](int result) { if (safe != nullptr && result > 0) { safe->bank.apply(result-1); safe->sync(); } });
    }
    void saveDialog() {
        auto* dialog = new juce::AlertWindow("Save preset", "Choose a name. Existing files are kept.", juce::MessageBoxIconType::NoIcon);
        dialog->addTextEditor("name", "My preset", "Name");
        dialog->addButton("SAVE", 1, juce::KeyPress(juce::KeyPress::returnKey));
        dialog->addButton("CANCEL", 0, juce::KeyPress(juce::KeyPress::escapeKey));
        juce::Component::SafePointer<QuantumHeader> safe(this);
        dialog->enterModalState(true, juce::ModalCallbackFunction::create([safe,dialog](int result) {
            if (safe == nullptr || result != 1) return;
            const auto saved = safe->bank.save(dialog->getTextEditorContents("name"));
            if (saved.failed())
                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Preset not saved", saved.getErrorMessage());
            safe->sync();
        }), true);
    }
    juce::AudioProcessorValueTreeState& state;
    DvlceLookAndFeel& look;
    juce::String title, subtitle;
    const bool defaultReduceMotion = Theme::defaultChoice().reduceMotion;
    QuantumWave wave;
    juce::TextButton previous{"<"}, name{"INIT / CUSTOM"}, next{">"}, save{"SAVE"}, motion{"MOTION"};
};
}
