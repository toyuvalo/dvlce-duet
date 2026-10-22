// Dvlce Duet - stereo file splitter.
// Copyright 2026 Yuval Olsha.  PolyForm Noncommercial 1.0.0 (see LICENSE).
#include "StereoSplitter.h"

namespace duetsplit {

juce::String checkPrefixes(const juce::String& left, const juce::String& right) {
    if (left.isEmpty() || right.isEmpty()) return "Enter a prefix for both sides.";
    if (left == right) return "The two prefixes must be different, or the files would have the same name.";
    for (const auto& p : {left, right})
        if (p.containsAnyOf("\\/:*?\"<>|")) return "A prefix cannot contain any of these characters:  \\ / : * ? \" < > |";
    return {};
}

juce::File freeName(const juce::File& dir, const juce::String& stem, const juce::String& ext) {
    juce::File f = dir.getChildFile(stem + ext);
    for (int n = 2; f.exists(); ++n) f = dir.getChildFile(stem + "_" + juce::String(n) + ext);
    return f;
}

namespace {
std::unique_ptr<juce::AudioFormatWriter> makeWriter(juce::AudioFormat& format, const juce::File& file,
                                                    const juce::AudioFormatReader& reader) {
    std::unique_ptr<juce::OutputStream> out = std::make_unique<juce::FileOutputStream>(file);
    if (!static_cast<juce::FileOutputStream&>(*out).openedOk()) return nullptr;
    std::unordered_map<juce::String, juce::String> meta;
    for (int i = 0; i < reader.metadataValues.size(); ++i)
        meta[reader.metadataValues.getAllKeys()[i]] = reader.metadataValues.getAllValues()[i];
    const auto opts = juce::AudioFormatWriterOptions{}
                          .withSampleRate(reader.sampleRate)
                          .withNumChannels(1)
                          .withBitsPerSample(static_cast<int>(reader.bitsPerSample))
                          .withSampleFormat(reader.usesFloatingPointData
                                                ? juce::AudioFormatWriterOptions::SampleFormat::floatingPoint
                                                : juce::AudioFormatWriterOptions::SampleFormat::integral)
                          .withMetadataValues(meta);
    return format.createWriterFor(out, opts);
}
}  // namespace

Result split(const juce::File& input, const juce::String& leftPrefix, const juce::String& rightPrefix) {
    Result r;
    if (const auto bad = checkPrefixes(leftPrefix, rightPrefix); bad.isNotEmpty()) {
        r.message = bad;
        return r;
    }
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    auto* format = formats.findFormatForFileExtension(input.getFileExtension());
    std::unique_ptr<juce::AudioFormatReader> reader(format ? formats.createReaderFor(input) : nullptr);
    if (reader == nullptr) {
        r.message = input.getFileName() + ": not a WAV, AIFF or FLAC file this can read (for MP3 use mono-izer).";
        return r;
    }
    if (reader->numChannels != 2) {
        r.message = input.getFileName() + " has " + juce::String(reader->numChannels) + " channel(s); split needs a stereo file.";
        return r;
    }

    const juce::File dir = input.getParentDirectory();
    const juce::String stem = input.getFileNameWithoutExtension(), ext = input.getFileExtension();
    r.left = freeName(dir, leftPrefix + stem, ext);
    r.right = freeName(dir, rightPrefix + stem, ext);
    if (r.left == r.right) r.right = freeName(dir, rightPrefix + stem + "_R", ext);  // cannot happen with distinct prefixes

    auto fail = [&r](const juce::String& msg) {
        r.ok = false;
        r.message = msg;
        r.left.deleteFile();
        r.right.deleteFile();
        return r;
    };
    {
        auto wl = makeWriter(*format, r.left, *reader);
        auto wr = makeWriter(*format, r.right, *reader);
        if (wl == nullptr || wr == nullptr) {
            wl.reset();
            wr.reset();
            return fail("Could not write " + r.left.getFileName() + " / " + r.right.getFileName() +
                        " (folder read-only, or this format and bit depth cannot be written).");
        }
        // The integer path is sample-exact: ints are left-justified 32-bit; float files pass their
        // float bits through the same buffers, and the float writer reads them back as floats.
        constexpr int block = 65536;
        juce::HeapBlock<int> l(block), rr(block);
        for (juce::int64 pos = 0; pos < reader->lengthInSamples; pos += block) {
            const int n = static_cast<int>(juce::jmin<juce::int64>(block, reader->lengthInSamples - pos));
            int* dest[2] = {l.get(), rr.get()};
            if (!reader->read(dest, 2, pos, n, false)) return fail("Read error in " + input.getFileName());
            const int* one[2] = {nullptr, nullptr};
            one[0] = l.get();
            if (!wl->write(one, n)) return fail("Write error in " + r.left.getFileName());
            one[0] = rr.get();
            if (!wr->write(one, n)) return fail("Write error in " + r.right.getFileName());
        }
    }  // writers flush and close here
    r.ok = true;
    r.message = "Split " + input.getFileName() + " -> " + r.left.getFileName() + " + " + r.right.getFileName();
    return r;
}

}  // namespace duetsplit
