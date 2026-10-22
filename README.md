# Dvlce Duet

A mutual ducking gate for two speakers (podcasts, interviews, dialogue). When one person talks, the other
person's mic drops by a set depth. Mic bleed (the other voice leaking into a mic) is rejected, so the
quiet mic stays ducked even though it hears the talker.

Formats: VST3 + Standalone (Windows, macOS), AU (macOS), AAX (Pro Tools, needs the AAX SDK: see below).

## Two ways to connect the speakers

| Mode | What goes in | Use it in |
|---|---|---|
| **Stereo pair** | One stereo track: left = speaker A, right = speaker B | Every host. The only way in Audacity and Premiere (no sidechain there). |
| **Sidechain** | This track = speaker A, sidechain input = speaker B | FL Studio, Pro Tools, REAPER. One instance per speaker, each keyed from the other. |

**Output** (Stereo pair only): *Separate* keeps A left and B right. *Mixed* puts both gated voices on both
channels, so the result is a finished centred dialogue track.

## Per host

- **Audacity (3.2 or later)**: put A's mono track above B's. On the top track's menu choose
  *Make Stereo Track*. Select it, then *Effect > Dvlce Duet*. Afterwards *Split Stereo to Mono* gives the two
  gated tracks back, or use Output = Mixed for one finished track. The first time, Audacity asks to enable
  the new plugin (or: *Effect > Plugin Manager*, enable Dvlce Duet).
- **FL Studio**: put Duet on both speakers' mixer tracks, mode Sidechain. On A's track, right-click B's
  route arrow > *Sidechain to this track*, and the other way round. In the plugin wrapper, set the
  sidechain input to that track (Processing tab, *Sidechain* / input port).
- **Pro Tools**: AAX only. Insert Duet on both tracks, mode Sidechain, set each instance's *key input* to a
  bus carrying the other speaker. A mono key is supported.
- **Premiere Pro**: select both clips, *Merge Clips* (or use a stereo clip with A left, B right), set the
  merged clip's channels to stereo L/R, then add Duet as a clip effect with Output = Mixed.

## Controls

| Control | Meaning | Default |
|---|---|---|
| Threshold A / B | Detector level (RMS dBFS) above which that speaker counts as talking | -40 dB |
| Depth | How far the silent speaker drops. Full right = mute | -30 dB |
| Bleed reject | A speaker this far under the other (each against its threshold) is bleed, not speech | 10 dB |
| Hysteresis | Talking ends only this far under the threshold | 3 dB |
| Open / Close | Time to open / duck across the full depth (linear ramp in dB) | 2 / 150 ms |
| Hold | Talking state survives pauses between words this long | 200 ms |
| Lookahead | Delays the audio so the gate opens before the first word. Adds this much latency | 0 ms |
| Detector HP | High-pass on the detector only (rumble, plosives) | 100 Hz |
| Overlap | Both talking: *Both open* passes the crosstalk, *Louder wins* ducks the quieter | Both open |
| Gate when idle | Nobody talking: duck both (a normal gate on top) | off |

Set each threshold just above that mic's noise floor, and under its quietest speech. Watch the meters:
the gold line is the threshold, the red bar is how far the speaker is ducked.

## Build

Windows: `build-win.bat` (VS 2022 Build Tools, CMake, Ninja). JUCE 8.0.13 is fetched. Optional env:
`JUCE_DIR`, `BUILD_DIR`, `AAX_SDK`.

AAX: get the AAX SDK from the Avid Developer Program, then `set AAX_SDK=<folder>` and build. An unsigned
AAX loads only in Pro Tools Developer. Retail Pro Tools needs the plugin signed with PACE `wraptool`
(a PACE/iLok developer account).

## Tests

- `dvlce_duet_core_tests` (built by `build-win.bat`): the engine on synthetic speakers, 18 checks.
- `DvlceDuetSidechainTest`: Sidechain mode on the real processor, mono key.
- `bench/plugin_test.py <inner .vst3 binary>`: the built VST3 through pedalboard, Stereo pair mode.
- pluginval strictness 10: pass.

Copyright 2026 Yuval Olsha. Private.
