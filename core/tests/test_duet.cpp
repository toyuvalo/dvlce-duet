// Dvlce Duet - engine tests.  Synthetic speakers: deterministic noise bursts at known levels.
// Copyright 2026 Yuval Olsha.  PolyForm Noncommercial 1.0.0 (see LICENSE).
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "duet/duet.h"

namespace {
int failures = 0;
constexpr double kSr = 48000.0;

void check(bool ok, const char* what, double value) {
    std::printf("%s  %-62s %.4f\n", ok ? "ok  " : "FAIL", what, value);
    if (!ok) ++failures;
}

struct Noise {
    uint32_t s;
    explicit Noise(uint32_t seed) : s(seed) {}
    float next() {  // uniform in [-1, 1): RMS = 1/sqrt3
        s = s * 1664525u + 1013904223u;
        return static_cast<float>(s >> 8) / 8388608.0f - 1.0f;
    }
};

// A segment of "speech": noise at an RMS level in dBFS.
void fill(std::vector<float>& v, int from, int to, float rmsDb, uint32_t seed) {
    Noise n(seed);
    const float a = std::pow(10.0f, rmsDb / 20.0f) * std::sqrt(3.0f);
    for (int i = from; i < to; ++i) v[static_cast<std::size_t>(i)] += a * n.next();
}

int ms(double t) { return static_cast<int>(t * 0.001 * kSr); }
double db(float g) { return 20.0 * std::log10(std::max(g, 1e-9f)); }

struct Run {
    std::vector<float> gA, gB;
};

Run run(const duet::Params& p, const std::vector<float>& a, const std::vector<float>& b, int block = 256) {
    duet::Engine e;
    e.setParams(p);
    e.prepare(kSr);
    const int n = static_cast<int>(a.size());
    Run r{std::vector<float>(a.size()), std::vector<float>(a.size())};
    for (int i = 0; i < n; i += block) {
        const int m = std::min(block, n - i);
        e.process(a.data() + i, b.data() + i, r.gA.data() + i, r.gB.data() + i, m);
    }
    return r;
}

std::vector<float> floorNoise(int n, uint32_t seed) {
    std::vector<float> v(static_cast<std::size_t>(n), 0.0f);
    fill(v, 0, n, -75.0f, seed);  // mic self-noise, well under threshold
    return v;
}
}  // namespace

