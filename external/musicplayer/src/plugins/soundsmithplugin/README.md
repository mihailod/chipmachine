# soundsmithplugin

Plays **Apple IIgs SoundSmith** music — the dominant IIgs tracker (Huibert
Aalbers, 1989), which drives the Ensoniq 5503 "DOC" digital oscillator chip
(32 oscillators / 16 stereo pairs, 64KB of sound RAM).

## Format: a song + a wavebank

On Modland each tune is a **pair** of files:

| File | Contents | Modland name |
|------|----------|--------------|
| song | patterns, order list, effects | bare name, e.g. `Bunny Tune` |
| wavebank | 64KB of DOC sound RAM + instrument/tuning tables | `Bunny Tune.W` (the `.W` = **W**avebank) |

The **song** is the playable entry; the **`.W` wavebank** is its sample-data
dependency.

* `canHandle()` identifies the song by its **header structure**, not a magic:
  the leading 6-byte signature varies per editor build (`SONGOK`, `IAN9OK`,
  `IAN92a`, …), so instead it checks that `blockLen` (LE16 @ offset 6) is a
  nonzero multiple of 896 (= 64 rows × 14 voices) and that the three pattern
  tables fit the file. It does **not** look for the `.W` sibling, because the
  host calls `canHandle()` *before* the secondary file is fetched.
* `getSecondaryFiles()` returns `<song>.W`, so the host downloads the wavebank
  next to the song (the standard EUP/KSS/SMUS companion mechanism).
* `fromFile()` reads the song plus the `.W` sibling (tries `.W` then `.w`).

## Playback

The Ensoniq 5503 is emulated in-process (no external library): oscillator 30
runs free as a ~50 Hz interrupt timer that clocks the tracker; 14 oscillator
pairs play the voices. Output is the chip's native **26320 Hz** stereo — the
host resamples to the device rate. Not routed through UADE.

## Provenance

Faithful C++ port of Sean Kasun's BSD-2-Clause SoundSmith player
(<https://github.com/mrkite/soundsmith>, vendored at repo-root `soundsmith/` —
see `src/es5503.ts`, `src/player.ts`, and `tools/ss2wav.c`). The port is
verified **bit-exact** against the reference `ss2wav.c` renderer on the test
fixture. Covered by the `SoundSmith plays sound` case in `chipmachine/test.cpp`.

## Known limitations

* `GSWV`-packed wavebanks (a few demos) are declined; Modland tunes use the
  regular wavebank layout.
* Instrument split records are read as the stored 12 bytes (two split entries:
  oscillator A and its pair B), matching the reference players. Hypothetical
  multi-split instruments needing more than two entries would degrade
  gracefully (read as zero) rather than crash.
