#include "Theme.h"
#include "DvlceLookAndFeel.h"
#include <DvlceUiFonts.h>
#include <cmath>
#include <array>
#include <map>
#include <optional>
#include <unordered_map>
namespace dvlce::ui {
namespace Theme {
static thread_local std::uint64_t backgroundBuilds = 0;
std::uint64_t cacheBuildCount() { return backgroundBuilds; }
const char* name(ThemeId id) {
    switch (id) {
        case ThemeId::Classic: return "classic";
        case ThemeId::Radiant: return "radiant";
        case ThemeId::Vault: return "vault";
        case ThemeId::Quantum: return "quantum";
        case ThemeId::Spectrum: break;
    }
    return "spectrum";
}
const char* displayName(ThemeId id) {
    switch (id) {
        case ThemeId::Classic: return "CLASSIC";
        case ThemeId::Radiant: return "RADIANT";
        case ThemeId::Vault: return "VAULT";
        case ThemeId::Quantum: return "QUANTUM";
        case ThemeId::Spectrum: break;
    }
    return "SPECTRUM";
}
bool parse(const juce::String& s, ThemeId& id) {
    for (auto candidate : {ThemeId::Classic, ThemeId::Spectrum, ThemeId::Radiant, ThemeId::Vault, ThemeId::Quantum})
        if (s.equalsIgnoreCase(name(candidate))) { id = candidate; return true; }
    return false;
}
// ---- palettes: [theme][0 = dark, 1 = light] -------------------------------------------------
Accent accentFor(const juce::String& title) {
    const auto t = title.toUpperCase();
    struct Rule { const char* word; Accent accent; };
    static const Rule rules[]{{"DROMEDARY", Accent::Magenta}, {"HARMONICS", Accent::Magenta}, {"WIENER", Accent::Magenta},
                              {"SUNSHINE", Accent::Amber},    {"FET", Accent::Amber},          {"OPTO", Accent::Amber},
                              {"VCA", Accent::Amber},          {"REVERB", Accent::Violet},      {"DELAY", Accent::Violet},
                              {"AIR", Accent::Violet},         {"LIMIT", Accent::Blue},         {"MULTIBAND", Accent::Blue},
                              {"RIDER", Accent::Blue},         {"AUTO DYN", Accent::Blue},      {"AUTODYN", Accent::Blue},
                              {"VOX", Accent::Green},          {"BREATH", Accent::Green},       {"TRANSIENT", Accent::Green}};
    for (const auto& r : rules)
        if (t.contains(r.word)) return r.accent;
    return Accent::Cyan;
}
namespace {
// Quantum: one palette per accent family and mode. The accent fills the light / ice roles, its partner the
// selected / hot roles (dragging, gain reduction, hot meter segments).
const Design& quantumDesign(Accent a, bool lightPalette) {
    static const auto table = [] {
        //                         light       ice         selected    hot
        const juce::uint32 accents[6][4]{{0xff38e0e8, 0xffb8fbff, 0xffff9a3d, 0xffffc58a},
                                         {0xffff52d9, 0xffffc2f2, 0xffff9a3d, 0xffffc58a},
                                         {0xffffb21f, 0xffffe3a0, 0xffff6a2b, 0xffff9e6e},
                                         {0xffa98bff, 0xffe2d8ff, 0xff38e0e8, 0xffb8fbff},
                                         {0xff4aa3ff, 0xffc6e2ff, 0xff38e0e8, 0xffb8fbff},
                                         {0xff3df2a6, 0xffc2ffe6, 0xffff9a3d, 0xffffc58a}};
        std::array<std::array<Design, 2>, 6> t{};
        for (size_t i = 0; i < 6; ++i) {
            const auto* c = accents[i];
            t[i][0] = {0xff10151b, 0xff172028, 0xff1c242c, 0xff2c3742, 0xffa3b4c2, 0xffe8f0f5, c[2], c[3], c[0], c[1], 6, 14, 19};
            auto deep = [](juce::uint32 v, float k) { return juce::Colour(v).interpolatedWith(juce::Colours::black, k).getARGB(); };
            t[i][1] = {0xffe9eef3, 0xfff6f8fb, 0xffe1e7ee, 0xffb9c5d1, 0xff3b4b59, 0xff0d1620, deep(c[2], .25f), deep(c[2], .45f),
                       deep(c[0], .38f), deep(c[0], .6f), 6, 14, 19};
        }
        return t;
    }();
    return table[size_t(a)][lightPalette ? 1 : 0];
}
} // namespace
const Design& designFor(ThemeId id, bool lightPalette) {
    if (id == ThemeId::Quantum) return quantumDesign(accent, lightPalette);
    //  bg          panel       surface     line        dim         text        purple      purpleHi    light       ice
    static const Design designs[4][2] {
        {{bg, panel, surface, line, textDim, text, purple, purpleHi, text, text, 4, 13, 13},
         {0xfff2f2f5, 0xffffffff, 0xffe8e8ec, 0xffc9c9d1, 0xff6a6a74, 0xff0a0a0c, 0xff7c3aed, 0xff5b21b6, 0xff0a0a0c, 0xff0a0a0c, 4, 13, 13}},
        {{0xff04060c, 0xff0a111a, 0xff0d1620, 0xff25364a, 0xff8ea3b9, 0xffeaf7ff, 0xff9d6bff, 0xffd6c3ff, 0xff3fd9ee, 0xffcdf6ff, 3, 13, 22},
         {0xffe8eef6, 0xfff5f8fc, 0xffdfe7f1, 0xffb6c5d8, 0xff3e5269, 0xff0a1424, 0xff7c3aed, 0xff5b21b6, 0xff0a8fa8, 0xff0b5a73, 3, 13, 22}},
        {{0xff03080d, 0x00000000, 0xff08121a, 0xff21353e, 0xff93aab7, 0xffe4f7ff, 0xffa878fa, 0xffdcf7ff, 0xff8ceaff, 0xffdcf7ff, 1, 13, 17},
         {0xffeaf1f5, 0x00f3f8fb, 0xffdde8ee, 0xffb4c8d2, 0xff3a5260, 0xff07202b, 0xff7c3aed, 0xff5b21b6, 0xff0c91ab, 0xff0a5266, 1, 13, 17}},
        {{0xff07090c, 0xff0d1116, 0xff11171e, 0xff2c343e, 0xff9da9b5, 0xffedf5ff, 0xffd8eaff, 0xfff3f9ff, 0xffd8eaff, 0xfff3f9ff, 0, 16, 17},
         {0xffedeff2, 0xfff7f8fa, 0xffe2e5ea, 0xffbcc3cd, 0xff46515e, 0xff0d131b, 0xff7c3aed, 0xff5b21b6, 0xff33465c, 0xff0d131b, 0, 16, 17}}};
    return designs[static_cast<int>(id)][lightPalette ? 1 : 0];
}
const Design& design() { return designFor(current, lightMode); }
juce::Colour paletteColour(juce::uint32 c) {
    const auto& d = design();
    if (c == bg) c = d.background;
    else if (c == panel) c = d.panel;
    else if (c == surface) c = d.surface;
    else if (c == line) c = d.line;
    else if (c == textDim) c = d.dim;
    else if (c == text) c = d.text;
    else if (c == purple) c = d.selected;
    else if (c == purpleHi) c = d.hot;
    return juce::Colour(c);
}
juce::Colour light() { return juce::Colour(design().light); }
juce::Colour ice() { return juce::Colour(design().ice); }
juce::Colour body() {
    if (isQuantum()) return lightMode ? juce::Colour(0xfff6f8fb) : juce::Colour(0xff121820);
    return lightMode ? juce::Colour(0xfff6f9fc) : juce::Colour(0xff02050a);
}
float bloomScale() {
    const float look = current == ThemeId::Spectrum || current == ThemeId::Quantum ? 1.f : current == ThemeId::Radiant ? .85f
                       : current == ThemeId::Vault ? .4f : 0.f;
    return look * (lightMode ? .6f : 1.f);
}
namespace {
// ---- user settings: the last choice, remembered for new instances (message thread only) ----
std::optional<ThemeChoice>& overrideChoice() { static std::optional<ThemeChoice> o; return o; }
juce::PropertiesFile::Options settingsOptions() {
    juce::PropertiesFile::Options o;
    o.applicationName = "DvlceUi";
    o.filenameSuffix = ".settings";
    o.folderName = "Dvlce";
    o.osxLibrarySubFolder = "Application Support";
    o.commonToAllUsers = false;
    o.storageFormat = juce::PropertiesFile::storeAsXML;
    o.millisecondsBeforeSaving = -1;
    return o;
}
} // namespace
void setDefaultOverride(ThemeChoice c) { overrideChoice() = c; }
ThemeChoice defaultChoice() {
    if (overrideChoice()) return *overrideChoice();
    ThemeChoice c;
    const auto opt = settingsOptions();
    if (opt.getDefaultFile().existsAsFile()) {
        juce::PropertiesFile file(opt);
        ThemeId id;
        // "look" replaced "theme" with the Quantum look (2026-10): a choice saved before it is ignored once.
        if (parse(file.getValue("look", "quantum"), id)) c.id = id;
        if (file.containsKey("look")) {
            c.light = file.getBoolValue("light", false);
            c.reduceMotion = file.getBoolValue("reduceMotion", false);
        }
    }
    return c;
}
void rememberChoice(ThemeChoice c) {
    if (overrideChoice()) return;
    juce::PropertiesFile file(settingsOptions());
    file.setValue("look", name(c.id));
    file.setValue("light", c.light);
    file.setValue("reduceMotion", c.reduceMotion);
    file.saveIfNeeded();
}
namespace {
struct Faces {
    juce::Typeface::Ptr anton = juce::Typeface::createSystemTypefaceFor(DvlceUiFonts::RajdhaniSemiBold_ttf, DvlceUiFonts::RajdhaniSemiBold_ttfSize);
    juce::Typeface::Ptr grotesk = juce::Typeface::createSystemTypefaceFor(DvlceUiFonts::BarlowSemiCondensedMedium_ttf, DvlceUiFonts::BarlowSemiCondensedMedium_ttfSize);
    juce::Typeface::Ptr mono = juce::Typeface::createSystemTypefaceFor(DvlceUiFonts::RajdhaniMedium_ttf, DvlceUiFonts::RajdhaniMedium_ttfSize);
    juce::Typeface::Ptr monoBold = juce::Typeface::createSystemTypefaceFor(DvlceUiFonts::RajdhaniSemiBold_ttf, DvlceUiFonts::RajdhaniSemiBold_ttfSize);
    juce::Typeface::Ptr square = juce::Typeface::createSystemTypefaceFor(DvlceUiFonts::RajdhaniMedium_ttf, DvlceUiFonts::RajdhaniMedium_ttfSize);
    juce::Typeface::Ptr squareBold = juce::Typeface::createSystemTypefaceFor(DvlceUiFonts::RajdhaniSemiBold_ttf, DvlceUiFonts::RajdhaniSemiBold_ttfSize);
    juce::Typeface::Ptr small = juce::Typeface::createSystemTypefaceFor(DvlceUiFonts::BarlowSemiCondensedMedium_ttf, DvlceUiFonts::BarlowSemiCondensedMedium_ttfSize);
    juce::Typeface::Ptr smallBold = juce::Typeface::createSystemTypefaceFor(DvlceUiFonts::BarlowSemiCondensedSemiBold_ttf, DvlceUiFonts::BarlowSemiCondensedSemiBold_ttfSize);
};
const Faces& faces() { static const Faces f; return f; }
// Space Mono advance is 0.612 em.
constexpr float monoAdvance = 0.612f;
juce::Font monoAt(float height, bool bold = false) {
    return juce::Font(juce::FontOptions(bold ? faces().monoBold : faces().mono)
                          .withHeight(height).withFeatureEnabled(juce::FontFeatureTag{"tnum"}));
}
juce::Font groteskAt(float height) {
    return juce::Font(juce::FontOptions(faces().grotesk).withHeight(height).withFeatureEnabled(juce::FontFeatureTag{"tnum"}));
}
// Rajdhani sits small on its em: 1.18 gives it the cap height of the other faces at the same size.
constexpr float squareScale = 1.18f;
juce::Font squareAt(float height, bool bold = false) {
    return juce::Font(juce::FontOptions(bold ? faces().squareBold : faces().square)
                          .withHeight(height * squareScale).withFeatureEnabled(juce::FontFeatureTag{"tnum"}));
}
// Quantum small text (labels, units, menus, scales): Rajdhani's thin strokes and tight counters blur below about
// 14 px, so small text uses Barlow Semi Condensed, which keeps open shapes at 10-12 px. 1.1 matches Rajdhani's cap height.
constexpr float smallScale = 1.1f, smallFloor = 10.5f, quantumTitleMin = 14.f;
juce::Font smallAt(float height, bool bold = false) {
    return juce::Font(juce::FontOptions(bold ? faces().smallBold : faces().small)
                          .withHeight(height * smallScale).withFeatureEnabled(juce::FontFeatureTag{"tnum"}));
}
float pi() { return juce::MathConstants<float>::pi; }
// Text is shaped once per (string, font, box, justification) and the glyph run is reused: the readouts
// repeat the same strings every frame, and shaping dominated a full paint.
std::string textKey(const juce::String& s, const juce::Font& f, float w, float h, int flags) {
    std::string key = s.toStdString();
    key += '|';
    key += std::to_string(reinterpret_cast<std::uintptr_t>(f.getTypefacePtr().get()));
    key += '|' + std::to_string(int(f.getHeight() * 20.f)) + '|' + std::to_string(int(f.getExtraKerningFactor() * 1000.f));
    key += '|' + std::to_string(int(w * 4.f)) + 'x' + std::to_string(int(h * 4.f)) + '|' + std::to_string(flags);
    return key;
}
void putText(juce::Graphics& g, const juce::String& s, const juce::Font& f, juce::Rectangle<float> r, juce::Justification just) {
    if (s.isEmpty() || r.isEmpty()) return;
    static thread_local std::unordered_map<std::string, juce::GlyphArrangement> cache;
    const auto key = textKey(s, f, r.getWidth(), r.getHeight(), just.getFlags());
    auto it = cache.find(key);
    if (it == cache.end()) {
        if (cache.size() > 2000) cache.clear();
        juce::GlyphArrangement arrangement;
        arrangement.addCurtailedLineOfText(f, s, 0.f, 0.f, r.getWidth(), false);
        arrangement.justifyGlyphs(0, arrangement.getNumGlyphs(), 0.f, 0.f, r.getWidth(), r.getHeight(), just);
        it = cache.emplace(key, std::move(arrangement)).first;
    }
    it->second.draw(g, juce::AffineTransform::translation(r.getX(), r.getY()));
}
float measure(const juce::Font& f, const juce::String& s) {
    static thread_local std::unordered_map<std::string, float> cache;
    const auto key = textKey(s, f, 0.f, 0.f, 0);
    auto it = cache.find(key);
    if (it == cache.end()) {
        if (cache.size() > 2000) cache.clear();
        it = cache.emplace(key, juce::GlyphArrangement::getStringWidth(f, s)).first;
    }
    return it->second;
}
juce::Point<float> onCircle(juce::Point<float> c, float a, float r) {
    return c + juce::Point<float>(std::sin(a) * r, -std::cos(a) * r);
}
juce::Path chamfer(juce::Rectangle<float> r, float cut = 8) {
    cut = juce::jmin(cut, r.getHeight() * .25f);
    juce::Path p;
    p.startNewSubPath(r.getX() + cut, r.getY()); p.lineTo(r.getRight(), r.getY());
    p.lineTo(r.getRight(), r.getBottom() - cut); p.lineTo(r.getRight() - cut, r.getBottom());
    p.lineTo(r.getX(), r.getBottom()); p.lineTo(r.getX(), r.getY() + cut); p.closeSubPath();
    return p;
}
// ---- cached light sprites: alpha-only white images, tinted once per colour ----
juce::Image makeSliceSprite() {
    constexpr int size = 48, lo = 18, hi = 30;
    juce::Image img(juce::Image::ARGB, size, size, true, juce::SoftwareImageType());
    juce::Image::BitmapData d(img, juce::Image::BitmapData::writeOnly);
    const float sigma = 4.2f;
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x) {
            const float dx = float(juce::jmax(lo - x, 0, x - hi)), dy = float(juce::jmax(lo - y, 0, y - hi));
            const float a = std::exp(-(dx * dx + dy * dy) / (2 * sigma * sigma));
            d.setPixelColour(x, y, juce::Colour::fromFloatRGBA(1, 1, 1, a));
        }
    return img;
}
const juce::Image& slice() { static const juce::Image img = makeSliceSprite(); return img; }
juce::Image makeRingSprite(int radius) {
    const int pad = 11, size = 2 * (radius + pad);
    juce::Image img(juce::Image::ARGB, size, size, true, juce::SoftwareImageType());
    juce::Image::BitmapData d(img, juce::Image::BitmapData::writeOnly);
    const float sigma = 3.4f, c = float(size) * .5f - .5f;
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x) {
            const float dist = std::hypot(float(x) - c, float(y) - c) - float(radius);
            d.setPixelColour(x, y, juce::Colour::fromFloatRGBA(1, 1, 1, std::exp(-dist * dist / (2 * sigma * sigma))));
        }
    return img;
}
// Plain image blits are far cheaper than clipping to an alpha mask, so each sprite is tinted once per colour.
juce::Image tint(const juce::Image& mask, juce::Colour c) {
    juce::Image out(juce::Image::ARGB, mask.getWidth(), mask.getHeight(), true, juce::SoftwareImageType());
    juce::Image::BitmapData in(mask, juce::Image::BitmapData::readOnly), dst(out, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < mask.getHeight(); ++y)
        for (int x = 0; x < mask.getWidth(); ++x)
            dst.setPixelColour(x, y, c.withAlpha(in.getPixelColour(x, y).getFloatAlpha()));
    return out;
}
const juce::Image& sliceTinted(juce::Colour c) {
    static std::map<juce::uint32, juce::Image> cache;
    const auto key = c.withAlpha(1.f).getARGB();
    auto it = cache.find(key);
    if (it == cache.end()) it = cache.emplace(key, tint(slice(), c)).first;
    return it->second;
}
const juce::Image& ringTinted(int radius, juce::Colour c) {
    static std::map<std::pair<int, juce::uint32>, juce::Image> sprites;
    const auto key = std::make_pair(radius, c.withAlpha(1.f).getARGB());
    auto it = sprites.find(key);
    if (it == sprites.end()) it = sprites.emplace(key, tint(makeRingSprite(radius), c)).first;
    return it->second;
}
} // namespace
juce::Font type(float size, bool bold, bool title) {
    if (isQuantum()) return title || size >= quantumTitleMin ? squareAt(size, bold || title) : smallAt(size, bold);
    if (title) return current == ThemeId::Spectrum ? juce::Font(juce::FontOptions(faces().anton).withHeight(size)) : groteskAt(size);
    if (current == ThemeId::Vault) return groteskAt(size);
    return monoAt(size * 1.1f, bold);
}
juce::Font numerals(float size) {
    if (isQuantum()) return size >= quantumTitleMin ? squareAt(size, true) : smallAt(size, true);
    return groteskAt(size);
}
void label(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, juce::uint32 c, float size) {
    if (classic()) {
        auto f = font(size);
        f.setExtraKerningFactor(0.08f);
        g.setFont(f);
        g.setColour(colour(c));
        g.drawText(s, r, juce::Justification::centred, false);
        return;
    }
    const bool section = size >= labelSize;
    if (isQuantum()) {
        // Never below the readable floor; a label that does not fit loses tracking first, then size (to 9 px).
        size = juce::jmax(size, smallFloor);
        float tracking = section ? .06f : .04f;
        auto f = smallAt(size, true);
        f.setExtraKerningFactor(tracking);
        float width = measure(f, s);
        if (width > r.getWidth() - 1.f) {
            tracking = 0.f;
            f.setExtraKerningFactor(tracking);
            width = measure(f, s);
        }
        if (width > r.getWidth() - 1.f) {
            f = smallAt(juce::jmax(9.f, size * (r.getWidth() - 1.f) / width), true);
            f.setExtraKerningFactor(tracking);
        }
        g.setColour(colour(c));
        putText(g, s, f, r, juce::Justification::centred);
        return;
    }
    if (isVault()) {
        const float tracking = section ? .1f : .03f;
        float h = size;
        const float width = float(s.length()) * h * (.6f + tracking);
        if (width > r.getWidth() - 1.f) h = juce::jmax(7.f, h * (r.getWidth() - 1.f) / width);
        auto f = groteskAt(h);
        f.setExtraKerningFactor(tracking);
        g.setColour(colour(c));
        putText(g, s, f, r, juce::Justification::centred);
        return;
    }
    const float tracking = section ? .14f : .04f;
    float h = size * 1.1f;
    const float width = float(s.length()) * h * (monoAdvance + tracking);
    if (width > r.getWidth() - 1.f) h = juce::jmax(7.f, h * (r.getWidth() - 1.f) / width);
    auto f = monoAt(h, section && c == text);
    f.setExtraKerningFactor(tracking);
    g.setColour(colour(c));
    putText(g, s, f, r, juce::Justification::centred);
}
void caption(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, bool alignRight) {
    if (classic()) { label(g, s, r, textDim, 9); return; }
    if (isQuantum()) {
        auto f = smallAt(11.f, true);
        f.setExtraKerningFactor(.05f);
        const float tw = measure(f, s);
        const float x = alignRight ? r.getRight() - tw - 12.f : r.getX() + 12.f, cy = r.getCentreY();
        g.setColour(colour(textDim).withAlpha(.9f));
        putText(g, s, f, juce::Rectangle<float>(x, r.getY(), tw + 2.f, r.getHeight()), juce::Justification::centredLeft);
        const float dx = alignRight ? r.getRight() - 4.f : r.getX() + 4.f;
        g.setColour(light().withAlpha(.85f));
        g.fillEllipse(dx - 1.6f, cy - 1.6f, 3.2f, 3.2f);
        return;
    }
    const bool vault = isVault();
    auto f = vault ? groteskAt(10.f) : monoAt(10.f);
    f.setExtraKerningFactor(vault ? .08f : .12f);
    g.setColour(colour(textDim).withAlpha(.9f));
    const float tw = float(s.length()) * 10.f * ((vault ? .6f : monoAdvance) + (vault ? .08f : .12f));
    const float x = alignRight ? r.getRight() - tw - 12.f : r.getX() + 12.f, cy = r.getCentreY();
    putText(g, s, f, juce::Rectangle<float>(x, r.getY(), tw + 2.f, r.getHeight()), juce::Justification::centredLeft);
    const float dx = alignRight ? r.getRight() - 4.f : r.getX() + 4.f;
    g.setColour(light().withAlpha(.8f));
    if (vault) g.fillRect(dx - 1.5f, cy - 1.5f, 3.f, 3.f);
    else {
        juce::Path d; d.addQuadrilateral(dx, cy - 2.f, dx + 2.f, cy, dx, cy + 2.f, dx - 2.f, cy);
        g.fillPath(d);
    }
}
void sectionLabel(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, bool rule) {
    if (classic()) { label(g, s, r, textDim, labelSize); return; }
    if (isQuantum()) {
        // Centred upper-case title between two hairlines, as on a panel border.
        auto f = squareAt(13.5f, true);
        f.setExtraKerningFactor(.08f);
        float tw = measure(f, s);
        if (tw > r.getWidth() - 2.f) {
            f = squareAt(juce::jmax(10.f, 13.5f * (r.getWidth() - 2.f) / tw), true);
            f.setExtraKerningFactor(.08f);
            tw = measure(f, s);
        }
        g.setColour(colour(text).withAlpha(.94f));
        putText(g, s, f, r, juce::Justification::centred);
        if (rule) {
            const float cy = std::floor(r.getCentreY()) + .5f, left = r.getCentreX() - tw * .5f - 9.f, right = r.getCentreX() + tw * .5f + 9.f;
            if (left - 4.f > r.getX()) {
                g.setGradientFill(juce::ColourGradient(colour(line).withAlpha(0.f), r.getX(), cy, colour(line), left, cy, false));
                g.fillRect(r.getX(), cy - .5f, left - r.getX(), 1.f);
            }
            if (right + 4.f < r.getRight()) {
                g.setGradientFill(juce::ColourGradient(colour(line), right, cy, colour(line).withAlpha(0.f), r.getRight(), cy, false));
                g.fillRect(right, cy - .5f, r.getRight() - right, 1.f);
            }
        }
        return;
    }
    if (isVault()) {
        const float natural = float(s.length()) * 11.f * (.62f + .1f);
        const float h = natural > r.getWidth() - 2.f ? juce::jmax(7.f, 11.f * (r.getWidth() - 2.f) / natural) : 11.f;
        auto f = groteskAt(h);
        f.setExtraKerningFactor(.1f);
        g.setColour(colour(text));
        putText(g, s, f, r, juce::Justification::centred);
        g.setColour(light().withAlpha(.9f));
        g.fillRect(r.getCentreX() - 7.f, r.getBottom() - 1.f, 14.f, 1.f);
        return;
    }
    const float tracking = .2f;
    const float natural = float(s.length()) * 11.5f * (monoAdvance + tracking);
    const float h = natural > r.getWidth() - 2.f ? juce::jmax(7.f, 11.5f * (r.getWidth() - 2.f) / natural) : 11.5f;
    const float tw = float(s.length()) * h * (monoAdvance + tracking);
    auto f = monoAt(h, false);
    f.setExtraKerningFactor(tracking);
    g.setColour(colour(text).withAlpha(.92f));
    putText(g, s, f, r, juce::Justification::centred);
    const float cy = std::floor(r.getCentreY()) + .5f, left = r.getCentreX() - tw * .5f - 7.f, right = r.getCentreX() + tw * .5f + 6.f;
    g.setColour(light().withAlpha(.9f));
    if (isRadiant()) {
        // Open registration brackets either side of the label.
        g.drawLine(left, cy - 4.f, left, cy + 4.f, .8f);
        g.drawLine(left, cy - 4.f, left + 2.5f, cy - 4.f, .8f);
        g.drawLine(left, cy + 4.f, left + 2.5f, cy + 4.f, .8f);
        g.drawLine(right, cy - 4.f, right, cy + 4.f, .8f);
        g.drawLine(right, cy - 4.f, right - 2.5f, cy - 4.f, .8f);
        g.drawLine(right, cy + 4.f, right - 2.5f, cy + 4.f, .8f);
    } else {
        juce::Path diamond;
        diamond.addQuadrilateral(left, cy - 2.2f, left + 2.2f, cy, left, cy + 2.2f, left - 2.2f, cy);
        g.fillPath(diamond);
    }
    if (rule) {
        g.setColour(colour(line).withAlpha(.9f));
        if (left - 6.f > r.getX() + 1) g.drawHorizontalLine(int(cy), r.getX(), left - 6.f);
        if (right + 5.f < r.getRight() - 1) g.drawHorizontalLine(int(cy), right + 5.f, r.getRight());
    }
}
} // namespace Theme
void drawGlow(juce::Graphics& g, const juce::Path& path, float intensity) {
    if (!Theme::classic()) {
        // Purple signature halo for engaged / dragged / selected shapes. Short strokes, no blur.
        const auto c = Theme::colour(Theme::purple);
        const float k = Theme::isLight() ? .6f : 1.f;
        for (auto width : {7.f, 3.5f}) {
            g.setColour(c.withAlpha((width > 5 ? .07f : .16f) * intensity * k));
            g.strokePath(path, juce::PathStrokeType(width));
        }
        return;
    }
    for (auto pair : {std::pair<int, float>{14, 0.18f}, {7, 0.30f}, {2, 0.55f}})
        juce::DropShadow(
            Theme::colour(Theme::purple).withAlpha(pair.second * juce::jlimit(0.f, 1.f, intensity)),
            pair.first, {})
            .drawForPath(g, path);
}
namespace Theme {
void bloomRect(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, float strength, float spread) {
    strength *= bloomScale();
    if (r.isEmpty() || strength <= 0) return;
    const auto& img = sliceTinted(c);
    const int border = 18; // sprite pixels outside the core on each side
    const float scale = juce::jmax(.4f, spread / 8.f);
    const int pad = juce::roundToInt(float(border) * scale);
    const auto outer = r.expanded(float(pad)).getSmallestIntegerContainer();
    const int sx[4]{0, border, 30, 48};
    const int dx[4]{outer.getX(), outer.getX() + pad, outer.getRight() - pad, outer.getRight()};
    const int dy[4]{outer.getY(), outer.getY() + pad, outer.getBottom() - pad, outer.getBottom()};
    juce::Graphics::ScopedSaveState save(g);
    const float k = juce::jlimit(0.f, 1.f, strength * c.getFloatAlpha());
    // The core of the sprite is solid: a plain fill is far cheaper than scaling it. Only the soft rim is blitted
    // (nearest sampling; the rim is smooth anyway).
    if (dx[2] > dx[1] && dy[2] > dy[1]) {
        g.setColour(c.withAlpha(k));
        g.fillRect(dx[1], dy[1], dx[2] - dx[1], dy[2] - dy[1]);
    }
    g.setOpacity(k);
    g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
    for (int j = 0; j < 3; ++j)
        for (int i = 0; i < 3; ++i) {
            if (i == 1 && j == 1) continue;
            const int w = dx[i + 1] - dx[i], h = dy[j + 1] - dy[j];
            if (w > 0 && h > 0)
                g.drawImage(img, dx[i], dy[j], w, h, sx[i], sx[j], sx[i + 1] - sx[i], sx[j + 1] - sx[j], false);
        }
}
void plotCurve(juce::Graphics& g, const juce::Path& path, juce::Colour c, float width, float bloom) {
    if (classic()) { g.setColour(c); g.strokePath(path, juce::PathStrokeType(width)); return; }
    bloom *= isVault() ? .35f : (isLight() ? .6f : 1.f);
    g.setColour(c.withAlpha(.045f * bloom));
    g.strokePath(path, juce::PathStrokeType(width + 7.f));
    g.setColour(c.withAlpha(.09f * bloom));
    g.strokePath(path, juce::PathStrokeType(width + 3.5f));
    g.setColour(c);
    g.strokePath(path, juce::PathStrokeType(width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
void glass(juce::Graphics& g, juce::Rectangle<float> r, float strength, bool corners) {
    const float radius = design().radius;
    if (isQuantum()) {
        // Raised slate panel: soft shadow, top-lit body, cool hairline and an inner top light.
        const bool lightUi = isLight();
        g.setColour(juce::Colours::black.withAlpha(lightUi ? .07f : .38f));
        g.fillRoundedRectangle(r.translated(0.f, 2.f).expanded(1.f), radius + 1.f);
        g.setGradientFill(juce::ColourGradient(lightUi ? juce::Colour(0xffffffff) : juce::Colour(0xff1b242d), r.getX(), r.getY(),
                                               lightUi ? juce::Colour(0xfff0f3f7) : juce::Colour(0xff141a21), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle(r, radius);
        g.setColour(colour(line).withAlpha(.6f + .4f * strength));
        g.drawRoundedRectangle(r.reduced(.5f), radius, 1.f);
        if (r.getWidth() > 2.f * radius + 4.f) {
            g.setColour(juce::Colours::white.withAlpha(lightUi ? .9f : .055f));
            g.fillRect(r.getX() + radius, r.getY() + 1.f, r.getWidth() - 2.f * radius, 1.f);
        }
        if (corners && r.getWidth() > 60 && r.getHeight() > 40) {
            // A short accent light on the top edge, centred: the panel's "power" mark.
            const float w = juce::jmin(48.f, r.getWidth() * .2f), cx = r.getCentreX();
            g.setGradientFill(juce::ColourGradient(light().withAlpha(0.f), cx - w, r.getY(), light().withAlpha(.75f), cx, r.getY(), false));
            g.fillRect(cx - w, r.getY(), w, 1.f);
            g.setGradientFill(juce::ColourGradient(light().withAlpha(.75f), cx, r.getY(), light().withAlpha(0.f), cx + w, r.getY(), false));
            g.fillRect(cx, r.getY(), w, 1.f);
        }
        return;
    }
    if (isVault()) {
        // Chamfered plate with a top light and one facet.
        auto p = chamfer(r);
        g.setGradientFill(juce::ColourGradient(isLight() ? juce::Colours::white : juce::Colour(0xff151c24), r.getX(), r.getY(),
                                               colour(panel), r.getX(), r.getBottom(), false));
        g.fillPath(p);
        g.setColour(colour(line).withAlpha(.65f + .2f * strength)); g.strokePath(p, juce::PathStrokeType(.7f));
        juce::Path facet; facet.addTriangle(r.getX(), r.getY() + 8, r.getRight(), r.getY(), r.getRight(), r.getY() + r.getHeight() * .22f);
        g.setColour(light().withAlpha(isLight() ? .04f : .025f)); g.fillPath(facet);
        return;
    }
    if (isRadiant()) {
        // A projection: open registration corners and a short top light; almost no body.
        g.setColour(body().withAlpha((isLight() ? .35f : .3f) * strength));
        g.fillRect(r);
        g.setColour(colour(line).withAlpha(.7f));
        const float length = 12.f;
        for (auto corner : {r.getTopLeft(), r.getTopRight(), r.getBottomLeft(), r.getBottomRight()}) {
            const float dx = corner.x < r.getCentreX() ? length : -length;
            const float dy = corner.y < r.getCentreY() ? length : -length;
            g.drawLine(corner.x, corner.y, corner.x + dx, corner.y, .7f);
            g.drawLine(corner.x, corner.y, corner.x, corner.y + dy, .7f);
        }
        g.setColour(light().withAlpha(.6f));
        g.drawLine(r.getX() + 8, r.getY(), r.getX() + 24, r.getY(), .8f);
        return;
    }
    g.setColour(body().withAlpha((isLight() ? .6f : .58f) * strength));
    g.fillRoundedRectangle(r, radius);
    g.setGradientFill(juce::ColourGradient((isLight() ? juce::Colours::white : ice()).withAlpha((isLight() ? .55f : .05f) * strength), r.getX(), r.getY(),
                                           ice().withAlpha(.0f), r.getX(), r.getY() + juce::jmin(r.getHeight(), 90.f), false));
    g.fillRoundedRectangle(r, radius);
    g.setColour(colour(line).withAlpha(.95f));
    g.drawRoundedRectangle(r.reduced(.5f), radius, .75f);
    if (r.getWidth() > 24) {
        const float peak = isLight() ? .8f : .55f;
        g.setGradientFill(juce::ColourGradient(light().withAlpha(0.f), r.getX() + 6, r.getY(),
                                               light().withAlpha(peak), r.getX() + 6 + (r.getWidth() - 12) * .25f, r.getY(), false));
        g.fillRect(r.getX() + 6, r.getY(), (r.getWidth() - 12) * .5f, 1.f);
        g.setGradientFill(juce::ColourGradient(light().withAlpha(peak), r.getCentreX(), r.getY(),
                                               light().withAlpha(0.f), r.getRight() - 6, r.getY(), false));
        g.fillRect(r.getCentreX(), r.getY(), r.getWidth() * .5f - 6, 1.f);
    }
    if (corners && r.getWidth() > 40 && r.getHeight() > 30) {
        const float t = 5.f;
        g.setColour(light().withAlpha(.8f));
        g.drawLine(r.getX() + .5f, r.getY() + .5f, r.getX() + .5f + t, r.getY() + .5f, 1.f);
        g.drawLine(r.getX() + .5f, r.getY() + .5f, r.getX() + .5f, r.getY() + .5f + t, 1.f);
        g.drawLine(r.getRight() - .5f, r.getBottom() - .5f, r.getRight() - .5f - t, r.getBottom() - .5f, 1.f);
        g.drawLine(r.getRight() - .5f, r.getBottom() - .5f, r.getRight() - .5f, r.getBottom() - .5f - t, 1.f);
        g.setColour(colour(textDim).withAlpha(.45f));
        g.drawLine(r.getRight() - .5f, r.getY() + .5f, r.getRight() - .5f - t, r.getY() + .5f, 1.f);
        g.drawLine(r.getRight() - .5f, r.getY() + .5f, r.getRight() - .5f, r.getY() + .5f + t, 1.f);
        g.drawLine(r.getX() + .5f, r.getBottom() - .5f, r.getX() + .5f + t, r.getBottom() - .5f, 1.f);
        g.drawLine(r.getX() + .5f, r.getBottom() - .5f, r.getX() + .5f, r.getBottom() - .5f - t, 1.f);
    }
}
void chip(juce::Graphics& g, juce::Rectangle<float> r, bool on, bool hover, bool enabled) {
    const float alpha = enabled ? 1.f : .45f;
    if (isQuantum()) {
        // Dark rounded key; engaged: accent outline, accent tint and an outer glow.
        const float radius = juce::jmin(4.f, r.getHeight() * .5f);
        const bool lightUi = isLight();
        if (on) bloomRect(g, r.reduced(1.f), light(), .5f * alpha, 6.f);
        if (on) {
            g.setColour(colour(bg).withAlpha(alpha));
            g.fillRoundedRectangle(r, radius);
            g.setColour(light().withAlpha(.16f * alpha));
        } else {
            const auto top = lightUi ? juce::Colour(0xffffffff) : juce::Colour(hover ? 0xff28323c : 0xff222b34);
            const auto bottom = lightUi ? juce::Colour(hover ? 0xffe6ecf2 : 0xffeef2f6) : juce::Colour(hover ? 0xff1d252d : 0xff191f26);
            g.setGradientFill(juce::ColourGradient(top.withMultipliedAlpha(alpha), r.getX(), r.getY(), bottom.withMultipliedAlpha(alpha), r.getX(), r.getBottom(), false));
        }
        g.fillRoundedRectangle(r, radius);
        g.setColour((on ? light() : (hover ? colour(textDim) : colour(line))).withAlpha(alpha * (on ? 1.f : .95f)));
        g.drawRoundedRectangle(r.reduced(.5f), radius, on ? 1.2f : 1.f);
        if (r.getWidth() > 2.f * radius + 2.f) {
            g.setColour((on ? ice().withAlpha(.35f) : juce::Colours::white.withAlpha(lightUi ? .8f : .06f)).withMultipliedAlpha(alpha));
            g.fillRect(r.getX() + radius, r.getY() + 1.f, r.getWidth() - 2.f * radius, 1.f);
        }
        return;
    }
    if (isRadiant()) {
        // Open readout: bottom hairline with end ticks; a tint when engaged.
        if (on) { g.setColour(colour(purple).withAlpha(.10f * alpha)); g.fillRect(r); }
        else if (hover) { g.setColour(light().withAlpha(.08f * alpha)); g.fillRect(r); }
        g.setColour((on ? colour(purple) : colour(line)).withAlpha(alpha));
        g.drawLine(r.getX(), r.getBottom(), r.getRight(), r.getBottom(), on ? 1.f : .7f);
        g.drawLine(r.getX(), r.getBottom() - 4, r.getX(), r.getBottom(), .7f);
        g.drawLine(r.getRight(), r.getBottom() - 4, r.getRight(), r.getBottom(), .7f);
        if (on) { g.setColour(colour(purple).withAlpha(.9f * alpha)); g.fillRect(r.getX() + 4, r.getBottom() - 2, r.getWidth() - 8, 1.f); }
        return;
    }
    if (isVault()) {
        auto p = chamfer(r, 4);
        g.setColour(colour(surface).withAlpha(alpha)); g.fillPath(p);
        g.setColour((on ? colour(purple) : (hover ? colour(textDim) : colour(line))).withAlpha(alpha)); g.strokePath(p, juce::PathStrokeType(.8f));
        if (on) {
            g.setColour(colour(purple).withAlpha(.06f * alpha)); g.fillRect(r);
            g.setColour(colour(purple).withAlpha(alpha)); g.fillRect(r.getX() + 4, r.getBottom() - 2, r.getWidth() - 8, 1.f);
        }
        return;
    }
    const float radius = juce::jmin(design().radius, r.getHeight() * .5f);
    if (on) bloomRect(g, r.reduced(2), colour(purple), .22f * alpha, 5.f);
    g.setColour((on ? colour(purple) : ice()).withAlpha((on ? .13f : (hover ? .08f : .035f)) * alpha));
    g.fillRoundedRectangle(r, radius);
    g.setColour((on ? colour(purple) : (hover ? colour(textDim) : colour(line))).withAlpha(alpha));
    g.drawRoundedRectangle(r.reduced(.5f), radius, on ? 1.f : .8f);
    if (on) {
        g.setColour(colour(purpleHi).withAlpha(.9f * alpha));
        g.fillRect(r.getX() + radius + 1, r.getBottom() - 1.5f, r.getWidth() - 2 * radius - 2, 1.f);
    }
}
void field(juce::Graphics& g, juce::Rectangle<float> r, bool active) { chip(g, r, active, false, true); }
void panelShape(juce::Graphics& g, juce::Rectangle<float> r) { glass(g, r, 1.f, true); }
void valueReadout(juce::Graphics& g, const juce::String& value, juce::Rectangle<float> r) {
    if (classic()) {
        g.setFont(font(valueSize));
        g.setColour(colour(text));
        g.drawFittedText(value, r.toNearestInt(), juce::Justification::centred, 1, .85f);
        return;
    }
    g.setColour(colour(text));
    if (isRadiant()) {
        g.setFont(font(design().valueSize, true));
        g.drawFittedText(value, r.toNearestInt(), juce::Justification::centred, 1, .85f);
        return;
    }
    const auto first = value.isEmpty() ? juce::juce_wchar(0) : value[0];
    const bool numeric = (first >= '0' && first <= '9') || first == '-' || first == '+' || first == '.' ||
                         first == juce::juce_wchar(0x2212);
    const auto number = numeric ? value.upToFirstOccurrenceOf(" ", false, false) : value;
    const auto unit = numeric ? value.fromFirstOccurrenceOf(" ", false, false) : juce::String();
    const float size = juce::jlimit(11.f, 21.f, r.getHeight() * .8f);
    auto main = numerals(size);
    auto suffix = isQuantum() ? smallAt(juce::jmax(smallFloor, size * .6f)) : isVault() ? groteskAt(juce::jmax(9.f, size * .55f)) : monoAt(juce::jmax(9.f, size * .6f));
    float numberWidth = measure(main, number);
    const float unitWidth = unit.isEmpty() ? 0.f : measure(suffix, unit) + 3.f;
    const float avail = r.getWidth() - 6.f;
    if (numberWidth + unitWidth > avail) {
        main = main.withHeight(size * juce::jmax(.5f, (avail - unitWidth) / juce::jmax(1.f, numberWidth)));
        numberWidth = measure(main, number);
    }
    const float x = r.getCentreX() - (numberWidth + unitWidth) * .5f;
    putText(g, number, main, juce::Rectangle<float>{x, r.getY(), numberWidth + 2, r.getHeight()}, juce::Justification::centredLeft);
    if (unit.isNotEmpty()) {
        g.setColour(colour(textDim));
        putText(g, unit, suffix, juce::Rectangle<float>{x + numberWidth + 3, r.getY() + 1.5f, unitWidth + 4, r.getHeight() - 1.5f},
                juce::Justification::centredLeft);
    }
}
namespace {
void drawVaultDial(juce::Graphics& g, juce::Point<float> centre, float radius, float value, float start, float end, const DialOptions& o) {
    const float angle = start + value * (end - start);
    const auto lit = o.active || o.reduction ? colour(purple) : light();
    g.setColour(colour(surface));
    g.fillEllipse(centre.x - radius + 4, centre.y - radius + 4, (radius - 4) * 2, (radius - 4) * 2);
    juce::Path ring, arc;
    const float lo = juce::jmin(o.origin, value), hi = juce::jmax(o.origin, value);
    ring.addCentredArc(centre.x, centre.y, radius, radius, 0, start, end, true);
    arc.addCentredArc(centre.x, centre.y, radius, radius, 0, start + lo * (end - start), start + hi * (end - start), true);
    g.setColour(colour(line)); g.strokePath(ring, juce::PathStrokeType(1));
    g.setColour(lit); g.strokePath(arc, juce::PathStrokeType(1.6f));
    for (int i = 0; i < 9; ++i) {
        const float a = start + (end - start) * float(i) / 8.f;
        g.setColour(colour(textDim)); g.drawLine({onCircle(centre, a, radius - 4), onCircle(centre, a, radius - 6)}, .7f);
    }
    if (o.peak >= 0.f) {
        const float a = start + juce::jlimit(0.f, 1.f, o.peak) * (end - start);
        g.setColour(colour(text));
        g.drawLine({onCircle(centre, a, radius - 7), onCircle(centre, a, radius + 1)}, 1.4f);
    }
    g.setColour(lit);
    if (o.pointer) g.drawLine({onCircle(centre, angle, radius * .42f), onCircle(centre, angle, juce::jmax(radius * .55f, radius - 13))}, 1.3f);
    const auto tip = onCircle(centre, angle, radius);
    g.fillEllipse(tip.x - 1.7f, tip.y - 1.7f, 3.4f, 3.4f);
}
void drawRadiantDial(juce::Graphics& g, juce::Point<float> centre, float radius, float value, float start, float end, const DialOptions& o) {
    const float angle = start + value * (end - start);
    const auto lit = o.active || o.reduction ? colour(purple) : light();
    const float lo = juce::jmin(o.origin, value), hi = juce::jmax(o.origin, value);
    auto arc = [&](float r, float a, float b, juce::Colour col, float stroke) {
        juce::Path p; p.addCentredArc(centre.x, centre.y, r, r, 0, a, b, true);
        g.setColour(col); g.strokePath(p, juce::PathStrokeType(stroke));
    };
    // Projected value arc: soft bloom wedge under it, hairline concentric rings, tick constellation.
    if (hi - lo > .004f) {
        juce::Path wedge;
        wedge.addPieSegment(centre.x - radius - 8, centre.y - radius - 8, (radius + 8) * 2, (radius + 8) * 2,
                            start + lo * (end - start) - .05f, start + hi * (end - start) + .05f, 0.f);
        juce::Graphics::ScopedSaveState save(g);
        g.reduceClipRegion(wedge);
        g.setOpacity(juce::jlimit(0.f, 1.f, (o.active ? .7f : .5f) * bloomScale() * o.glow * (1.f + .5f * activity)));
        const auto& img = ringTinted(juce::roundToInt(radius - 5), lit);
        g.drawImageAt(img, juce::roundToInt(centre.x) - img.getWidth() / 2, juce::roundToInt(centre.y) - img.getHeight() / 2, false);
    }
    for (float factor : {1.f, .86f}) arc(radius * factor, start, end, colour(line), .6f);
    arc(radius * .68f, 0, 2 * pi(), colour(line).withAlpha(.65f), .5f);
    arc(radius - 5, start + lo * (end - start), start + hi * (end - start), lit, 1.2f);
    juce::Path dotsOn, dotsOff;
    for (int i = 0; i <= 36; ++i) {
        const float f = float(i) / 36.f;
        const auto p = onCircle(centre, start + (end - start) * f, radius);
        ((f >= lo - .01f && f <= hi + .01f) ? dotsOn : dotsOff).addEllipse(p.x - .65f, p.y - .65f, 1.3f, 1.3f);
    }
    g.setColour(colour(line)); g.fillPath(dotsOff);
    g.setColour(lit); g.fillPath(dotsOn);
    juce::Path ticks;
    for (int i = 0; i <= 8; ++i) {
        const float a = start + (end - start) * float(i) / 8.f;
        ticks.addLineSegment({onCircle(centre, a, radius + 2), onCircle(centre, a, radius + 4)}, .6f);
    }
    g.setColour(colour(textDim)); g.fillPath(ticks);
    if (o.peak >= 0.f) {
        const float a = start + juce::jlimit(0.f, 1.f, o.peak) * (end - start);
        g.setColour(colour(text));
        g.drawLine({onCircle(centre, a, radius - 9), onCircle(centre, a, radius + 3)}, 1.4f);
    }
    g.setColour(lit);
    if (o.pointer) g.drawLine({onCircle(centre, angle, radius * .42f), onCircle(centre, angle, juce::jmax(radius * .55f, radius - 13))}, 1.3f);
    const auto tip = onCircle(centre, angle, radius - 5);
    g.fillEllipse(tip.x - 1.7f, tip.y - 1.7f, 3.4f, 3.4f);
}
void drawQuantumDial(juce::Graphics& g, juce::Point<float> centre, float radius, float value, float start, float end, const DialOptions& o) {
    const float angle = start + value * (end - start);
    const bool hotRole = o.active || o.reduction;
    const auto lit = hotRole ? colour(purple) : light();
    const auto litHi = hotRole ? colour(purpleHi) : ice();
    const float lo = juce::jmin(o.origin, value), hi = juce::jmax(o.origin, value);
    const bool big = radius >= 34.f, mid = radius >= 18.f, lightUi = isLight();
    const float stroke = big ? 3.4f : mid ? 2.8f : 2.f;
    const float arcR = radius - stroke * .5f - (big ? 5.f : 1.f);
    auto arcPath = [&](float r, float a, float b) { juce::Path p; p.addCentredArc(centre.x, centre.y, r, r, 0, a, b, true); return p; };
    const juce::PathStrokeType arcStroke(stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    // Track, then the glowing value arc.
    g.setColour(lightUi ? colour(line).withAlpha(.8f) : juce::Colour(0xff0b0f13));
    g.strokePath(arcPath(arcR, start, end), arcStroke);
    g.setColour(colour(line).withAlpha(lightUi ? .0f : .55f));
    g.strokePath(arcPath(arcR + stroke * .5f + .5f, start, end), juce::PathStrokeType(.6f));
    const float a0 = start + lo * (end - start), a1 = start + hi * (end - start);
    if (hi - lo > .004f) {
        juce::Path wedge;
        wedge.addPieSegment(centre.x - arcR - 14, centre.y - arcR - 14, (arcR + 14) * 2, (arcR + 14) * 2, a0 - .08f, a1 + .08f, 0.f);
        {
            juce::Graphics::ScopedSaveState save(g);
            g.reduceClipRegion(wedge);
            g.setOpacity(juce::jlimit(0.f, 1.f, (o.active ? .95f : .8f) * bloomScale() * o.glow * (1.f + .5f * activity)));
            const auto& img = ringTinted(juce::roundToInt(arcR), lit);
            g.drawImageAt(img, juce::roundToInt(centre.x) - img.getWidth() / 2, juce::roundToInt(centre.y) - img.getHeight() / 2, false);
        }
        const auto p0 = onCircle(centre, a0, arcR), p1 = onCircle(centre, a1, arcR);
        if (p0.getDistanceFrom(p1) > 2.f) g.setGradientFill(juce::ColourGradient(lit, p0.x, p0.y, litHi, p1.x, p1.y, false));
        else g.setColour(litHi);
        g.strokePath(arcPath(arcR, a0, a1), arcStroke);
    }
    // Scale ticks outside the arc on large dials.
    if (big) {
        juce::Path ticks;
        for (int i = 0; i <= 10; ++i) {
            const float a = start + (end - start) * float(i) / 10.f;
            ticks.addLineSegment({onCircle(centre, a, arcR + stroke * .5f + 2.5f), onCircle(centre, a, arcR + stroke * .5f + (i % 5 == 0 ? 6.f : 4.5f))}, .9f);
        }
        g.setColour(colour(textDim).withAlpha(.7f));
        g.fillPath(ticks);
    }
    if (o.peak >= 0.f) {
        const float a = start + juce::jlimit(0.f, 1.f, o.peak) * (end - start);
        g.setColour(colour(text));
        g.drawLine({onCircle(centre, a, arcR - stroke - 1.f), onCircle(centre, a, arcR + stroke + 1.f)}, 1.4f);
    }
    // Convex cap: shadow, top-lit body, rim, and a pointer in the accent.
    if (o.pointer || mid) {
        const float capR = arcR - stroke * .5f - (big ? 6.f : mid ? 4.f : 2.5f);
        if (capR > 3.f) {
            g.setColour(juce::Colours::black.withAlpha(lightUi ? .12f : .5f));
            g.fillEllipse(centre.x - capR - 1.f, centre.y - capR + 1.f, (capR + 1.f) * 2.f, (capR + 1.f) * 2.f);
            juce::ColourGradient body(lightUi ? juce::Colour(0xffffffff) : juce::Colour(0xff323d48), centre.x, centre.y - capR * .7f,
                                      lightUi ? juce::Colour(0xffdfe5ec) : juce::Colour(0xff151a20), centre.x, centre.y + capR, true);
            g.setGradientFill(body);
            g.fillEllipse(centre.x - capR, centre.y - capR, capR * 2.f, capR * 2.f);
            g.setColour(lightUi ? colour(line) : juce::Colour(0xff0a0d10));
            g.drawEllipse(centre.x - capR, centre.y - capR, capR * 2.f, capR * 2.f, 1.f);
            juce::Path rim;
            rim.addCentredArc(centre.x, centre.y, capR - 1.f, capR - 1.f, 0, -pi() * .4f, pi() * .4f, true);
            g.setColour(juce::Colours::white.withAlpha(lightUi ? .9f : .12f));
            g.strokePath(rim, juce::PathStrokeType(1.f));
            if (o.pointer) {
                g.setColour(litHi);
                g.drawLine({onCircle(centre, angle, capR * (big ? .38f : .3f)), onCircle(centre, angle, capR * .86f)}, big ? 2.2f : 1.6f);
            }
        }
    }
    // Light at the end of the arc.
    if (mid && hi - lo > .004f) {
        const auto tip = onCircle(centre, angle, arcR);
        g.setColour(litHi.withAlpha(.35f));
        g.fillEllipse(tip.x - stroke * 1.3f, tip.y - stroke * 1.3f, stroke * 2.6f, stroke * 2.6f);
        g.setColour(juce::Colours::white.withAlpha(lightUi ? .0f : .9f));
        g.fillEllipse(tip.x - stroke * .45f, tip.y - stroke * .45f, stroke * .9f, stroke * .9f);
    }
}
void dialDirect(juce::Graphics& g, juce::Rectangle<float> bounds, float value, float start, float end, const DialOptions& o);
std::uint64_t floatKey(float v, float steps) { return std::uint64_t(std::int64_t(std::lround(double(v) * double(steps))) + (1ll << 40)); }
} // namespace
void dial(juce::Graphics& g, juce::Rectangle<float> bounds, float value, float start, float end, const DialOptions& o) {
    value = juce::jlimit(0.f, 1.f, value);
    // Bloom sprites, clipped wedges and the Quantum cap are costly; every dial picture is rendered once per state and
    // blitted afterwards. Classic and Vault draw a few hairlines and stay direct. At a fractional scale (150 %) the
    // picture is made at the physical size and may land half a pixel off; a soft arc does not show it.
    const float scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    if (classic() || isVault() || bounds.isEmpty() || scale <= 0.f) {
        dialDirect(g, bounds, value, start, end, o);
        return;
    }
    constexpr float pad = 14.f;
    const float ox = std::floor(bounds.getX() - pad), oy = std::floor(bounds.getY() - pad);
    const int w = int(std::ceil(bounds.getRight() + pad - ox)), h = int(std::ceil(bounds.getBottom() + pad - oy));
    std::uint64_t k = mix(floatKey(value, 2048.f), floatKey(start, 1000.f));
    for (auto v : {floatKey(end, 1000.f), floatKey(o.origin, 1000.f), floatKey(o.peak, 512.f), floatKey(o.glow, 32.f),
                   floatKey(bounds.getX() - ox, 8.f), floatKey(bounds.getY() - oy, 8.f), floatKey(bounds.getWidth(), 8.f),
                   floatKey(bounds.getHeight(), 8.f), floatKey(scale, 8.f), floatKey(activity, 5.f),
                   std::uint64_t(o.active) | std::uint64_t(o.pointer) << 1 | std::uint64_t(o.reduction) << 2 |
                       std::uint64_t(o.constellation) << 3 | std::uint64_t(o.bars) << 8,
                   std::uint64_t(current) | std::uint64_t(lightMode) << 4 | std::uint64_t(accent) << 8})
        k = mix(k, v);
    static thread_local std::unordered_map<std::uint64_t, juce::Image> cache;
    auto it = cache.find(k);
    if (it == cache.end()) {
        if (cache.size() > 384) cache.clear();
        juce::Image img(juce::Image::ARGB, juce::roundToInt(float(w) * scale), juce::roundToInt(float(h) * scale), true, juce::SoftwareImageType());
        {
            juce::Graphics ig(img);
            ig.addTransform(juce::AffineTransform::translation(-ox, -oy).scaled(scale));
            dialDirect(ig, bounds, value, start, end, o);
        }
        it = cache.emplace(k, std::move(img)).first;
    }
    if (std::abs(scale - 1.f) < .02f) g.drawImageAt(it->second, int(ox), int(oy));
    else g.drawImage(it->second, juce::Rectangle<float>(ox, oy, float(w), float(h)));
}
namespace {
void dialDirect(juce::Graphics& g, juce::Rectangle<float> bounds, float value, float start, float end, const DialOptions& o) {
    const auto centre = bounds.getCentre();
    const float radius = juce::jmax(3.f, juce::jmin(bounds.getWidth(), bounds.getHeight()) * .5f - 3.f);
    const float angle = start + value * (end - start);
    if (classic()) {
        g.setColour(colour(surface)); g.fillEllipse(centre.x - radius + 4, centre.y - radius + 4, (radius - 4) * 2, (radius - 4) * 2);
        juce::Path ring, arc;
        ring.addCentredArc(centre.x, centre.y, radius, radius, 0, start, end, true);
        arc.addCentredArc(centre.x, centre.y, radius, radius, 0, start, angle, true);
        g.setColour(colour(line)); g.strokePath(ring, juce::PathStrokeType(1));
        g.setColour(o.active ? colour(purple) : colour(text)); g.strokePath(arc, juce::PathStrokeType(1.4f));
        for (int i = 0; i < 9; ++i) {
            const float a = start + (end - start) * float(i) / 8.f;
            g.setColour(colour(textDim)); g.drawLine({onCircle(centre, a, radius - 4), onCircle(centre, a, radius - 6)}, .7f);
        }
        g.setColour(o.active ? colour(purple) : colour(text));
        if (o.pointer) g.drawLine({onCircle(centre, angle, radius * .42f), onCircle(centre, angle, juce::jmax(radius * .55f, radius - 13))}, 1.3f);
        const auto tip = onCircle(centre, angle, radius);
        g.fillEllipse(tip.x - 1.7f, tip.y - 1.7f, 3.4f, 3.4f);
        return;
    }
    if (isVault()) { drawVaultDial(g, centre, radius, value, start, end, o); return; }
    if (isQuantum()) { drawQuantumDial(g, centre, radius, value, start, end, o); return; }
    if (isRadiant()) { drawRadiantDial(g, centre, radius, value, start, end, o); return; }
    const bool big = radius >= 40, mid = radius >= 22;
    const float segLength = juce::jlimit(4.f, 10.f, radius * .2f);
    const float segOuter = radius - 4.f, segInner = segOuter - segLength, segMid = (segOuter + segInner) * .5f;
    const int bars = o.bars > 0 ? o.bars : (big ? 44 : mid ? 30 : 20);
    const float segWidth = big ? 2.2f : mid ? 1.8f : 1.4f;
    const auto lit = o.reduction || o.active ? colour(purple) : light();
    const auto litEnd = o.reduction || o.active ? colour(purpleHi) : ice();
    const float lo = juce::jmin(o.origin, value), hi = juce::jmax(o.origin, value);
    const float eps = .5f / float(bars - 1);
    // Projected light: faint disc inside the ring, then bloom under the lit wedge. Both follow audio activity.
    g.setGradientFill(juce::ColourGradient(lit.withAlpha((o.active ? .12f : .07f) * (1.f + .6f * activity) * (isLight() ? .6f : 1.f)), centre.x, centre.y,
                                           lit.withAlpha(0.f), centre.x + radius * .86f, centre.y, true));
    g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2, radius * 2);
    if (hi - lo > .004f || value > .004f) {
        const int sprite = juce::roundToInt(segMid);
        const float a0 = start + lo * (end - start), a1 = start + hi * (end - start);
        juce::Path wedge;
        wedge.addPieSegment(centre.x - segOuter - 12, centre.y - segOuter - 12, (segOuter + 12) * 2, (segOuter + 12) * 2,
                            a0 - .05f, a1 + .05f, 0.f);
        juce::Graphics::ScopedSaveState save(g);
        g.reduceClipRegion(wedge);
        g.setOpacity(juce::jlimit(0.f, 1.f, (o.active ? .72f : .6f) * bloomScale() * o.glow * (1.f + .5f * activity)));
        const auto& img = ringTinted(sprite, lit);
        g.drawImageAt(img, juce::roundToInt(centre.x) - img.getWidth() / 2, juce::roundToInt(centre.y) - img.getHeight() / 2, false);
    }
    // Hairline rings: outer sweep, inner sweep, and a closed inner circle.
    auto arc = [&](float r, float a, float b, juce::Colour col, float stroke) {
        juce::Path p; p.addCentredArc(centre.x, centre.y, r, r, 0, a, b, true);
        g.setColour(col); g.strokePath(p, juce::PathStrokeType(stroke));
    };
    arc(radius - .5f, start, end, colour(line).withAlpha(.7f), .6f);
    if (mid) {
        arc(segInner - 4.f, start, end, colour(line).withAlpha(.85f), .6f);
        if (big) arc(radius * .42f, 0, 2 * pi(), colour(line).withAlpha(.5f), .5f);
    }
    // Segmented ring.
    juce::Path onPath, offPath;
    for (int i = 0; i < bars; ++i) {
        const float f = float(i) / float(bars - 1), a = start + f * (end - start);
        const bool isOn = f >= lo - eps && f <= hi + eps && (hi - lo > .001f || i == juce::roundToInt(value * float(bars - 1)));
        (isOn ? onPath : offPath).addLineSegment({onCircle(centre, a, segInner), onCircle(centre, a, segOuter)}, segWidth);
    }
    g.setColour(colour(line).withAlpha(.9f + .1f * activity));
    g.fillPath(offPath);
    if (!onPath.isEmpty()) {
        const auto p0 = onCircle(centre, start + lo * (end - start), segMid), p1 = onCircle(centre, start + hi * (end - start), segMid);
        if (p0.getDistanceFrom(p1) > 2.f) g.setGradientFill(juce::ColourGradient(lit, p0.x, p0.y, litEnd, p1.x, p1.y, false));
        else g.setColour(litEnd);
        g.fillPath(onPath);
    }
    // Tick constellation inside the ring (large and medium dials).
    if (o.constellation && mid) {
        const float rc = segInner - 8.f;
        const int dots = big ? 40 : 24;
        juce::Path dotOn, dotOff;
        for (int i = 0; i <= dots; ++i) {
            const float f = float(i) / float(dots), a = start + f * (end - start);
            const auto p = onCircle(centre, a, rc);
            const bool isOn = f >= lo - .01f && f <= hi + .01f;
            (isOn ? dotOn : dotOff).addEllipse(p.x - (i % 5 == 0 ? .9f : .55f), p.y - (i % 5 == 0 ? .9f : .55f),
                                                (i % 5 == 0 ? 1.8f : 1.1f), (i % 5 == 0 ? 1.8f : 1.1f));
        }
        g.setColour(colour(textDim).withAlpha(.55f)); g.fillPath(dotOff);
        g.setColour(litEnd.withAlpha(.9f)); g.fillPath(dotOn);
    }
    if (o.peak >= 0.f) {
        const float a = start + juce::jlimit(0.f, 1.f, o.peak) * (end - start);
        g.setColour(colour(text));
        g.drawLine({onCircle(centre, a, segInner - 2.f), onCircle(centre, a, segOuter + 2.f)}, 1.6f);
    }
    if (o.pointer) {
        g.setColour(litEnd.withAlpha(.95f));
        g.drawLine({onCircle(centre, angle, radius * (big ? .3f : .22f)), onCircle(centre, angle, segInner - (mid ? 11.f : 2.f))}, big ? 1.4f : 1.1f);
        if (mid) {
            const auto tip = onCircle(centre, angle, segInner - 11.f);
            g.fillEllipse(tip.x - 1.7f, tip.y - 1.7f, 3.4f, 3.4f);
        }
    }
}
} // namespace
// ---- background ---------------------------------------------------------------------------
class BackdropLayer;
class Background::Impl : private juce::ComponentListener {
public:
    juce::Image base, glow;
    ThemeId builtId = ThemeId::Classic;
    Accent builtAccent = Accent::Cyan;
    bool builtLight = false;
    int pixelWidth = 0, pixelHeight = 0, builtKey = 0;
    bool builtWithPainter = false;
    StaticPainter* painter = nullptr;
    std::vector<juce::Rectangle<float>> panels;
    std::unique_ptr<juce::Component> layer;
    juce::Component::SafePointer<juce::Component> parent, canvas;
    ~Impl() override;
    // Keep the backdrop on top of the editor background and exactly under the canvas, at its scale.
    void follow(juce::Component& c) {
        canvas = &c;
        c.addComponentListener(this);
    }
    void componentMovedOrResized(juce::Component& c, bool, bool) override {
        if (layer != nullptr) {
            if (layer->getBounds() != c.getBounds()) layer->setBounds(c.getBounds());
            if (layer->getTransform() != c.getTransform()) layer->setTransform(c.getTransform());
        }
    }
    bool needsBuild(const juce::Component& c, int& w, int& h, float& scale) const {
        const auto transform = c.getTransform();
        scale = std::sqrt(std::abs(transform.getDeterminant()));
        w = juce::jmax(1, juce::roundToInt(float(c.getWidth()) * scale));
        h = juce::jmax(1, juce::roundToInt(float(c.getHeight()) * scale));
        return !base.isValid() || pixelWidth != w || pixelHeight != h || builtId != current || builtLight != lightMode ||
               (current == ThemeId::Quantum && builtAccent != accent) ||
               (painter != nullptr) != builtWithPainter || (painter != nullptr && painter->staticKey() != builtKey);
    }
    void build(const juce::Component& c, int w, int h, float scale);
    void paintLayer(juce::Graphics& g, juce::Component& c);
};
class BackdropLayer : public juce::Component {
public:
    explicit BackdropLayer(Background::Impl& o) : owner(o) {
        setInterceptsMouseClicks(false, false);
        setOpaque(true);
        setComponentID("dvlce-backdrop");
    }
    void paint(juce::Graphics& g) override { Scope scope(*this); owner.paintLayer(g, *this); }
    Background::Impl& owner;
};
Background::Impl::~Impl() {
    if (canvas != nullptr) canvas->removeComponentListener(this);
    if (layer != nullptr)
        if (auto* p = layer->getParentComponent()) p->removeChildComponent(layer.get());
}
namespace {
std::vector<float> contourField(int cols, int rows, int step) {
    std::vector<float> field(size_t(cols * rows));
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < cols; ++x) {
            const float px = float(x * step) * .011f, py = float(y * step) * .011f;
            const float u = px + 1.3f * std::sin(py * .8f + std::cos(px * .6f));
            const float v = py + 1.1f * std::cos(px * .7f + std::sin(py * .9f));
            field[size_t(y * cols + x)] = std::sin(u * 1.4f + v * .6f) + .7f * std::cos(v * 1.7f - u * .4f)
                                          + .28f * std::sin(u * 3.1f + std::cos(v * 2.2f));
        }
    return field;
}
juce::Path contourPath(const std::vector<float>& field, int cols, int rows, int step) {
    juce::Path contours;
    for (float level : {-.9f, -.45f, 0.f, .45f, .9f})
        for (int y = 0; y < rows - 1; ++y)
            for (int x = 0; x < cols - 1; ++x) {
                const juce::Point<float> points[]{{float(x * step), float(y * step)}, {float((x + 1) * step), float(y * step)},
                                                  {float((x + 1) * step), float((y + 1) * step)}, {float(x * step), float((y + 1) * step)}};
                const float values[]{field[size_t(y * cols + x)], field[size_t(y * cols + x + 1)],
                                     field[size_t((y + 1) * cols + x + 1)], field[size_t((y + 1) * cols + x)]};
                for (auto indices : {std::array<int, 3>{0, 1, 2}, std::array<int, 3>{0, 2, 3}}) {
                    juce::Point<float> hits[3]; int count = 0;
                    for (int e = 0; e < 3; ++e) {
                        const int i = indices[size_t(e)], j = indices[size_t((e + 1) % 3)];
                        if ((values[i] < level) != (values[j] < level)) {
                            const float t = (level - values[i]) / (values[j] - values[i]);
                            hits[count++] = points[i] + (points[j] - points[i]) * t;
                        }
                    }
                    if (count == 2) { contours.startNewSubPath(hits[0]); contours.lineTo(hits[1]); }
                }
            }
    return contours;
}
void starDust(juce::Graphics& g, float width, float height, float density, bool glintsOnly) {
    juce::Random random(0x44564c43);
    const int count = int(width * height / density);
    const auto soft = isLight() ? colour(text) : ice();
    for (int i = 0; i < count; ++i) {
        const float x = random.nextFloat() * width, y = random.nextFloat() * height, k = random.nextFloat();
        const auto tint = random.nextFloat() < .22f ? colour(purple) : soft;
        const float a = isLight() ? .55f : 1.f;
        if (glintsOnly) {
            // Brighter points for the breathing layer: the same positions as the dust, a sparse subset.
            if (k > .93f) {
                g.setColour(tint.withAlpha(.18f * a));
                g.fillEllipse(x - 2.4f, y - 2.4f, 4.8f, 4.8f);
                g.setColour(tint.withAlpha(.95f * a));
                g.fillEllipse(x - .9f, y - .9f, 1.8f, 1.8f);
                g.setColour(tint.withAlpha(.3f * a));
                g.drawLine(x - 4.5f, y, x + 4.5f, y, .5f);
                g.drawLine(x, y - 4.5f, x, y + 4.5f, .5f);
            }
            continue;
        }
        if (k < .86f) {
            g.setColour(tint.withAlpha((.14f + random.nextFloat() * .26f) * a));
            g.fillRect(x, y, .75f, .75f);
        } else if (k < .975f) {
            g.setColour(tint.withAlpha((.45f + random.nextFloat() * .3f) * a));
            g.fillEllipse(x - .6f, y - .6f, 1.4f, 1.4f);
        } else {
            g.setColour(tint.withAlpha(.14f * a));
            g.fillEllipse(x - 2.2f, y - 2.2f, 4.4f, 4.4f);
            g.setColour(tint.withAlpha(.85f * a));
            g.fillEllipse(x - .8f, y - .8f, 1.6f, 1.6f);
            g.setColour(tint.withAlpha(.22f * a));
            g.drawLine(x - 4.f, y, x + 4.f, y, .5f);
            g.drawLine(x, y - 4.f, x, y + 4.f, .5f);
        }
    }
}
} // namespace
void Background::Impl::build(const juce::Component& c, int w, int h, float scale) {
    ++backgroundBuilds;
    pixelWidth = w; pixelHeight = h; builtId = current; builtLight = lightMode; builtAccent = accent;
    builtKey = painter != nullptr ? painter->staticKey() : 0;
    builtWithPainter = painter != nullptr;
    const float width = float(c.getWidth()), height = float(c.getHeight());
    base = juce::Image(juce::Image::RGB, w, h, true, juce::SoftwareImageType());
    glow = juce::Image();
    const bool lightPalette = lightMode;
    {
        juce::Graphics bgGraphics(base);
        bgGraphics.addTransform(juce::AffineTransform::scale(scale));
        bgGraphics.fillAll(colour(bg));
        if (isSpectrum()) {
            bgGraphics.setGradientFill(juce::ColourGradient(colour(purple).withAlpha(lightPalette ? .12f : .13f), width * .05f, -height * .05f,
                                                            colour(purple).withAlpha(0.f), width * .75f, height * .55f, true));
            bgGraphics.fillAll();
            bgGraphics.setGradientFill(juce::ColourGradient(light().withAlpha(lightPalette ? .10f : .085f), width * 1.0f, height * 1.05f,
                                                            light().withAlpha(0.f), width * .3f, height * .45f, true));
            bgGraphics.fillAll();
            const int step = 4;
            const int cols = int(width) / step + 2, rows = int(height) / step + 2;
            const auto contours = contourPath(contourField(cols, rows, step), cols, rows, step);
            const auto violet = colour(purple), cyan = light();
            bgGraphics.setGradientFill(juce::ColourGradient(violet.withAlpha(.05f), 0, 0, cyan.withAlpha(.05f), width, height, false));
            bgGraphics.strokePath(contours, juce::PathStrokeType(5.f));
            bgGraphics.setGradientFill(juce::ColourGradient(violet.withAlpha(lightPalette ? .26f : .34f), 0, 0, cyan.withAlpha(lightPalette ? .24f : .30f), width, height, false));
            bgGraphics.strokePath(contours, juce::PathStrokeType(.7f));
            starDust(bgGraphics, width, height, 800.f, false);
            bgGraphics.setGradientFill(juce::ColourGradient(colour(bg).withAlpha(0.f), width * .5f, height * .5f,
                                                            (lightPalette ? colour(line) : colour(bg)).withAlpha(lightPalette ? .35f : .55f), width * .5f,
                                                            height * .5f - juce::jmax(width, height) * .62f, true));
            bgGraphics.fillAll();
        } else if (isRadiant()) {
            bgGraphics.setGradientFill(juce::ColourGradient(colour(bg).interpolatedWith(light(), lightPalette ? .12f : .055f), width * .5f, 0, colour(bg), width * .5f, height, true));
            bgGraphics.fillAll();
            starDust(bgGraphics, width, height, 1000.f, false);
        } else if (isQuantum()) {
            // Slate ground: top-lit, a faint instrument grid, an accent haze at the top and the partner colour low.
            bgGraphics.setGradientFill(juce::ColourGradient(lightPalette ? juce::Colour(0xfff1f4f8) : juce::Colour(0xff151c23), 0, 0,
                                                            lightPalette ? juce::Colour(0xffe2e8ee) : juce::Colour(0xff0c1015), 0, height, false));
            bgGraphics.fillAll();
            bgGraphics.setColour(colour(line).withAlpha(lightPalette ? .35f : .22f));
            for (float x = 12.f; x < width; x += 24.f) bgGraphics.fillRect(x, 0.f, 1.f, height);
            for (float y = 12.f; y < height; y += 24.f) bgGraphics.fillRect(0.f, y, width, 1.f);
            bgGraphics.setGradientFill(juce::ColourGradient(light().withAlpha(lightPalette ? .10f : .09f), width * .5f, -height * .1f,
                                                            light().withAlpha(0.f), width * .5f, height * .6f, true));
            bgGraphics.fillAll();
            bgGraphics.setGradientFill(juce::ColourGradient(colour(purple).withAlpha(lightPalette ? .05f : .05f), width, height,
                                                            colour(purple).withAlpha(0.f), width * .55f, height * .55f, true));
            bgGraphics.fillAll();
            bgGraphics.setGradientFill(juce::ColourGradient(colour(bg).withAlpha(0.f), width * .5f, height * .5f,
                                                            juce::Colours::black.withAlpha(lightPalette ? .0f : .35f), width * .5f,
                                                            height * .5f - juce::jmax(width, height) * .65f, true));
            bgGraphics.fillAll();
        }
        for (auto r : panels) panelShape(bgGraphics, r);
        if (painter != nullptr) painter->paintStatic(bgGraphics);
    }
    if (isSpectrum() || isRadiant() || isQuantum()) {
        // The breathing layer: stronger contours / glints and a stronger nebula, absent under panels.
        glow = juce::Image(juce::Image::ARGB, w, h, true, juce::SoftwareImageType());
        juce::Graphics glowGraphics(glow);
        glowGraphics.addTransform(juce::AffineTransform::scale(scale));
        for (auto r : panels) glowGraphics.excludeClipRegion(r.getSmallestIntegerContainer());
        if (isQuantum()) {
            // Quantum lights its grid at paint time (paintLayer): a blend of a full-size layer cost more than the grid.
            glow = juce::Image();
            return;
        }
        if (isSpectrum()) {
            glowGraphics.setGradientFill(juce::ColourGradient(colour(purple).withAlpha(.22f), width * .05f, -height * .05f,
                                                              colour(purple).withAlpha(0.f), width * .7f, height * .5f, true));
            glowGraphics.fillAll();
            glowGraphics.setGradientFill(juce::ColourGradient(light().withAlpha(.18f), width, height * 1.05f,
                                                              light().withAlpha(0.f), width * .35f, height * .5f, true));
            glowGraphics.fillAll();
            const int step = 4;
            const int cols = int(width) / step + 2, rows = int(height) / step + 2;
            const auto contours = contourPath(contourField(cols, rows, step), cols, rows, step);
            glowGraphics.setGradientFill(juce::ColourGradient(colour(purple).withAlpha(.55f), 0, 0, light().withAlpha(.5f), width, height, false));
            glowGraphics.strokePath(contours, juce::PathStrokeType(1.1f));
        }
        starDust(glowGraphics, width, height, isSpectrum() ? 800.f : 1000.f, true);
    }
}
void Background::Impl::paintLayer(juce::Graphics& g, juce::Component& c) {
    int w, h; float scale;
    if (needsBuild(c, w, h, scale)) build(c, w, h, scale);
    // At an exact 1:1 mapping an integer blit takes the fast path of the software renderer.
    const bool oneToOne = std::abs(g.getInternalContext().getPhysicalPixelScaleFactor() - 1.f) < 1e-4f &&
                          base.getWidth() == c.getWidth() && base.getHeight() == c.getHeight();
    if (oneToOne) g.drawImageAt(base, 0, 0);
    else g.drawImage(base, c.getLocalBounds().toFloat());
    if (builtId == ThemeId::Quantum && !reduceMotion && activity > .02f) {
        // Audio lights the instrument grid between the panels.
        for (auto r : panels) g.excludeClipRegion(r.getSmallestIntegerContainer());
        const float width = float(c.getWidth()), height = float(c.getHeight());
        g.setColour(light().withAlpha(juce::jlimit(0.f, 1.f, activity) * (lightMode ? .3f : .45f)));
        for (float x = 12.f; x < width; x += 24.f) g.fillRect(x, 0.f, 1.f, height);
        for (float y = 12.f; y < height; y += 24.f) g.fillRect(0.f, y, width, 1.f);
        return;
    }
    if (glow.isValid() && !reduceMotion && activity > .02f) {
        // The glow layer is empty under the panels: blend only the space between them.
        for (auto r : panels) g.excludeClipRegion(r.getSmallestIntegerContainer());
        g.setOpacity(juce::jlimit(0.f, 1.f, activity * (lightMode ? .55f : .85f)));
        if (oneToOne) g.drawImageAt(glow, 0, 0);
        else g.drawImage(glow, c.getLocalBounds().toFloat());
    }
}
Background::Background() : impl(std::make_unique<Impl>()) {}
Background::~Background() = default;
void Background::draw(juce::Graphics& g, const juce::Component& c, std::initializer_list<juce::Rectangle<float>> p, StaticPainter* painter) {
    if (classic()) return;
    impl->painter = painter;
    const bool panelsChanged = impl->panels.size() != p.size() || !std::equal(impl->panels.begin(), impl->panels.end(), p.begin());
    int w, h; float scale;
    if (panelsChanged) { impl->panels.assign(p); impl->base = juce::Image(); }
    if (impl->needsBuild(c, w, h, scale)) {
        impl->build(c, w, h, scale);
        if (impl->layer != nullptr) impl->layer->repaint();
    }
    if (impl->layer == nullptr) {
        // First paint: create the backdrop under the canvas, and paint it directly this once.
        if (auto* parent = c.getParentComponent()) {
            impl->layer = std::make_unique<BackdropLayer>(*impl);
            impl->parent = parent;
            parent->addAndMakeVisible(impl->layer.get(), 0);
            impl->layer->setBounds(c.getBounds());
            impl->layer->setTransform(c.getTransform());
            impl->follow(const_cast<juce::Component&>(c));
            if (auto* look = dynamic_cast<DvlceLookAndFeel*>(&c.getLookAndFeel())) look->setBackdrop(impl->layer.get());
        }
        g.drawImage(impl->base, c.getLocalBounds().toFloat());
        return;
    }
    if (impl->layer->getBounds() != c.getBounds()) impl->layer->setBounds(c.getBounds());
    if (impl->layer->getTransform() != c.getTransform()) impl->layer->setTransform(c.getTransform());
}
std::uint64_t hashText(const juce::String& t) { return std::uint64_t(t.hashCode64()); }
void PaintCache::draw(juce::Graphics& g, juce::Rectangle<int> bounds, std::uint64_t key, const std::function<void(juce::Graphics&)>& painter,
                      bool opaque) {
    const float scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    if (bounds.isEmpty() || scale <= 0.f || std::abs(scale - std::round(scale)) > .02f) { painter(g); return; }
    const int w = juce::roundToInt(float(bounds.getWidth()) * scale), h = juce::roundToInt(float(bounds.getHeight()) * scale);
    std::uint64_t k = mix(key, std::uint64_t(current));
    k = mix(k, (lightMode ? 7u : 3u) + 16u * std::uint64_t(accent));
    k = mix(k, std::uint64_t(w) * 65537u + std::uint64_t(h));
    k = mix(k, std::uint64_t(juce::roundToInt(activity * 5.f)));
    if (!valid || k != cachedKey || image.getWidth() != w || image.getHeight() != h) {
        image = juce::Image(opaque ? juce::Image::RGB : juce::Image::ARGB, w, h, !opaque, juce::SoftwareImageType());
        juce::Graphics cacheGraphics(image);
        cacheGraphics.addTransform(juce::AffineTransform::scale(scale));
        painter(cacheGraphics);
        cachedKey = k;
        valid = true;
    }
    g.drawImage(image, bounds.toFloat());
}
// ---- re-theming widgets that were configured with the previous look ------------------------------
namespace {
// Quantum uses two faces: Barlow below quantumTitleMin, Rajdhani at and above it.
float typeScale(ThemeId id, float height) {
    if (id == ThemeId::Quantum) return height / smallScale < quantumTitleMin ? smallScale : squareScale;
    return id == ThemeId::Classic || id == ThemeId::Vault ? 1.f : 1.1f;
}
juce::Colour mapColour(juce::Colour old, ThemeChoice from, ThemeChoice to) {
    const auto& a = designFor(from.id, from.light);
    const auto& b = designFor(to.id, to.light);
    const juce::uint32 rgb = old.withAlpha(1.f).getARGB();
    const juce::uint32 fromRoles[]{a.background, a.panel, a.surface, a.line, a.dim, a.text, a.selected, a.hot, a.light, a.ice};
    const juce::uint32 toRoles[]{b.background, b.panel, b.surface, b.line, b.dim, b.text, b.selected, b.hot, b.light, b.ice};
    for (size_t i = 0; i < 10; ++i)
        if ((fromRoles[i] | 0xff000000u) == rgb && i != 1)
            return juce::Colour(toRoles[i] | (i == 1 ? 0u : 0xff000000u)).withAlpha(old.getFloatAlpha());
    return old;
}
juce::Font mapFont(const juce::Font& old, ThemeChoice from, ThemeChoice to) {
    // Rounded to 1/100 px: 16.52 / 1.18 is 13.9999 in float, which would cross the 14 px face switch on a round trip.
    const float size = std::round(old.getHeight() / typeScale(from.id, old.getHeight()) * 100.f) / 100.f;
    Scope scope(to);
    return font(size, old.isBold());
}
void walk(juce::Component& c, ThemeChoice from, ThemeChoice to) {
    if (auto* l = dynamic_cast<juce::Label*>(&c)) {
        l->setFont(mapFont(l->getFont(), from, to));
        if (l->isColourSpecified(juce::Label::textColourId))
            l->setColour(juce::Label::textColourId, mapColour(l->findColour(juce::Label::textColourId), from, to));
    } else if (auto* e = dynamic_cast<juce::TextEditor*>(&c)) {
        e->applyFontToAllText(mapFont(e->getFont(), from, to));
        for (int id : {juce::TextEditor::textColourId, juce::TextEditor::backgroundColourId, juce::TextEditor::outlineColourId,
                       juce::TextEditor::focusedOutlineColourId, juce::TextEditor::highlightColourId, juce::TextEditor::highlightedTextColourId})
            if (e->isColourSpecified(id)) e->setColour(id, mapColour(e->findColour(id), from, to));
    } else if (auto* s = dynamic_cast<juce::ScrollBar*>(&c)) {
        for (int id : {juce::ScrollBar::thumbColourId, juce::ScrollBar::trackColourId, juce::ScrollBar::backgroundColourId})
            if (s->isColourSpecified(id)) s->setColour(id, mapColour(s->findColour(id), from, to));
    }
    for (auto* child : c.getChildren()) walk(*child, from, to);
}
} // namespace
void retheme(juce::Component& root, ThemeChoice from, ThemeChoice to) { walk(root, from, to); }
} // namespace Theme
} // namespace dvlce::ui
