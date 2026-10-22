// Dvlce Duet - engine.
// Copyright 2026 Yuval Olsha.
#include "duet/duet.h"

#include <algorithm>
#include <cmath>

namespace duet {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kEnvAttackMs = 2.0;    // detector envelope: fast rise
constexpr double kEnvReleaseMs = 40.0;  // and a slower fall, so one word reads as one burst
constexpr float kLouderMarginDb = 3.0f; // LouderWins: the floor changes hands only past this margin

// gain for a dB value; the full mute depth is true silence
float dbToGain(float db) { return db <= -kMuteDb ? 0.0f : std::pow(10.0f, db / 20.0f); }

double coef(double ms, double sr) { return ms <= 0.0 ? 0.0 : std::exp(-1.0 / (ms * 0.001 * sr)); }
}  // namespace

// ---- Delay ----
void Delay::prepare(int maxDelay) {
    ring.assign(static_cast<std::size_t>(std::max(maxDelay, 0) + 1), 0.0f);
    pos = 0;
    delay = std::min(delay, maxDelay);
}

void Delay::setDelay(int samples) {
    const int d = std::clamp(samples, 0, static_cast<int>(ring.size()) - 1);
    if (d != delay) {
        delay = d;
        reset();
    }
}

void Delay::reset() {
    std::fill(ring.begin(), ring.end(), 0.0f);
    pos = 0;
}

void Delay::process(float* x, int n) {
    if (delay == 0) return;
    const int size = delay + 1;
    for (int i = 0; i < n; ++i) {
        ring[static_cast<std::size_t>(pos)] = x[i];
        int r = pos - delay;
        if (r < 0) r += size;
        x[i] = ring[static_cast<std::size_t>(r)];
        if (++pos == size) pos = 0;
    }
}

// ---- Engine ----
float Engine::Biquad::run(float x) {
    const double y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2;
    z2 = b2 * x - a2 * y;
    return static_cast<float>(y);
}

void Engine::prepare(double sampleRate) {
    sr = sampleRate > 0 ? sampleRate : 48000.0;
    updateCoefficients();
    reset();
}

void Engine::reset() {
    for (int c = 0; c < 2; ++c) {
        hp[c].z1 = hp[c].z2 = 0;
        env[c] = 0;
        held[c] = false;
        holdLeft[c] = 0;
        active[c] = false;
        lastLevel[c] = -200.0f;
        gainDb[c] = params.gateWhenIdle ? -rangeDb : 0.0f;
        gain[c] = dbToGain(gainDb[c]);
    }
    floorOwner = Floor::None;
}

void Engine::setParams(const Params& p) {
    params = p;
    updateCoefficients();
}

void Engine::updateCoefficients() {
    // 2nd-order Butterworth high-pass (RBJ cookbook, Q = 1/sqrt2)
    const double f = std::clamp(static_cast<double>(params.detectorHpHz), 10.0, 0.45 * sr);
    const double w = 2.0 * kPi * f / sr, cw = std::cos(w), alpha = std::sin(w) / (2.0 * 0.7071067811865476);
    const double a0 = 1.0 + alpha;
    for (auto& h : hp) {
        h.b0 = (1.0 + cw) / 2.0 / a0;
        h.b1 = -(1.0 + cw) / a0;
        h.b2 = h.b0;
        h.a1 = -2.0 * cw / a0;
        h.a2 = (1.0 - alpha) / a0;
    }
    envAtt = coef(kEnvAttackMs, sr);
    envRel = coef(kEnvReleaseMs, sr);
    holdSamples = static_cast<int>(std::lround(std::max(params.holdMs, 0.0f) * 0.001 * sr));
    rangeDb = std::clamp(params.depthDb, 0.0f, kMuteDb);
    const auto step = [&](float ms) { return ms <= 0.0f ? kMuteDb : rangeDb / static_cast<float>(ms * 0.001 * sr); };
    openStep = step(params.openMs);
    closeStep = step(params.closeMs);
    lookahead = static_cast<int>(std::lround(std::clamp(params.lookaheadMs, 0.0f, kMaxLookaheadMs) * 0.001 * sr));
}

void Engine::process(const float* detA, const float* detB, float* gainA, float* gainB, int n) {
    const float thr[2] = {params.thresholdADb, params.thresholdBDb};
    const float* det[2] = {detA, detB};
    float* out[2] = {gainA, gainB};
    float level[2] = {lastLevel[0], lastLevel[1]};

    for (int i = 0; i < n; ++i) {
        for (int c = 0; c < 2; ++c) {
            const double x = hp[c].run(det[c] ? det[c][i] : 0.0f);
            const double e = x * x;
            env[c] = e + (e > env[c] ? envAtt : envRel) * (env[c] - e);
            level[c] = static_cast<float>(10.0 * std::log10(env[c] + 1e-20));
            if (level[c] > thr[c]) {
                held[c] = true;
                holdLeft[c] = holdSamples;
            } else if (level[c] < thr[c] - params.hysteresisDb) {
                if (holdLeft[c] > 0)
                    --holdLeft[c];
                else
                    held[c] = false;
            }
        }
        // Bleed: while the other speaker is above threshold, a speaker far below them (each relative to
        // its own threshold) is the other voice leaking into this mic.  That also ends a hold early, so
        // a reply is not cut off by the previous speaker's hold.
        const float margin[2] = {level[0] - thr[0], level[1] - thr[1]};
        for (int c = 0; c < 2; ++c) {
            const int o = 1 - c;
            const bool bleed = level[o] > thr[o] && margin[c] < margin[o] - params.bleedDb;
            active[c] = held[c] && !bleed;
        }

        float target[2] = {0.0f, 0.0f};  // dB
        if (active[0] && !active[1]) {
            floorOwner = Floor::A;
        } else if (active[1] && !active[0]) {
            floorOwner = Floor::B;
        } else if (active[0] && active[1]) {
            if (params.overlap == Overlap::BothOpen)
                floorOwner = Floor::None;
            else if (margin[0] > margin[1] + kLouderMarginDb)
                floorOwner = Floor::A;
            else if (margin[1] > margin[0] + kLouderMarginDb)
                floorOwner = Floor::B;
            // within the margin the floor stays where it was
        } else {
            floorOwner = Floor::None;
            if (params.gateWhenIdle) target[0] = target[1] = -rangeDb;
        }
        if (floorOwner == Floor::A) target[1] = -rangeDb;
        if (floorOwner == Floor::B) target[0] = -rangeDb;

        for (int c = 0; c < 2; ++c) {
            if (gainDb[c] != target[c]) {
                gainDb[c] = gainDb[c] < target[c] ? std::min(target[c], gainDb[c] + openStep)
                                                  : std::max(target[c], gainDb[c] - closeStep);
                gain[c] = dbToGain(gainDb[c]);
            }
            if (out[c]) out[c][i] = gain[c];
        }
    }
    lastLevel[0] = level[0];
    lastLevel[1] = level[1];
}

}  // namespace duet
