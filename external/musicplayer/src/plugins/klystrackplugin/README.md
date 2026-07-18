# klystrackplugin

Plays the modland **Klystrack** corpus: chiptunes authored in Tero Lindeman's
**klystrack** tracker (`.kt`), rendered by the tracker's own software synth.

## How it works

A `.kt` file is a klystrack song: an 8-byte `cyd!song` signature, a format
version byte, then the pattern/instrument data for klystrack's **cyd** software
sound engine (a virtual chip with pulse/saw/noise/triangle oscillators, a
wavetable, an FM operator, filters and effects). Playback is done by the
engine's own bundled player library, **libksnd**:

1. `canHandle()` gates on the `cyd!song` magic so unrelated `.kt` files are
   declined (Skip, not mis-play).
2. The song is loaded from memory (`KSND_LoadSongFromMemory`) into a player
   created with `KSND_CreatePlayerUnregistered` — i.e. no SDL audio thread; the
   host pulls audio synchronously.
3. `getSamples()` calls `KSND_FillBuffer`, which mixes the cyd engine straight
   into a 44100 Hz interleaved-stereo buffer. Song length is computed from
   `KSND_GetPlayTime`, and end-of-song is signalled to the host from that
   duration (libksnd fills silence past the end rather than short-reading).

## Vendored components / licensing

| Component | Source | License |
|-----------|--------|---------|
| `klystron/lib/` (libksnd wrapper) + `klystron/snd/` (cyd synth core) | Tero Lindeman ("kometbomb"), klystron (`https://github.com/kometbomb/klystron`) | MIT |
| `klystron/shim/` (SDL type/endian/audio shim) | this plugin | MIT (same as klystron) |

klystron is MIT-licensed (the license text is in `klystron/macros.h`), so the
plugin is MIT.

## The SDL shim (why `klystron/shim/`)

klystron's `snd/` core was written against SDL for three things: SDL's
fixed-width integer typedefs, its little-endian byte-swap macros, and its
mutex/audio-callback plumbing. This plugin drives libksnd manually through
`KSND_CreatePlayerUnregistered()` / `KSND_FillBuffer()` and builds with
`NOSDL_MIXER` and without `USESDLMUTEXES` / `USESDL_RWOPS`, which compiles out
the audio thread, mutex and RWops paths. That leaves only the integer types and
endian macros as a genuine dependency; `klystron/shim/SDL.h` +
`klystron/shim/SDL_endian.h` supply those (plus inert stubs for the dead
`cyd_register` audio-device path) so the plugin links **without any real SDL
dependency**. The vendored `snd/` and `lib/` sources are otherwise unmodified.

## Re-vendoring

To update klystron, re-copy `src/snd/*.{c,h}`, `src/lib/ksnd.{c,h}`,
`src/macros.h` and `src/sllhdr.h` from upstream into `klystron/`. The `shim/`
directory and this plugin's CMake handle the SDL-free build; no edits to the
vendored sources are required (re-check the `#ifdef NOSDL_MIXER` /
`USESDLMUTEXES` / `USESDL_RWOPS` guards if upstream reworks its audio layer).