int main() {
    const duet::Params p;  // defaults: thr -40, depth 30, bleed 10, open 2, hold 200, close 150

    // 1. Turn-taking: A talks 0..1 s, B talks 1..2 s, nobody 2..3 s.
    {
        const int n = ms(3000);
        auto a = floorNoise(n, 1), b = floorNoise(n, 2);
        fill(a, 0, ms(1000), -20.0f, 11);
        fill(b, ms(1000), ms(2000), -20.0f, 12);
        const Run r = run(p, a, b);
        check(std::abs(db(r.gA[ms(500)])) < 0.1, "A talking: A open (dB)", db(r.gA[ms(500)]));
        check(std::abs(db(r.gB[ms(500)]) + 30.0) < 0.5, "A talking: B ducked 30 dB (dB)", db(r.gB[ms(500)]));
        check(db(r.gB[ms(1010)]) > -1.0, "B starts: B open within 10 ms (dB)", db(r.gB[ms(1010)]));
        check(std::abs(db(r.gA[ms(1700)]) + 30.0) < 0.5, "B talking: A ducked 30 dB (dB)", db(r.gA[ms(1700)]));
        check(std::abs(db(r.gA[ms(2800)])) < 0.1, "nobody talking: A open (dB)", db(r.gA[ms(2800)]));
        check(std::abs(db(r.gB[ms(2800)])) < 0.1, "nobody talking: B open (dB)", db(r.gB[ms(2800)]));
        // the same result whatever the host block size
        const Run r2 = run(p, a, b, 37);
        float worst = 0;
        for (std::size_t i = 0; i < r.gA.size(); ++i)
            worst = std::max({worst, std::abs(r.gA[i] - r2.gA[i]), std::abs(r.gB[i] - r2.gB[i])});
        check(worst == 0.0f, "block size 256 vs 37: identical gains (max diff)", worst);
    }

    // 2. Bleed: A talks at -20 dB and leaks into B's mic at -35 dB (15 dB down).  B must stay ducked.
    {
        const int n = ms(1000);
        auto a = floorNoise(n, 3), b = floorNoise(n, 4);
        fill(a, 0, n, -20.0f, 13);
        fill(b, 0, n, -35.0f, 13);  // same seed: the same voice, quieter
        const Run r = run(p, a, b);
        check(std::abs(db(r.gB[ms(700)]) + 30.0) < 0.5, "bleed 15 dB down: B ducked (dB)", db(r.gB[ms(700)]));
        check(std::abs(db(r.gA[ms(700)])) < 0.1, "bleed 15 dB down: A open (dB)", db(r.gA[ms(700)]));
    }

    // 3. Overlap: both talk at nearly the same level.
    {
        const int n = ms(1000);
        auto a = floorNoise(n, 5), b = floorNoise(n, 6);
        fill(a, 0, n, -20.0f, 14);
        fill(b, 0, n, -26.0f, 15);
        const Run r = run(p, a, b);
        check(std::abs(db(r.gA[ms(700)])) < 0.1 && std::abs(db(r.gB[ms(700)])) < 0.1,
              "overlap, Both open: B open (dB)", db(r.gB[ms(700)]));
        duet::Params q = p;
        q.overlap = duet::Overlap::LouderWins;
        const Run s = run(q, a, b);
        check(std::abs(db(s.gB[ms(700)]) + 30.0) < 0.5, "overlap, Louder wins: quieter B ducked (dB)", db(s.gB[ms(700)]));
        check(std::abs(db(s.gA[ms(700)])) < 0.1, "overlap, Louder wins: A open (dB)", db(s.gA[ms(700)]));
    }

    // 4. Gate when idle + mute depth.
    {
        const int n = ms(1000);
        auto a = floorNoise(n, 7), b = floorNoise(n, 8);
        duet::Params q = p;
        q.gateWhenIdle = true;
        q.depthDb = duet::kMuteDb;
        const Run r = run(q, a, b);
        check(r.gA[ms(900)] == 0.0f && r.gB[ms(900)] == 0.0f, "idle gate, depth = mute: both silent (gain)", r.gA[ms(900)]);
        fill(a, ms(500), n, -20.0f, 16);
        const Run s = run(q, a, b);
        check(std::abs(db(s.gA[ms(900)])) < 0.1, "idle gate: A opens when A talks (dB)", db(s.gA[ms(900)]));
    }

    // 5. Hold: a 200 ms pause inside A's speech keeps B ducked (hold 300 ms).  Threshold -25 dB, so the
    //    detector falls below it ~40 ms into the pause and only the hold keeps A talking.
    {
        const int n = ms(1500);
        auto a = floorNoise(n, 9), b = floorNoise(n, 10);
        fill(a, 0, ms(600), -20.0f, 17);
        fill(a, ms(800), n, -20.0f, 18);
        duet::Params q = p;
        q.thresholdADb = q.thresholdBDb = -25.0f;
        q.holdMs = 300.0f;
        const Run r = run(q, a, b);
        check(std::abs(db(r.gB[ms(790)]) + 30.0) < 0.5, "hold: B stays ducked through A's 200 ms pause (dB)", db(r.gB[ms(790)]));
        q.holdMs = 0.0f;
        const Run s = run(q, a, b);
        check(db(s.gB[ms(790)]) > -1.0, "no hold: B opens in A's 200 ms pause (dB)", db(s.gB[ms(790)]));
    }

    // 6. Lookahead delay: latency and an impulse.
    {
        duet::Params q = p;
        q.lookaheadMs = 5.0f;
        duet::Engine e;
        e.setParams(q);
        e.prepare(kSr);
        check(e.latencySamples() == 240, "lookahead 5 ms at 48 kHz = 240 samples", e.latencySamples());
        duet::Delay d;
        d.prepare(ms(duet::kMaxLookaheadMs));
        d.setDelay(240);
        std::vector<float> x(1000, 0.0f);
        x[10] = 1.0f;
        for (int i = 0; i < 1000; i += 64) d.process(x.data() + i, std::min(64, 1000 - i));
        int at = -1;
        for (int i = 0; i < 1000; ++i)
            if (x[static_cast<std::size_t>(i)] == 1.0f) at = i;
        check(at == 250, "delay 240: impulse at 10 arrives at 250", at);
    }

    std::printf(failures ? "\n%d FAILED\n" : "\nall passed\n", failures);
    return failures ? 1 : 0;
}
