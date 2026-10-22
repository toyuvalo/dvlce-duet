// Stereo splitter check: sample-exact halves, names, no-overwrite, refusals.
//   DvlceDuetSplitTest      (exit 0 = pass; works in a temp folder it deletes afterwards)
// Copyright 2026 Yuval Olsha.  PolyForm Noncommercial 1.0.0 (see LICENSE).
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_events/juce_events.h>

#include <cstdio>
#include <vector>

#include "StereoSplitter.h"

namespace {
int failures = 0;
void check(bool ok, const juce::String& what) {
    std::printf("%s  %s\n", ok ? "ok  " : "FAIL", what.toRawUTF8());
    if (!ok) ++failures;
}

// A stereo file with different random content on each side.  Integer formats get exact integer
// sample values (k / 2^(bits-1)); float files get arbitrary floats.
juce::AudioBuffer<float> makeSignal(int n, int bits, bool isFloat, int seed) {
    juce::Random rng(seed);
    juce::AudioBuffer<float> b(2, n);
    const double scale = std::ldexp(1.0, bits - 1);
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < n; ++i) {
            const double v = rng.nextDouble() * 1.8 - 0.9;
            b.setSample(c, i, isFloat ? static_cast<float>(v) : static_cast<float>(std::round(v * scale) / scale));
        }
    return b;
}

bool writeFile(const juce::File& f, const juce::AudioBuffer<float>& b, int bits, bool isFloat, int channels) {
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> out = std::make_unique<juce::FileOutputStream>(f);
    auto w = wav.createWriterFor(out, juce::AudioFormatWriterOptions{}
                                          .withSampleRate(48000.0)
                                          .withNumChannels(channels)
                                          .withBitsPerSample(bits)
                                          .withSampleFormat(isFloat ? juce::AudioFormatWriterOptions::SampleFormat::floatingPoint
                                                                    : juce::AudioFormatWriterOptions::SampleFormat::integral));
    return w != nullptr && w->writeFromAudioSampleBuffer(b, 0, b.getNumSamples());
}

// Number of samples where a mono file differs from one channel of the source FILE, compared as the raw
// integers (or float bits) the reader returns.  -1 = unreadable or the wrong shape / format.
long long mismatches(const juce::File& mono, const juce::File& source, int ch) {
    juce::AudioFormatManager m;
    m.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r(m.createReaderFor(mono)), s(m.createReaderFor(source));
    if (!r || !s || r->numChannels != 1 || s->numChannels != 2 || r->lengthInSamples != s->lengthInSamples ||
        r->bitsPerSample != s->bitsPerSample || r->usesFloatingPointData != s->usesFloatingPointData || r->sampleRate != s->sampleRate)
        return -1;
    const int n = static_cast<int>(s->lengthInSamples);
    std::vector<int> a(static_cast<size_t>(n)), l(static_cast<size_t>(n)), rr(static_cast<size_t>(n));
    int* one[1] = {a.data()};
    int* two[2] = {l.data(), rr.data()};
    r->read(one, 1, 0, n, false);
    s->read(two, 2, 0, n, false);
    const auto& want = ch == 0 ? l : rr;
    long long bad = 0;
    for (int i = 0; i < n; ++i) bad += a[static_cast<size_t>(i)] != want[static_cast<size_t>(i)];
    // and the two channels really differ, so a swapped split cannot pass
    long long same = 0;
    for (int i = 0; i < n; ++i) same += l[static_cast<size_t>(i)] == rr[static_cast<size_t>(i)];
    return same > n / 2 ? -2 : bad;
}
}  // namespace

int main() {
    juce::ScopedJuceInitialiser_GUI init;  // message manager for the format readers
    const juce::File dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                               .getChildFile("dvlce-duet-split-test-" + juce::String(juce::Time::currentTimeMillis()));
    dir.createDirectory();
    const int n = 48000 * 3 + 123;  // not a multiple of the 65536 block

    struct Case { int bits; bool isFloat; const char* name; };
    for (const Case c : {Case{24, false, "24-bit PCM"}, Case{16, false, "16-bit PCM"}, Case{32, true, "32-bit float"}}) {
        const auto sig = makeSignal(n, c.bits, c.isFloat, c.bits);
        const juce::File in = dir.getChildFile(juce::String("Episode 12 ") + c.name + ".wav");
        check(writeFile(in, sig, c.bits, c.isFloat, 2), juce::String(c.name) + ": wrote the stereo source");
        const auto r = duetsplit::split(in, "Host_", "Guest_");
        check(r.ok, juce::String(c.name) + ": split ok (" + r.message + ")");
        check(r.left.getFileName() == "Host_" + in.getFileName(), "left name = Host_<name>: " + r.left.getFileName());
        check(r.right.getFileName() == "Guest_" + in.getFileName(), "right name = Guest_<name>: " + r.right.getFileName());
        const auto ml = mismatches(r.left, in, 0), mr = mismatches(r.right, in, 1);
        check(ml == 0, juce::String(c.name) + ": left file == left channel, sample-exact (mismatches " + juce::String(ml) + ")");
        check(mr == 0, juce::String(c.name) + ": right file == right channel, sample-exact (mismatches " + juce::String(mr) + ")");
    }

    // Never overwrite: a second split of the same file gets _2 names, the first files are untouched.
    {
        const juce::File in = dir.getChildFile("Episode 12 24-bit PCM.wav");
        const juce::File first = dir.getChildFile("Host_Episode 12 24-bit PCM.wav");
        const auto before = first.getLastModificationTime();
        const auto size = first.getSize();
        const auto r = duetsplit::split(in, "Host_", "Guest_");
        check(r.ok && r.left.getFileName() == "Host_Episode 12 24-bit PCM_2.wav" &&
                  r.right.getFileName() == "Guest_Episode 12 24-bit PCM_2.wav",
              "second split -> _2 names: " + r.left.getFileName() + " / " + r.right.getFileName());
        check(first.getSize() == size && first.getLastModificationTime() == before, "first split files untouched");
    }

    // Refusals write nothing.
    {
        const int before = dir.getNumberOfChildFiles(juce::File::findFiles);
        const juce::File mono = dir.getChildFile("mono.wav");
        writeFile(mono, makeSignal(4800, 24, false, 7), 24, false, 1);
        const auto r1 = duetsplit::split(mono, "L_", "R_");
        check(!r1.ok && r1.message.contains("stereo"), "mono input refused: " + r1.message);
        const juce::File in = dir.getChildFile("Episode 12 16-bit PCM.wav");
        check(!duetsplit::split(in, "", "R_").ok, "empty prefix refused");
        check(!duetsplit::split(in, "X_", "X_").ok, "equal prefixes refused");
        check(!duetsplit::split(in, "a/b", "R_").ok, "prefix with / refused");
        const juce::File txt = dir.getChildFile("notes.txt");
        txt.replaceWithText("not audio");
        check(!duetsplit::split(txt, "L_", "R_").ok, "non-audio file refused");
        check(dir.getNumberOfChildFiles(juce::File::findFiles) == before + 2, "refusals wrote no audio files");
    }

    dir.deleteRecursively();
    std::printf(failures ? "\n%d FAILED\n" : "\nall passed\n", failures);
    return failures ? 1 : 0;
}
