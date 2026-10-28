#include <cmath>
#include "DvlceLookAndFeel.h"
namespace dvlce::ui {
DvlceLookAndFeel::DvlceLookAndFeel(ThemeChoice choice) : theme(choice) { applyColours(); }
void DvlceLookAndFeel::setChoice(ThemeChoice choice) {
    theme = choice;
    level = 0.f;
    applyColours();
}
void DvlceLookAndFeel::setAccent(Accent a) {
    if (a == accent) return;
    accent = a;
    applyColours();
}
float DvlceLookAndFeel::themeActivity() const {
    if (theme.reduceMotion) return 0.f;
    const double now = juce::Time::getMillisecondCounterHiRes();
    if (stamp > 0.0) level *= float(std::exp(-(now - stamp) / 380.0));
    stamp = now;
    return level;
}
void DvlceLookAndFeel::reportActivity(float level01) {
    if (theme.reduceMotion) return;
    const float decayed = themeActivity();
    level = juce::jmax(decayed, juce::jlimit(0.f, 1.f, level01));
    const int step = juce::roundToInt(level * 10.f);
    const double now = juce::Time::getMillisecondCounterHiRes();
    // Repaint only the backdrop, at most ~16 Hz and only when the level moved a step.
    if (step != lastStep && now - lastRepaint > 60.0) {
        lastStep = step;
        lastRepaint = now;
        if (backdrop != nullptr) backdrop->repaint();
    }
}
void DvlceLookAndFeel::applyColours() {
    Theme::Scope scope(static_cast<const ThemeProvider&>(*this));
    using namespace juce;
    using namespace Theme;
    for (auto id :
         std::initializer_list<int>{ResizableWindow::backgroundColourId, PopupMenu::backgroundColourId,
                                    TooltipWindow::backgroundColourId})
        setColour(id, colour(classic() ? panel : bg));
    for (auto id : std::initializer_list<int>{Label::textColourId, TextButton::textColourOffId,
                                              TextButton::textColourOnId, ToggleButton::textColourId,
                                              ComboBox::textColourId, PopupMenu::textColourId,
                                              TooltipWindow::textColourId, TextEditor::textColourId})
        setColour(id, colour(text));
    for (auto id : std::initializer_list<int>{ComboBox::backgroundColourId, TextEditor::backgroundColourId,
                                              TextButton::buttonColourId})
        setColour(id, colour(surface));
    for (auto id : std::initializer_list<int>{ComboBox::outlineColourId, TextEditor::outlineColourId,
                                              TooltipWindow::outlineColourId})
        setColour(id, colour(line));
    setColour(PopupMenu::highlightedBackgroundColourId, colour(surface));
    setColour(PopupMenu::highlightedTextColourId, colour(purpleHi));
    setColour(TextEditor::highlightColourId, colour(purple).withAlpha(.3f));
    setColour(TextEditor::focusedOutlineColourId, colour(purple));
    setColour(ComboBox::arrowColourId, colour(textDim));
    setColour(CaretComponent::caretColourId, colour(text));
    setColour(Slider::textBoxTextColourId, colour(text));
    setColour(Slider::textBoxBackgroundColourId, colour(surface));
    setColour(Slider::textBoxOutlineColourId, colour(line));
    setColour(Slider::textBoxHighlightColourId, colour(purple).withAlpha(.3f));
    setColour(Slider::backgroundColourId, colour(surface));
    setColour(Slider::trackColourId, colour(text));
    setColour(Slider::thumbColourId, colour(text));
    setColour(ToggleButton::tickColourId, colour(text));
    setColour(PopupMenu::headerTextColourId, colour(textDim));
    setColour(PopupMenu::highlightedBackgroundColourId, colour(surface));
}
void DvlceLookAndFeel::drawButtonBackground(juce::Graphics &g, juce::Button &b, const juce::Colour &,
                                            bool hover, bool) {
    Theme::Scope scope(static_cast<const ThemeProvider&>(*this));
    if (!Theme::classic()) {
        // An LED chip (the motion switch) shows its state with a lamp, not a lit body: it is a setting, not a control.
        const bool led = b.getProperties().contains("dvlceLed");
        Theme::chip(g, b.getLocalBounds().toFloat().reduced(1), b.getToggleState() && !led, hover, b.isEnabled());
        return;
    }
    auto r = b.getLocalBounds().toFloat().reduced(1);
    juce::Path p;
    p.addRoundedRectangle(r, 4);
    if (b.getToggleState())
        drawGlow(g, p, .4f);
    g.setColour(Theme::colour(Theme::surface));
    g.fillPath(p);
    g.setColour(Theme::colour(b.getToggleState() ? Theme::purple : (hover ? Theme::textDim : Theme::line))
                    .withAlpha(b.isEnabled() ? 1.f : .4f));
    g.strokePath(p, juce::PathStrokeType(1));
}
void DvlceLookAndFeel::drawToggleButton(juce::Graphics &g, juce::ToggleButton &b, bool hover, bool down) {
    Theme::Scope scope(static_cast<const ThemeProvider&>(*this));
    drawButtonBackground(g, b, {}, hover, down);
    Theme::label(g, b.getButtonText(), b.getLocalBounds().toFloat(),
                 b.getToggleState() ? Theme::text : Theme::textDim);
}
void DvlceLookAndFeel::drawLinearSlider(juce::Graphics &g, int x, int y, int w, int h, float pos, float,
                                        float, juce::Slider::SliderStyle, juce::Slider &s) {
    Theme::Scope scope(static_cast<const ThemeProvider&>(*this));
    const bool vertical = s.isVertical();
    if (!Theme::classic()) {
        const float cx = float(x) + float(w) * .5f, cy = float(y) + float(h) * .5f;
        const bool hot = s.isMouseButtonDown();
        const auto c = hot ? Theme::colour(Theme::purple) : Theme::ice();
        g.setColour(Theme::colour(Theme::line));
        g.drawLine(vertical ? juce::Line<float>{cx, float(y), cx, float(y + h)} : juce::Line<float>{float(x), cy, float(x + w), cy}, .8f);
        const auto thumb = vertical ? juce::Rectangle<float>(cx - 12, pos - 1, 24, 2) : juce::Rectangle<float>(pos - 1, cy - 12, 2, 24);
        Theme::bloomRect(g, thumb, c, hot ? .6f : .42f, 5.f);
        g.setColour(c);
        g.fillRect(thumb);
        return;
    }
    g.setColour(Theme::colour(Theme::surface));
    auto track = vertical ? juce::Rectangle<float>(float(x + w / 2 - 2), float(y), 4.f, float(h))
                          : juce::Rectangle<float>(float(x), float(y + h / 2 - 2), float(w), 4.f);
    g.fillRoundedRectangle(track, 2);
    juce::Path p;
    p.addRoundedRectangle(vertical ? juce::Rectangle<float>(track.getCentreX() - 12, pos - 2, 24, 4)
                                   : juce::Rectangle<float>(pos - 2, track.getCentreY() - 12, 4, 24),
                          2);
    if (s.isMouseButtonDown())
        drawGlow(g, p);
    g.setColour(Theme::colour(Theme::text));
    g.fillPath(p);
}
void DvlceLookAndFeel::drawRotarySlider(juce::Graphics &g, int x, int y, int w, int h, float pos, float start,
                                        float end, juce::Slider &s) {
    Theme::Scope scope(static_cast<const ThemeProvider&>(*this));
    auto r = juce::Rectangle<float>(float(x), float(y), float(w), float(h)).reduced(5);
    if (!Theme::classic()) { Theme::dial(g, r, pos, start, end, s.isMouseButtonDown()); return; }
    auto radius = juce::jmin(r.getWidth(), r.getHeight()) / 2;
    juce::Path ring, arc;
    ring.addCentredArc(r.getCentreX(), r.getCentreY(), radius, radius, 0, start, end, true);
    arc.addCentredArc(r.getCentreX(), r.getCentreY(), radius, radius, 0, start, start + pos * (end - start),
                      true);
    g.setColour(Theme::colour(Theme::line));
    g.strokePath(ring, juce::PathStrokeType(2));
    if (s.isMouseButtonDown()) {
        juce::Path glow;
        juce::PathStrokeType(2).createStrokedPath(glow, arc);
        drawGlow(g, glow);
    }
    g.setColour(Theme::colour(Theme::text));
    g.strokePath(arc, juce::PathStrokeType(2));
}
} // namespace dvlce::ui

