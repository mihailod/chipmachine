# zxtuneplugin

Plays **ZX Spectrum Sound Tracker 1.1** (`.st11`) through a trimmed build of the
[ZXTune](https://github.com/djdron/zxtune) engine, vendored as a sibling repo at
`chipmachine-as/zxtune/` (a checkout of **djdron/zxtune, branch `cmake`**).

## Why this plugin exists

Modland's *Sound Tracker 1.1* collection (59 files) is stored in a `ZXAYST11`
container: an 8-byte `ZXAY`-style header + metadata, wrapping a **raw, uncompiled
Sound Tracker v1.x module** starting at offset `0x38`. The existing `ayflyplugin`
(libayfly) only decodes the *compiled* `.stc` variant and has no uncompiled ST1
player, so before this plugin these tunes played in nothing. ZXTune's raw
container scanner finds the embedded module by content and its ST1 player renders
it; the plugin just feeds the whole file to `ZXTune::Service::DetectModules`.

`canHandle` is intentionally scoped to `.st11` only, to avoid colliding with the
many AY/tracker formats `ayflyplugin` (and others) already own. ZXTune supports
dozens more formats — widen `supported_ext` in `ZXTunePlugin.cpp` if desired.

## Build integration

`CMakeLists.txt` builds a trimmed subset of ZXTune (the AY/YM/DAC/FM/SAA decoder
family + the container scanner) as a static lib, at **C++20** with **hidden
symbol visibility** (so ZXTune's bundled `fmt` stays private and does not collide
with chipmachine's own `fmt::fmt` at final link — the same trick `kssplugin` uses
for OPLL). Only `fmt`, `z80ex`, `liblhasa` and `liblzma` are pulled from
ZXTune's `3rdparty/`; the heavyweight decoders (gme, libxmp, sidplayfp, asap,
vgmstream, …) are **not** compiled.

The archive-plugins factory is provided locally (`zxtune_archives.cpp`) instead
of ZXTune's `archives/{stub,full,lite}`. The *full* container set is required,
not just the raw container: ZXTune's raw scanner takes a lookahead optimization
over the other registered archive plugins, and registering raw alone leaves that
table empty and crashes.

## Required engine patches

The vendored `zxtune/` tree needs 5 small edits (kept in
[`zxtune-engine.patch`](zxtune-engine.patch); apply with `git apply` inside
`zxtune/`, or commit them to your fork):

| File | Change |
|------|--------|
| `src/strings/format.h` | include `fmt/format.h` instead of `fmt/core.h` |
| `src/core/plugins/players/CMakeLists.txt` | glob only the AY-family player dirs (`ay dac multi saa tfm`) |
| `src/core/plugins/players/plugins_list.cpp` | register only the self-contained AY/ST/PT/… players (drop XMP/GME/ASAP/MPT/VGMStream/V2M/SID/xSF/codecs) |
| `src/module/players/CMakeLists.txt` | drop the `xsf/` glob |
| `src/formats/archived/CMakeLists.txt` | also glob `multitrack/*.cpp` (the AY multitrack container) |

## Test corpus

`chipmachine/testmus/st11/` holds 21 known-good rips (covered by the `ZXTune` and
`ZXTune ST11 plays sound` cases in `chipmachine/test.cpp`). A handful of Modland
`.st11` rips have invalid/empty embedded modules and are intentionally excluded.
