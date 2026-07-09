# musxplugin

Plays **Acorn Archimedes Tracker** modules (`.musx`) — the native format of
Dan Wilson's **!Tracker** (1991), an Amiga-Soundtracker-style editor for the
Acorn Archimedes that, unusually for its day, supported up to **8 channels**
with soft panning. Files begin with the ASCII magic `MUSX` and store IFF-style
chunks (`MNAM`/`ANAM` title & author, `MVOX` channel count, `PNUM`/`PATT`
patterns, up to 36 VIDC samples).

> The "Archimedes ran on ARM, and we're on ARM" angle is a red herring: this is
> not native code emulation. The replayer is portable C — it runs the same on
> any host.

## Engine: libxmp `arch_loader`

Playback uses **libxmp**'s Archimedes Tracker loader (`arch_load.c`), which is
already vendored in the tree under `zxtune/3rdparty/xmp/`. Rather than build the
full ~58-format libxmp or depend on the shared zxtune libxmp target, this plugin
compiles a **minimal single-loader slice** directly into `libmusxplugin.a`:

* core libxmp (player/mixer/scan/effects/…) + `loaders/{common,iff,sample,
  mmd_common,voltable,arch_load}.c`
* same trimmed build defines zxtune uses
  (`NO_COMPOSITE_LOADER`, `NO_PROWIZARD`, `NO_EXTERNALFILES`, `DECRUNCH_MAX=0`, …)
* `voltable.c` is a required link dependency — it provides `arch_vol_table`,
  used by `arch_load.c`'s `get_samp()`.

It deliberately does **not** `add_subdirectory()` the shared
`zxtune/3rdparty/xmp` build, so there is no duplicate `xmp` CMake target and
chipmachine's `zxtuneplugin` is untouched.

The module is driven through libxmp's internal
`xmp_load_typed_module_from_memory(ctx, mem, size, &arch_loader)` so only the
Archimedes loader is ever tried — the public 58-format auto-detect table is
never compiled in.

## Routing

`canHandle()` accepts a file only when the extension is `.musx` **and** the
first four bytes are `MUSX`. The `.musx` extension is also used by unrelated
software (e.g. Finale notation), so the magic check keeps the plugin from
grabbing foreign payloads. Because the gate is this narrow, there is no overlap
with OpenMPT's Amiga/PC tracker formats.

`getSamples()` renders 16-bit signed stereo at 44100 Hz via
`xmp_play_buffer(..., loop=1)` and returns `-1` once the module has played
through once, so the host advances instead of looping forever.

## License

libxmp is MIT (`zxtune/3rdparty/xmp/COPYING`, © 1996–2024 Claudio Matsuoka and
Hipolito Carraro Jr). The specific loader compiled here, `arch_load.c`, carries
an LGPL-2.1-or-later header from its 2013 authorship. Either way it is
redistributable with attribution; see the credits entry.

## Tests

`testmus/musx/` holds five Modland fixtures (varied authors and 4/8-channel
tunes). Covered by `TEST_CASE("Musx")` (corpus scan) and
`TEST_CASE("Musx plays sound")` (asserts non-silent output + magic gate) in
`chipmachine/test.cpp`.
