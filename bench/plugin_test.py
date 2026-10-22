"""Built-plugin check: run the real VST3 on synthetic speakers and measure what each side does.

    python -I bench/plugin_test.py <path to the inner Dvlce Duet.vst3 binary>

Needs numpy and pedalboard.  On Windows pass the inner binary:
    ...\\Dvlce Duet.vst3\\Contents\\x86_64-win\\Dvlce Duet.vst3
Stereo pair mode only (pedalboard does not feed a sidechain bus).
"""
import sys

import numpy as np
from pedalboard import load_plugin

SR = 48000
failures = 0


def check(ok, what, value):
    global failures
    print(f"{'ok  ' if ok else 'FAIL'}  {what:<60} {value:.3f}")
    if not ok:
        failures += 1


def noise(n, rms_db, seed):
    rng = np.random.default_rng(seed)
    return (rng.uniform(-1, 1, n) * np.sqrt(3) * 10 ** (rms_db / 20)).astype(np.float32)


def seg(t0, t1):
    return slice(int(t0 * SR), int(t1 * SR))


def rms_db(x):
    return 10 * np.log10(np.mean(np.square(x.astype(np.float64))) + 1e-30)


def main():
    path = sys.argv[1]
    n = 3 * SR
    a = noise(n, -75, 1)
    b = noise(n, -75, 2)
    a[seg(0, 1)] += noise(SR, -20, 11)
    b[seg(1, 2)] += noise(SR, -20, 12)
    x = np.stack([a, b])

    p = load_plugin(path)
    p.mode = "Stereo pair"
    p.output = "Separate"
    y = p.process(x, SR, buffer_size=512, reset=True)

    # gain change = output RMS - input RMS over the steady middle of each turn
    def change(ch, t0, t1):
        return rms_db(y[ch][seg(t0, t1)]) - rms_db(x[ch][seg(t0, t1)])

    check(abs(change(0, 0.4, 0.9)) < 0.1, "A talking: A unchanged (dB)", change(0, 0.4, 0.9))
    check(abs(change(1, 0.4, 0.9) + 30) < 0.5, "A talking: B ducked 30 dB (dB)", change(1, 0.4, 0.9))
    check(abs(change(0, 1.5, 1.9) + 30) < 0.5, "B talking: A ducked 30 dB (dB)", change(0, 1.5, 1.9))
    check(abs(change(1, 1.4, 1.9)) < 0.1, "B talking: B unchanged (dB)", change(1, 1.4, 1.9))
    check(abs(change(0, 2.6, 2.9)) < 0.1, "nobody talking: A unchanged (dB)", change(0, 2.6, 2.9))

    p.output = "Mixed"
    y = p.process(x, SR, buffer_size=512, reset=True)
    check(np.array_equal(y[0], y[1]), "Mixed: left == right", float(np.max(np.abs(y[0] - y[1]))))
    expect = rms_db(x[0][seg(0.4, 0.9)])
    check(abs(rms_db(y[0][seg(0.4, 0.9)]) - expect) < 0.1, "Mixed: A's turn at A's level (dB)", rms_db(y[0][seg(0.4, 0.9)]) - expect)

    p.output = "Separate"
    p.depth = "Mute"
    y = p.process(x, SR, buffer_size=512, reset=True)
    check(np.max(np.abs(y[1][seg(0.4, 0.9)])) == 0.0, "depth Mute: B silent while A talks (peak)", float(np.max(np.abs(y[1][seg(0.4, 0.9)]))))

    print(f"\n{failures} FAILED" if failures else "\nall passed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
