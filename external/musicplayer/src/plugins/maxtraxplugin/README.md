# maxtraxplugin

Plays MaxTrax `.mxtx` Amiga modules (e.g. David A. Bean's *Dark Seed* score)
via a vendored port of ScummVM's portable MaxTrax player.

UADE is **not** the path: its `amifilemagic.c` detects the `MXTX` magic but
there is no MaxTrax eagleplayer upstream or in our vendored player set.

## Layout

- `MaxTraxPlugin.{h,cpp}` — the chipmachine `ChipPlugin`/`ChipPlayer` wrapper.
  `canHandle` gates on the `mxtx` extension **and** the `MXTX` content magic;
  `getSamples` is a thin pass-through to `Paula::readBuffer`.
  Handles self-contained modules **and split sets**, resolved by *content*
  (a cheap `probe()` counts scores/samples in a file's header):
    - A self-contained file (scores>0, samples>0) plays directly.
    - A score-only file (samples==0) borrows the instrument bank of a sibling
      `.mxtx` in the same directory that has samples.
    - An instrument-only bank (scores==0) borrows the scores of a sibling part.
  The sibling is chosen by the **longest shared filename prefix** (then sample
  count), so a score only pairs within its own set even when several sets share
  a directory. This covers both the suffix split (Kyrandia `…scr`/`…inst`) and
  the shared-bank split with no naming marker (Russell Lieblich's A-Train: many
  tiny score parts + one big `a-train (intro).mxtx` bank). `load()` is then
  called as `load(score, scores=true, samples=false)` + `load(inst, false, true)`.
  When streaming (no local mirror), the bank's filename usually can't be derived
  from a score part, so `getSecondaryFiles` probes the primary and, for any split
  half, returns the sentinel `"./"` — asking the host (MusicPlayerList) to list
  the song's own directory and fetch every sibling next to it, so the bank lands
  in the cache dir for `fromFile`'s scan. (`MusicPlayerList` was extended with a
  `"./"` = current-directory case alongside the existing subdir sentinel used by
  IFF-SMUS `Instruments/`.) A self-contained module requests nothing.
- `maxtrax/maxtrax.{h,cpp}` — score/sample loader + sequencer (ScummVM, GPLv3)
- `maxtrax/paula.{h,cpp}`   — Amiga Paula mixer (ScummVM, GPLv3)
- `maxtrax/compat.h`        — small shim replacing the ScummVM deps
  (`Common::SeekableReadStream`, `Common::Mutex`, `frac_t`, `MIN/MAX/CLIP`,
  `debug/warning`) so the two files build standalone.
- `maxtrax/poc_main.cpp`    — standalone decode→WAV harness (not built by CMake).

### Local modifications to the vendored ScummVM sources
- `maxtrax.h`: upstream `ENABLE_KYRA`/`DYNAMIC_MODULES` gate replaced with a
  plain include guard; added `getScoreCount()`. `#include` of paula → `"paula.h"`.
- `paula.h`: `class Paula : public AudioStream` → `class Paula`; includes →
  `compat.h`; `Common::Mutex &_mutex` → value member.
- `paula.cpp`: includes → `compat.h`+`paula.h`; dropped
  `_mutex(g_system->getMixer()->mutex())` from the ctor init list; deleted the
  `AmigaMusicPlugin` / `REGISTER_PLUGIN_STATIC` block.
- `maxtrax.cpp`: ScummVM includes → `compat.h`+`maxtrax.h`.

## Format

Single combined module, all big-endian: `MXTX` magic, `uint16` tempo, `uint16`
flags (bit0 lowpass filter, bit1 attack-volume, bit15 microtonal), optional
microtonal block, `uint16` score count + scores (6-byte events), `uint16`
sample count + sample patches (envelopes + 8-bit signed PCM).

## Standalone decode harness (optional)

```sh
cd maxtrax
clang++ -std=c++17 -O2 poc_main.cpp paula.cpp maxtrax.cpp -o maxtrax_poc
./maxtrax_poc darkseed_00.mxtx out.wav 12 0
```

## License

The vendored MaxTrax/Paula sources are GPL-3-or-later (ScummVM).

**Plus build only.** Because of that licence this plugin is NOT compiled into the
Mac App Store variant: `CM_VARIANT=mas` sets `CM_HAVE_MAXTRAX=OFF`, so the target
is not built and `maxtraxplugin_register()` is `#ifdef`'d out
(`CM_NO_MAXTRAX`). The 93 `.mxtx` rows are dropped from that variant's index to
match — `.mxtx` is sole-claimed (vgmstream names it only to decline it), so the
plain extension test in `songHasNoPlayer()` covers the modland rows and
`nameHasPlayer()`'s leading-token branch covers the UnExoticA `mxtx.<song>`
prefix form; no `formatPlayer` key is needed.

Nothing replaces it there: ScummVM's is the only native MaxTrax implementation
that exists, every other player is unlicensed 68k assembly (Wanted Team's
EaglePlayer), and a `.mxtx` carries no replayer of its own, so the sc68/SNDH
"emulate the original driver" route is closed too. See `CM_HAVE_MAXTRAX` in
`chipmachine/CMakeLists.txt` for the full reasoning and the clean-room estimate,
and `chipmachine/LEGAL-PLUS`.
