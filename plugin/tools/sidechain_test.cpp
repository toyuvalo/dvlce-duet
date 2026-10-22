// Sidechain mode check on the real processor: main = speaker A, sidechain = speaker B.
//   DvlceDuetSidechainTest      (exit 0 = pass)
// Copyright 2026 Yuval Olsha.  PolyForm Noncommercial 1.0.0 (see LICENSE).
#include <juce_audio_processors/juce_audio_processors.h>

#include <cmath>
#include <cstdio>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

namespace {
int failures = 0;
void check(bool ok, const char* what, double v) {
    std::printf("%s  %-60s %.3f\n", ok ? "ok  " : "FAIL", what, v);
    if (!ok) ++failures;
}
}  // namespace

int main() {
    juce::ScopedJuceInitialiser_GUI init;
    std::unique_ptr<juce::AudioProcessor> proc(createPluginFilter());
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add(juce::AudioChannelSet::stereo());
    layout.inputBuses.add(juce::AudioChannelSet::mono());  // Pro Tools gives a mono key
    layout.outputBuses.add(juce::AudioChannelSet::stereo());
    if (!proc->setBusesLayout(layout)) {
        std::printf("FAIL  layout stereo + mono sidechain refused\n");
        return 1;
    }
    constexpr double sr = 48000.0;
    constexpr int block = 480, n = 3 * 48000;
    proc->prepareToPlay(sr, block);
    if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(proc->getParameters()[0]))  // mode = Sidechain
        p->setValueNotifyingHost(1.0f);

    // A (main, both channels) talks 0..1 s; B (sidechain) talks 1..2 s; then silence.  -20 dB noise.
    juce::Random rng(7);
    const auto speech = [&](int i, int from, int to) {
        return (i >= from && i < to ? 0.1f * std::sqrt(3.0f) : 0.0f) * (rng.nextFloat() * 2.0f - 1.0f);
    };
    double inA[3] = {}, outA[3] = {};
    juce::AudioBuffer<float> buf(3, block);
    juce::MidiBuffer midi;
    for (int pos = 0; pos < n; pos += block) {
        for (int i = 0; i < block; ++i) {
            const int t = pos + i;
            const float a = speech(t, 0, 48000) + 1e-4f * (rng.nextFloat() - 0.5f);
            buf.setSample(0, i, a);
            buf.setSample(1, i, a);
            buf.setSample(2, i, speech(t, 48000, 96000));
        }
        double before = 0;
        for (int i = 0; i < block; ++i) before += double(buf.getSample(0, i)) * buf.getSample(0, i);
        proc->processBlock(buf, midi);
        double after = 0;
        for (int i = 0; i < block; ++i) after += double(buf.getSample(0, i)) * buf.getSample(0, i);
        const int sec = pos / 48000;
        if (pos % 48000 >= 24000) {  // the steady second half of each second
            inA[sec] += before;
            outA[sec] += after;
        }
    }
    const auto change = [&](int s) { return 10.0 * std::log10(outA[s] / inA[s]); };
    check(std::abs(change(0)) < 0.1, "A talking: A unchanged (dB)", change(0));
    check(std::abs(change(1) + 30.0) < 0.5, "key (B) talking: A ducked 30 dB (dB)", change(1));
    check(std::abs(change(2)) < 0.1, "nobody talking: A unchanged (dB)", change(2));
    std::printf(failures ? "\n%d FAILED\n" : "\nall passed\n", failures);
    proc.reset();
    return failures ? 1 : 0;
}
