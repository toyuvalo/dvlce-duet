// Dvlce Duet - mutual ducking gate for two speakers.  No JUCE: the plugin and the tests share this engine.
// Copyright 2026 Yuval Olsha.  PolyForm Noncommercial 1.0.0 (see LICENSE).
//
// Two detector signals in (speaker A, speaker B), two gain curves out.  When one speaker talks the other
// is ducked by `depthDb`.  The detector sees the undelayed input; the plugin delays the audio by
// latencySamples() (lookahead), so a gain opens before the word it belongs to reaches the output.
#pragma once

#include <cstdint>
#include <vector>

namespace duet {

enum class Overlap { BothOpen = 0, LouderWins = 1 };
enum class Floor { None = 0, A = 1, B = 2 };

struct Params {
    float thresholdADb = -40.0f;  // detector level (RMS dBFS) at which A counts as talking
    float thresholdBDb = -40.0f;
    float hysteresisDb = 3.0f;    // a speaker stops counting as talking this far below threshold (after hold)
    float bleedDb = 10.0f;        // a speaker this far below the other (relative to threshold) is mic bleed, not speech
    float depthDb = 30.0f;        // how far the ducked speaker drops; >= kMuteDb = silence
    float openMs = 2.0f;          // time to open across the full depth (a linear ramp in dB)
    float holdMs = 200.0f;        // talking state survives this long after the level falls
    float closeMs = 150.0f;       // time to duck across the full depth
    float detectorHpHz = 100.0f;  // detector high-pass (rumble, plosives, desk bumps)
    Overlap overlap = Overlap::BothOpen;
    bool gateWhenIdle = false;    // nobody talking: duck both
    float lookaheadMs = 0.0f;     // 0 .. kMaxLookaheadMs
};

constexpr float kMuteDb = 80.0f;
constexpr float kMaxLookaheadMs = 20.0f;

// Delay line for the audio path (lookahead).  Delay 0 = pass-through.
class Delay {
public:
    void prepare(int maxDelay);
    void setDelay(int samples);
    void reset();
    void process(float* x, int n);

private:
    std::vector<float> ring;
    int pos = 0, delay = 0;
};

class Engine {
public:
    void prepare(double sampleRate);
    void reset();
    void setParams(const Params& p);
    int latencySamples() const { return lookahead; }

    // detA, detB: n detector samples each.  gainA, gainB: n linear gains to apply to the delayed audio.
    void process(const float* detA, const float* detB, float* gainA, float* gainB, int n);

    // Last values of the most recent block (for meters).
    float levelADb() const { return lastLevel[0]; }
    float levelBDb() const { return lastLevel[1]; }
    float gainA() const { return gain[0]; }
    float gainB() const { return gain[1]; }
    bool talkingA() const { return active[0]; }
    bool talkingB() const { return active[1]; }
    Floor floor() const { return floorOwner; }

private:
    struct Biquad {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        float run(float x);
    };
    void updateCoefficients();

    double sr = 48000.0;
    Params params;
    Biquad hp[2];
    double env[2] = {0, 0};
    double envAtt = 0, envRel = 0;
    bool held[2] = {false, false};
    int holdLeft[2] = {0, 0};
    int holdSamples = 0;
    bool active[2] = {false, false};
    float lastLevel[2] = {-200.0f, -200.0f};
    float gain[2] = {1.0f, 1.0f};
    float gainDb[2] = {0.0f, 0.0f};
    float rangeDb = 30.0f, openStep = 0, closeStep = 0;
    Floor floorOwner = Floor::None;
    int lookahead = 0;
};

}  // namespace duet
