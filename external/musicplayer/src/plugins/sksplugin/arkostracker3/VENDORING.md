# Arkos Tracker 3 — vendored slice

Vendored to play STarKos (`.sks`) and (potentially) the other AY/YM formats the
Arkos Tracker 3 importers handle. Driven by
`musicplayer/src/plugins/sksplugin/`.

## Provenance

- Upstream: https://bitbucket.org/JulienNevo/arkostracker3
- Version: 3.6
- Commit: 55957b049b0cf6f278eafe78b378bb8ea96f195b (2026-06-08)
- License: MIT (see `LICENSE.txt`), by Julien Névo (Targhan / Arkos)

## What is vendored

- `source/` — the full AT3 source tree (only the non-GUI "BaseExport" slice is
  actually compiled; see `source/commandLineTools/baseExport/CMakeLists.txt`).
  The whole tree is kept so header includes resolve.
- `thirdParty/` — AT3's bundled helpers. Only `rasm` (Roudoudou's Z80
  assembler) and `lzh` (LzhStSound) are compiled, as `BaseExport`/`BaseCli`
  link them.
- `JUCE/` — Julien Névo's fork of JUCE 7.0.1.2 (patched for Xcode 16 / macOS
  Tahoe), **trimmed** to `modules/`, `extras/Build/` and the top `CMakeLists.txt`
  (examples, docs and the other `extras/` were dropped). AT3 normally fetches
  this via CPM; it is vendored here instead. Only three ISC-licensed modules are
  compiled and linked: `juce_core`, `juce_events`, `juce_audio_basics`.

## How it is built

`sksplugin/CMakeLists.txt` `add_subdirectory`s the upstream `baseCli` /
`baseExport` / `thirdParty` CMakeLists verbatim and the vendored `JUCE/`, then
compiles `SksPlugin.cpp` against `BaseExport`. The plugin reproduces the render
pipeline of AT3's headless `SongToWav` command-line tool
(`export/wav/SongWavExporter.cpp`): `SongLoader` → `SongPlayer` →
one `PsgStreamGenerator` per PSG → `PsgsProcessor` mix, streamed to PCM in
`getSamples()` instead of being written to a WAV file.

## Local modifications

None to the upstream source. The only adaptation lives in the plugin
(`musicplayer/src/plugins/sksplugin/`), which substitutes the vendored JUCE for
AT3's CPM fetch.
