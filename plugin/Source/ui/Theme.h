#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
namespace dvlce::ui {
// Five looks, each in dark and light. Quantum is the default. The choice is stored in the plugin
// state (properties uiLook / uiLight / uiReduceMotion on the APVTS tree) and remembered as the
// user's last choice for new instances (UserSettings). It never touches the audio thread.
enum class ThemeId { Classic, Spectrum, Radiant, Vault, Quantum };
// Quantum colours each editor with one accent family; the Header picks it from the product name.
enum class Accent { Cyan, Magenta, Amber, Violet, Blue, Green };
struct ThemeChoice {
    ThemeId id = ThemeId::Quantum;
    bool light = false;
    bool reduceMotion = false;
    bool operator==(const ThemeChoice& o) const { return id == o.id && light == o.light && reduceMotion == o.reduceMotion; }
    bool operator!=(const ThemeChoice& o) const { return !(*this == o); }
};
// Per-LookAndFeel selection; no parameters, state-tree properties or audio work.
struct ThemeProvider {
    virtual ~ThemeProvider() = default;
    virtual ThemeChoice themeChoice() const = 0;
    // 0..1 smoothed audio activity of this editor (message thread). Zero when motion is reduced.
    virtual float themeActivity() const { return 0.f; }
    virtual Accent themeAccent() const { return Accent::Cyan; }
};
namespace Theme {
inline thread_local ThemeId current = ThemeId::Quantum;
inline thread_local Accent accent = Accent::Cyan;
inline thread_local bool lightMode = false;
inline thread_local bool reduceMotion = false;
inline thread_local float activity = 0.f;
// The choice a new editor starts with: the user's last choice (UserSettings), or the override below.
ThemeChoice defaultChoice();
void setDefaultOverride(ThemeChoice); // Snapshot / tests: ignore UserSettings
void rememberChoice(ThemeChoice);     // writes the user's last choice (message thread)
struct Scope {
    ThemeId previous = current;
    bool previousLight = lightMode, previousReduce = reduceMotion;
    float previousActivity = activity;
    Accent previousAccent = accent;
    explicit Scope(ThemeChoice c) { current = c.id; lightMode = c.light; reduceMotion = c.reduceMotion; }
    explicit Scope(ThemeId id) { current = id; }
    explicit Scope(const ThemeProvider& provider) { apply(provider); }
    explicit Scope(const juce::Component& c) {
        if (auto* provider = dynamic_cast<ThemeProvider*>(&c.getLookAndFeel())) apply(*provider);
    }
    ~Scope() { current = previous; lightMode = previousLight; reduceMotion = previousReduce; activity = previousActivity; accent = previousAccent; }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
private:
    void apply(const ThemeProvider& provider) {
        const auto choice = provider.themeChoice();
        current = choice.id; lightMode = choice.light; reduceMotion = choice.reduceMotion;
        activity = provider.themeActivity();
        accent = provider.themeAccent();
    }
};
inline bool classic() { return current == ThemeId::Classic; }
inline bool isSpectrum() { return current == ThemeId::Spectrum; }
inline bool isRadiant() { return current == ThemeId::Radiant; }
inline bool isVault() { return current == ThemeId::Vault; }
inline bool isQuantum() { return current == ThemeId::Quantum; }
// The accent family for a product title such as "DVLCE  FET  v0.1.5".
Accent accentFor(const juce::String& title);
inline bool isLight() { return lightMode; }
const char* name(ThemeId);
const char* displayName(ThemeId);
bool parse(const juce::String&, ThemeId&);
struct Design {
    juce::uint32 background, panel, surface, line, dim, text, selected, hot, light, ice;
    float radius, valueSize, titleSize;
};
const Design& design();
std::uint64_t cacheBuildCount();
juce::Colour light(); // cyan light: values, lit segments, curves
juce::Colour ice();   // hot core of a light
juce::Colour body();  // translucent panel body: near-black (dark) or white (light)
// Fonts. Typefaces are created once from BinaryData.
juce::Font type(float size, bool bold, bool title = false);
juce::Font numerals(float size);
// Radial segmented ring: the signature element of every knob and every round meter.
struct DialOptions {
    bool active = false;      // dragging or selected: purple light
    bool pointer = true;      // thin pointer line from the inner ring to the value
    float origin = 0.f;       // 0..1: where the lit arc starts (0.5 for bipolar)
    bool reduction = false;   // gain reduction meter: lit segments in purple
    float peak = -1.f;        // 0..1 peak-hold tick, or <0 for none
    bool constellation = true;
    int bars = 0;             // 0 = chosen from the radius
    float glow = 1.f;         // bloom multiplier (a gain-reduction meter passes its depth)
};
void dial(juce::Graphics&, juce::Rectangle<float>, float value, float start, float end, const DialOptions&);
inline void dial(juce::Graphics& g, juce::Rectangle<float> b, float v, float s, float e, bool active,
                 bool pointer = true) {
    DialOptions o; o.active = active; o.pointer = pointer; dial(g, b, v, s, e, o);
}
// Soft light under a lit rectangle / line. Cached, pre-tinted sprites; no blur at paint time.
void bloomRect(juce::Graphics&, juce::Rectangle<float>, juce::Colour, float strength = 1.f, float spread = 6.f);
// Hairline curve with a bloom under it (cheap path strokes; use inside cached images or small paths).
void plotCurve(juce::Graphics&, const juce::Path&, juce::Colour, float width = 1.4f, float bloom = 1.f);
// Panel in the current look (dark glass, open corners, or chamfered plate).
void glass(juce::Graphics&, juce::Rectangle<float>, float strength = 1.f, bool corners = true);
void valueReadout(juce::Graphics&, const juce::String&, juce::Rectangle<float>);
void field(juce::Graphics&, juce::Rectangle<float>, bool active = false);
void chip(juce::Graphics&, juce::Rectangle<float>, bool on, bool hover, bool enabled = true);
void panelShape(juce::Graphics&, juce::Rectangle<float>);
// Section label in the current look, optionally with a rule to the panel edge.
void sectionLabel(juce::Graphics&, const juce::String&, juce::Rectangle<float>, bool rule = true);
// Small footer caption with a glyph.
void caption(juce::Graphics&, const juce::String&, juce::Rectangle<float>, bool alignRight = false);
// Bloom strength multiplier for the current look and mode.
float bloomScale();
// Apply a theme change to labels, text editors and scrollbars that were configured with the previous look.
void retheme(juce::Component& root, ThemeChoice from, ThemeChoice to);
// The cached editor background. One image per editor size, theme, mode and panel set. Spectrum and
// Radiant also bake a glow layer that breathes with the editor's audio activity; a backdrop component
// below the canvas paints both layers, so a level change repaints only the backdrop.
// Renders a small widget once per state and blits it afterwards (value boxes, chips, dials). The image is made at the
// physical pixel scale of the target context; at a fractional scale it paints directly rather than blur.
class PaintCache {
public:
    // key: everything that changes the picture (text, state, value step). The look, mode and size are added here.
    void draw(juce::Graphics&, juce::Rectangle<int> bounds, std::uint64_t key, const std::function<void(juce::Graphics&)>& painter,
              bool opaque = false);
private:
    juce::Image image;
    std::uint64_t cachedKey = 0;
    bool valid = false;
};
std::uint64_t hashText(const juce::String&);
inline std::uint64_t mix(std::uint64_t seed, std::uint64_t v) { return (seed ^ (v + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2))) * 0xff51afd7ed558ccdull; }
// Content that never changes while the editor is open (section labels, scale numbers, captions) is painted by the
// canvas once into the cached background image, instead of every frame. staticKey() names the variant.
struct StaticPainter {
    virtual ~StaticPainter() = default;
    virtual void paintStatic(juce::Graphics&) = 0;
    virtual int staticKey() const { return 0; }
};
class BackdropLayer;
class Background {
public:
    Background();
    ~Background();
    void draw(juce::Graphics&, const juce::Component&, std::initializer_list<juce::Rectangle<float>> panels = {},
              StaticPainter* painter = nullptr);
private:
    friend class BackdropLayer;
    class Impl;
    std::unique_ptr<Impl> impl;
};
constexpr juce::uint32 bg = 0xff000000, panel = 0xff0b0b0d, surface = 0xff151518, line = 0xff2c2c31,
                       textDim = 0xff8c8c94, text = 0xffffffff, purple = 0xffa855f7, purpleHi = 0xffd8b4fe;
constexpr float grid = 8, panelRadius = 6, controlRadius = 4, labelSize = 11, valueSize = 13;
juce::Colour paletteColour(juce::uint32 classicConstant);
inline juce::Colour colour(juce::uint32 c) {
    if (!classic() || lightMode) return paletteColour(c);
    return juce::Colour(c);
}
inline juce::Font font(float size = valueSize, bool bold = false) {
    if (!classic()) return type(size, bold);
    return juce::Font(juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain)
                          .withFeatureEnabled(juce::FontFeatureTag{"tnum"}));
}
// Upper-case instrument label. Non-classic looks track section-size labels wide and shrink text to fit.
void label(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r,
           juce::uint32 c = textDim, float size = labelSize);
} // namespace Theme
// Draw beneath the foreground shape; all radii are in logical editor pixels.
void drawGlow(juce::Graphics& g, const juce::Path& path, float intensity = 1);
} // namespace dvlce::ui
