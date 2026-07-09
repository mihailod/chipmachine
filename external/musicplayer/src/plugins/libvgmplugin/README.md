# libvgmplugin

Plays **OPL-family VGM/VGZ** logs — YM3812 (OPL2), YMF262 (OPL3), YM3526 (OPL1),
Y8950 — through ValleyBell's **libvgm**. These are the AdLib / Sound Blaster PC
chips.

## Why a separate plugin

The existing `.vgm/.vgz` handler is `gmeplugin`, whose blargg `Vgm_Emu` only
decodes SN76489 (PSG), YM2413 (OPLL) and YM2612. Fed an OPL log it renders
**silence** (OPL3) or **aborts** with a `Blip_Buffer.cpp` assertion (OPL2). So:

- `LibVGMPlugin::canHandle` claims `vgm/vgz` **only when the header carries an
  OPL clock** (see `../../vgm_opl_detect.h`).
- `GMEPlugin::canHandle` **declines** those same OPL files.

The two gates are disjoint, so the ~14k non-OPL console VGZ in the library
(BBC Micro / Coleco / MSX / Sega, all SN76489/YM2612) stay on GME untouched.

## Source

libvgm is vendored once at `zxtune/3rdparty/vgm/` (zxtune ships the same tree
for its own — currently disabled — VGM support). This plugin compiles that
tree's own `vgm` CMake source set (all chip cores, so `SoundEmu`'s device
table resolves) plus the `USE_ZLIB` FileLoader/MemoryLoader, which read `.vgz`
directly with no external gunzip step.

## Build notes

libvgm's bundled chip cores (`ay8910`, `ym2612`, `emu2413`, `nes_apu`, `gb`, …)
share global C symbol names with GME / s98 / famitracker / kss. As with
dmf/goattracker/ned/mikmod, the slice is compiled `-fvisibility=hidden` and
partial-linked with `ld -r` to demote those externs to locals; only
`LibVGMPlugin.cpp` stays at default visibility (the exported plugin surface).
