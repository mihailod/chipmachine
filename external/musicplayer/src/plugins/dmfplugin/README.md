# dmfplugin — DefleMask `.dmf` playback

Plays **DefleMask** modules (`.dmf`) across every DefleMask target — Sega
Genesis/Mega Drive (YM2612 + SN76489), Sega Master System (SN76489), Game Boy,
PC Engine (HuC6280), NES (2A03), Commodore 64 (SID 6581/8580), Arcade
(YM2151 + SegaPCM) and Neo Geo (YM2610).

DefleMask `.dmf` shares its extension with the unrelated **X-Tracker DMF**
(`DDMF`) format handled by libopenmpt. DefleMask files are zlib-compressed
(first byte `0x78`) and inflate to the magic `.DelekDefleMask.`; both
`DMFPlugin::canHandle` and `OpenMPTPlugin::canHandle` content-gate on that byte
so the two formats coexist (`DMFPlugin::priority()` returns 1 to win the
extension). Prior to this plugin, DefleMask `.dmf` was declined with
"Deflemask DMF format is unsupported".

## How it works

The plugin wraps a headless slice of the **Furnace** engine (`DivEngine`),
driven directly:

- `DivEngine::load()` inflates and parses the module (loader:
  `furnace/src/engine/fileOps/dmf.cpp`).
- `DivEngine::init()` with `DIV_AUDIO_DUMMY` (no live audio backend, no SDL).
- `getSamples()` pulls float PCM via `DivEngine::nextBuf()` and converts to
  interleaved int16.

**Large-stack load.** `DivEngine::loadDMF` (like every Furnace loader) puts a
full `DivSong` — ~744 KB — on the stack. chipmachine loads songs from a
`MusicPlayerList` worker thread whose default stack is ~512 KB on macOS, so the
load overflowed and bus-errored ("thread stack size exceeded"); the main thread
(cmtest, CLI, `-X` textmode) has 8 MB and never hit it. `DMFPlayer` therefore
runs `engine->load()` on a dedicated 16 MB-stack thread. Regression:
`TEST_CASE("DMF loads on a small-stack worker thread")` invokes `fromFile()` from
a 512 KB-stack thread.

## Vendored engine

- **Upstream:** [tildearrow/furnace](https://github.com/tildearrow/furnace),
  commit `ebb8a9d806226fc389cc8f6d4e1bb3b28446f22b`.
- **License:** GPL v2 or later (see [`../../../../furnace/LICENSE`](../../../../furnace/LICENSE)).
  Bundled chip-emulation cores under `furnace/extern/` carry their own
  compatible licenses (BSD/MIT/LGPL/GPL) — see each subdirectory.
- **Slice:** exactly the source set Furnace's own `BUILD_GUI=OFF` headless
  target compiles, minus `src/main.cpp` / `src/cli` (we provide our own entry
  point). The GUI, SDL, PortAudio, libsndfile and RtMidi are not vendored.

Furnace bundles its own copies of chip cores (`sn76496`, `ay8910`, `segapcm`,
NES `apu`, Nuked-OPN/OPLL/PSG, emu2413, reSID-family, …) whose global C symbols
collide with GME / s98 / famitracker. As with `goattrackerplugin` /
`nedplugin` / `mikmodplugin`, everything is compiled with hidden visibility and
partial-linked with `ld -r`, leaving `musix::DMFPlugin` + `dmfplugin_register`
as the only exported symbols.

## Local patches (re-apply on Furnace revendor)

Grep the tree for `[chipmachine local patch]`:

- `furnace/src/fileutils.h` — add `#include <cerrno>` (`ENXIO` was only
  transitively available on Furnace's own toolchain).
- `furnace/src/fileutils/cfile.cpp` — add `#include <cstring>` (`memset`).
- `furnace/src/engine/dispatchContainer.cpp` — **licence fix**: drop the
  `platform/vic20.h` include and the `case DIV_SYSTEM_VIC20` arm.
  `sound/vic20sound.c` is lifted from VICE and carries VICE's GPLv2 header (it
  is the same file `victrackerplugin`'s VIC-I core was extracted from), and
  `platform/vic20.cpp` was its only user. `DIV_SYSTEM_VIC20` is a Furnace
  `.fur`-only target — DefleMask has no VIC-20 system and `DMFPlugin::canHandle`
  claims only `dmf` — so the code was unreachable, and both files are omitted
  from the source list in `CMakeLists.txt` for **both** build variants. The
  switch's `default:` arm already falls back to `DivPlatformDummy`, so nothing
  else changes. Verify with
  `nm -a <binary> | grep voltagefunction` — it must print nothing.
- `furnace/src/engine/engine.h` — **memory-safety fix** (report upstream): the
  `DivEngine` constructor zeroed the `float* filePlayerBuf[DIV_MAX_OUTPUTS]`
  pointer array with `sizeof(float)` instead of `sizeof(float*)`, leaving the
  upper half uninitialized. `nextBuf()`'s buffer-resize path then does
  `delete[] filePlayerBuf[i]` on those garbage pointers. It only appeared benign
  because a fresh process's heap is zero-filled; running the engine after other
  allocators had dirtied the heap (e.g. later in a test suite) turned it into a
  `pointer being freed was not allocated` abort.

Additionally, the Furnace slice is compiled at **C++14** (upstream's standard);
at C++17 Furnace's bundled fmt 10.1 selects a `std::string_view` `vsprintf`
overload in `log.cpp` that it does not accept. Only `DMFPlugin.cpp` is compiled
at C++17 (it needs `chipplugin.h`'s `static inline`). Furnace's bundled fmt is
compiled in and pinned ahead of chipmachine's newer fmt via `BEFORE`.

`sndfile_stub/` provides no-op stubs for the handful of libsndfile symbols the
engine references (external audio-file instruments / audio export — never
exercised by `.dmf` playback), so libsndfile need not be vendored.

## Patched vendored file: `platform/sound/rss.h`

Furnace shipped the YM2608's internal ADPCM-A rhythm ROM here — copyrighted
Yamaha firmware under no licence, byte-for-byte the same dump libvgm carried.
It has been replaced with synthesized voices. Regenerate with
[`libvgmplugin/gen_2608rom_synth.c`](../libvgmplugin/gen_2608rom_synth.c), which
emits this file and libvgm's copy from one source:

```
/tmp/g ../dmfplugin/furnace/src/engine/platform/sound/rss.h nonstatic
```

The `nonstatic` argument matters: `ym2608Interface.cpp` includes this as plain
`const`, whereas libvgm's `fmopn_2608rom.h` is `static const`. Furnace's address
layout is identical (`set_start_end()` in `sound/ymfm/ymfm_opn.cpp`), so nothing
else needs changing. **Re-apply on revendor** — and note there is deliberately no
`.orig` copy, since keeping one would mean keeping the ROM. See that generator's
README section for what the voices are and are not.

## Tests

`chipmachine/testmus/dmf/` fixtures span Genesis (`Spring Yard.dmf`), SMS
(`SuPeHaRiMAIN-StarrySky.dmf`) and Game Boy (`darkman_bonus.dmf`);
`TEST_CASE("DMF")` in `chipmachine/test.cpp` asserts each loads and produces
non-silent audio.
