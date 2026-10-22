// Dvlce Duet - stereo file splitter (the built-in mono-izer "Split L / R"): one stereo file in,
// <leftprefix><name>.<ext> (left channel) and <rightprefix><name>.<ext> (right channel) out.
// Copyright 2026 Yuval Olsha.  PolyForm Noncommercial 1.0.0 (see LICENSE).
#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

namespace duetsplit {

struct Result {
    bool ok = false;
    juce::String message;  // error text, or a one-line summary when ok
    juce::File left, right;
};

// Empty string = the two prefixes are usable.
juce::String checkPrefixes(const juce::String& left, const juce::String& right);

// First free name: <dir>/<stem><ext>, then <stem>_2<ext>, <stem>_3<ext> ...  Never an existing file.
juce::File freeName(const juce::File& dir, const juce::String& stem, const juce::String& ext);

// Sample-exact for every PCM and float format the reader and writer share (WAV, AIFF, FLAC).
// Both files are written or neither: a failure removes what was written.
Result split(const juce::File& input, const juce::String& leftPrefix, const juce::String& rightPrefix);

}  // namespace duetsplit