namespace dvlce::ui {
void DvlceLookAndFeel::drawComboBox(juce::Graphics& g,int w,int h,bool down,int bx,int by,int bw,int bh,juce::ComboBox& box) {
    Theme::Scope scope(static_cast<const ThemeProvider&>(*this));
    if (Theme::classic()) { juce::LookAndFeel_V4::drawComboBox(g,w,h,down,bx,by,bw,bh,box); return; }
    Theme::chip(g, box.getLocalBounds().toFloat().reduced(.5f), box.hasKeyboardFocus(true), box.isMouseOver(), box.isEnabled());
    const float x = float(w) - 12, y = float(h) * .5f;
    juce::Path p; p.startNewSubPath(x - 3, y - 1.5f); p.lineTo(x, y + 1.5f); p.lineTo(x + 3, y - 1.5f);
    g.setColour(Theme::light()); g.strokePath(p, juce::PathStrokeType(1));
}
}

namespace dvlce::ui {
void DvlceLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool hover, bool down) {
    Theme::Scope scope(static_cast<const ThemeProvider&>(*this));
    if (Theme::classic()) { juce::LookAndFeel_V4::drawButtonText(g, b, hover, down); return; }
    const bool on = b.getToggleState();
    auto r = b.getLocalBounds().toFloat().reduced(2, 0);
    if (b.getProperties().contains("dvlceLed")) {
        // Lamp at the left: lit in the accent with a little bloom when on, a hollow ring when off.
        const float d = 6.f, cx = r.getX() + 9.f, cy = r.getCentreY();
        if (on) {
            Theme::bloomRect(g, {cx - 1.f, cy - 1.f, 2.f, 2.f}, Theme::light(), .7f, 4.f);
            g.setColour(Theme::light());
            g.fillEllipse(cx - d * .5f, cy - d * .5f, d, d);
        } else {
            g.setColour(Theme::colour(Theme::textDim).withAlpha(.7f));
            g.drawEllipse(cx - d * .5f + .5f, cy - d * .5f + .5f, d - 1.f, d - 1.f, 1.f);
        }
        r.removeFromLeft(14.f);
        Theme::label(g, Theme::isQuantum() ? b.getButtonText().toUpperCase() : b.getButtonText(), r,
                     on ? Theme::text : Theme::textDim, b.getHeight() >= 20 ? 11.f : 10.f);
        return;
    }
    const auto onRole = Theme::isQuantum() ? Theme::design().ice : Theme::purpleHi;
    // A disabled chip keeps a readable, clearly muted label (the line colour vanished on the Quantum ground).
    const auto offRole = b.isEnabled() ? Theme::textDim : Theme::colour(Theme::textDim).withAlpha(.42f).getARGB();
    // Quantum chips are all upper case: choice names such as "Dark" / "Electro" read as one set with the rest.
    Theme::label(g, Theme::isQuantum() ? b.getButtonText().toUpperCase() : b.getButtonText(), r, on ? onRole : offRole,
                 b.getButtonText().length() == 1 && b.getButtonText()[0] > 127 ? 16.f : (b.getHeight() >= 20 ? 11.f : 10.f));
}
void DvlceLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int w, int h) {
    Theme::Scope scope(static_cast<const ThemeProvider&>(*this));
    if (Theme::classic()) { juce::LookAndFeel_V4::drawPopupMenuBackground(g, w, h); return; }
    g.fillAll(Theme::colour(Theme::bg));
    Theme::glass(g, juce::Rectangle<float>(0, 0, float(w), float(h)), 1.f, false);
}
}
