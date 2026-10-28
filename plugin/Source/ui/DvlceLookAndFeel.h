#pragma once
#include "Theme.h"
namespace dvlce::ui {
class DvlceLookAndFeel : public juce::LookAndFeel_V4, public ThemeProvider {
  public:
    explicit DvlceLookAndFeel(ThemeChoice choice = Theme::defaultChoice());
    // Code selection of one look (Classic included), e.g. DvlceLookAndFeel(ThemeId::Classic).
    explicit DvlceLookAndFeel(ThemeId id) : DvlceLookAndFeel(ThemeChoice{id, false, false}) {}
    ThemeChoice themeChoice() const override { return theme; }
    ThemeChoice choice() const { return theme; }
    ThemeId themeId() const { return theme.id; }
    float themeActivity() const override;
    Accent themeAccent() const override { return accent; }
    // Message thread: switch look / mode / motion. The caller re-themes widgets and repaints (Header does).
    void setChoice(ThemeChoice);
    // Message thread: the Quantum accent family of this editor (the Header sets it from the product name).
    void setAccent(Accent);
    // Message thread: a meter or display reports its normalised level; drives the breathing backdrop.
    void reportActivity(float level01);
    void setBackdrop(juce::Component* c) { backdrop = c; }
    ThemeChoice theme;
    void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override { Theme::Scope scope(static_cast<const ThemeProvider&>(*this)); return Theme::font(); }
    void drawButtonBackground(juce::Graphics &, juce::Button &, const juce::Colour &, bool, bool) override;
    void drawToggleButton(juce::Graphics &, juce::ToggleButton &, bool, bool) override;
    void drawLinearSlider(juce::Graphics &, int, int, int, int, float, float, float,
                          juce::Slider::SliderStyle, juce::Slider &) override;
    void drawRotarySlider(juce::Graphics &, int, int, int, int, float, float, float, juce::Slider &) override;
    void drawButtonText(juce::Graphics &, juce::TextButton &, bool, bool) override;
    void drawPopupMenuBackground(juce::Graphics &, int, int) override;
    juce::Font getTextButtonFont(juce::TextButton &, int) override { Theme::Scope scope(static_cast<const ThemeProvider&>(*this)); return Theme::font(11); }
    juce::Font getLabelFont(juce::Label &) override { Theme::Scope scope(static_cast<const ThemeProvider&>(*this)); return Theme::font(); }
    juce::Font getComboBoxFont(juce::ComboBox &) override { Theme::Scope scope(static_cast<const ThemeProvider&>(*this)); return Theme::font(); }

  private:
    void applyColours();
    juce::Component::SafePointer<juce::Component> backdrop;
    Accent accent = Accent::Cyan;
    mutable float level = 0.f;
    mutable double stamp = 0.0;
    int lastStep = 0;
    double lastRepaint = 0.0;
};
} // namespace dvlce::ui
