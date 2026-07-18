# Vendored webixs (Ixalance / .ixs player)

Source: the `src/` tree of **RetrovertApp/playback-ixalance**
(https://github.com/RetrovertApp/playback-ixalance), which vendors Juergen
Wothke's **webixs** (https://bitbucket.org/wothke/webixs).

Vendored from playback-ixalance commit
`651f86941e09a6b1b01a3ca9253df4daf9ab144e`.

## What it is

Ixalance (`.ixs`) is an Impulse-Tracker-family format from the (defunct)
**Shortcut Software Development BV** (~2000). It stores no PCM samples: it
synthesizes and zlib-compresses its own wavetable data, so whole songs are only
a few kilobytes. All known tunes are by **Maarten van Strien** (Modland
`Ixalance/Crystal Score/`).

The original player sources were lost. webixs is a **native C++
reimplementation reverse-engineered with Ghidra** (plus IDA) from Shortcut's
surviving Win32 `IXSPlayer` demo executable — hence the machine-address-suffixed
function names (`IXS__PlayerIXS__createPlayer_00405d90`, …) and the
`asmEmu` helpers throughout.

## License — IMPORTANT

**CC BY-NC-SA 4.0** (Attribution-NonCommercial-ShareAlike) — see `LICENSE`.
Copyright (C) 2022 Juergen Wothke; original x86 code (C) Shortcut Software
Development BV.

This is the **only NonCommercial-licensed component** in chipmachine. The player
may not be used for commercial purposes, and derivatives must keep the same
license.

## How it is built (see `musicplayer/src/plugins/ixsplugin/CMakeLists.txt`)

- Compiled with **`-DLINUX`** — webixs uses `LINUX` to mean "modern,
  non-Emscripten, non-Win32 build". This compiles out the original's Windows
  audio-device + worker-thread paths (all guarded by
  `#if !defined(EMSCRIPTEN) && !defined(LINUX)`) and exposes the pull-style
  render API in the vftable: `genAudio` / `getAudioBuffer` / `getAudioBufferLen`
  / `isSongEnd`. No `Windows.h` / `mmeapi.h` / `timeapi.h` is needed.
- Compiled with **`-fsigned-char`** — REQUIRED. The umbrella project builds with
  `-funsigned-char`, but the decompiled code relies on signed `char` semantics
  (signed sample/index math in `WaveGen.cpp`); without it the synth indexes out
  of bounds and crashes (SIGBUS on arm64). Same trap as `playerproplugin`.
- `WaveLibrary.c` is plain C (three small wavetable resource blobs that shipped
  with the original player) and is compiled as C by extension.
- Links **zlib** (`${ZLIB_LIBRARIES}`) for the wavetable (de)compression in
  `Packer.cpp`.

## Local modifications

None. The `src/` tree is vendored unmodified; all chipmachine integration lives
in `musicplayer/src/plugins/ixsplugin/`. `IxsScopeCapture.h` (a tracker-display
hook used by Retrovert) is kept because `PlayerCore.cpp` includes it; the
`scopeCapture` pointer is left null, so it has no effect.
