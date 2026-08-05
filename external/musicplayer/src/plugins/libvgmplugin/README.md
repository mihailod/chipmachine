# libvgmplugin

Plays **VGM/VGZ** logs through ValleyBell's **libvgm** — originally just the
OPL family (YM3812/OPL2, YMF262/OPL3, YM3526/OPL1, Y8950, i.e. the AdLib /
Sound Blaster PC chips), now everything GME cannot decode.

## Why a separate plugin

The other `.vgm/.vgz` handler is `gmeplugin`, whose blargg `Vgm_Emu` only
decodes SN76489 (PSG), YM2413 (OPLL), YM2612 and AY8910. Fed an OPL log it
renders **silence** (OPL3) or **aborts** with a `Blip_Buffer.cpp` assertion
(OPL2). So:

- `LibVGMPlugin::canHandle` claims `vgm/vgz` when the header carries **any chip
  GME cannot handle**, or a dual-chip flag on one it can (see
  `../../vgm_opl_detect.h`).
- `GMEPlugin::canHandle` **declines** those same files.

The two gates are disjoint, so plain Sega / AY console logs stay on GME.

## Source

libvgm is vendored once at `zxtune/3rdparty/vgm/` (zxtune ships the same tree
for its own — currently disabled — VGM support), plus the `USE_ZLIB`
FileLoader/MemoryLoader, which read `.vgz` directly with no external gunzip.

## Chip core selection

libvgm ships **several emulation cores per chip** and enables all of them unless
`SNDDEV_SELECT` is defined — its headers say so outright: *"if not asked to
select certain sound devices, just include everything (comfort option)"*. This
plugin defines `SNDDEV_SELECT` and lists chips (`SNDDEV_*`) and cores (`EC_*`)
explicitly in `CMakeLists.txt`. The `SNDDEV_*` list is copied verbatim from
libvgm's own default block in `emu/SoundEmu.c`, so **a chip added upstream must
be added there too or it silently disappears**.

Selecting cores is what lets the Mac App Store build leave libvgm's
GPL-licensed cores out (see `LEGAL` / `LEGAL-PLUS` for the licence side). Five
GPL cores turned out to be unreachable dead code — libvgm's own preference order
already put a permissive core first — and are compiled in **no** build:
`ym2413.c` and `nukedopll.c` (emu2413 wins), `ymf262.c` (AdLibEmu wins, and
VGMPlayer hard-requests `FCC_ADLE` for an OPL4's linked OPL3), `nes_apu.c`
(NSFPlay wins) and `ym2612.c` (GPGX, inside `fmopn.c`, wins).

Three chips differ per variant. `plus` keeps what it always played:

| chip | plus | mas |
|---|---|---|
| YM2203 / YM2608 / YM2610 | `fmopn.c` (MAME) | **ymfm**, via `opn_ymfm.cpp` |
| YM2612 | GPGX (in `fmopn.c`) | `ym3438.c` (Nuked OPN2) |
| YM2151 | `ym2151.c` (MAME) | `nukedopm.c` (Nuked OPM) |
| HuC6280 | `Ootake_PSG.c` | `c6280_mame.c` |

Measured against the MAME/Ootake cores on real game music, RMS ratios are 1.00
(OPN), 1.12 (YM2612), 1.00 (YM2151) and 0.98 (HuC6280). Two caveats:

- **Nuked OPM is slower** than the MAME YM2151. It matters on the X68000 corpus.
- **The MAME HuC6280 runs hot on noise/DDA-heavy material** and clips where
  Ootake does not (one Battle of the Bits rip: 1.49x, 785 clipped samples in
  30 s against Ootake's 3). Cause: libvgm's `_CHIP_VOLUME` table in
  `player/vgmplayer.cpp` holds **one volume per chip type**, hand-corrected for
  K051649 and C140/C219 only, so it is calibrated for whichever core is listed
  first. A per-core correction there (~0.7x for MAME) would fix it.

A core that is *not* compiled does **not** fail cleanly: VGMPlayer sets
`devDef = NULL` and continues, so the file still plays with that chip **silent**.
That is why cores are only dropped when something replaces them.

## The ymfm OPN adapter (`opn_ymfm.cpp`)

libvgm has no permissive OPN emulation at all — `fmopn.c` is MAME-derived and
GPL, and it is the only core it ships for YM2203/YM2608/YM2610. `opn_ymfm.cpp`
wraps Aaron Giles' **ymfm** (BSD-3, vendored at `external/ymfm`) in libvgm's C
`DEV_DEF` interface so the App Store build has one. Upstream libvgm has its own
`ymfmintf.cpp`, but only for the YM2414 (OPZ); it is a good template and nothing
more.

Four things the OPN family needs that a naive wrapper gets wrong:

1. **The SSG must stay a separate device.** libvgm models the YM2203/YM2608 SSG
   as a linked `DEVID_AY8910` with its own mixer channel and volume, and
   `fmopn.c` forwards registers `0x00-0x0F` to it. ymfm has an internal SSG,
   which would fold it into the FM output at ymfm's own balance. The adapter
   installs an `ymfm::ssg_override` that forwards to the linked device, and
   replicates `opnintf.c`'s `init_ssg_devinfo` (clock divider 1 for YM2203, 2 for
   YM2608/YM2610) — without that declaration libvgm never creates the SSG.
2. **YM2608 and YM2610 output must be doubled.** Both halve their output sum on
   real hardware and ymfm does it, but libvgm's `fmopn.c` leaves that step
   commented out (`//lt >>= 1; // shift right verified on real YM2608`), so
   libvgm runs 2x hot and `_CHIP_VOLUME` is calibrated to that. Measured exactly
   0.50 before the correction. YM2203 needs none.
3. **ymfm's sample rate must be folded down 3:1 for YM2203/YM2608.** ymfm's rate
   is prescaler-independent (`clock/24`, `clock/48`) — three times what `fmopn`
   reports. A 166 kHz device rate is outside anything libvgm's own cores produce
   (they sit at or below ~60 kHz) and **destabilises its resampler**: the
   fixed-point step maths in `Resmpl_Exec_LinearDown`, which carries its own
   comment about overflow "with extremely high chip sample rates", intermittently
   produced an absurd sample count, and `Resmpl_EnsureBuffers()` calls `abort()`
   when the resulting malloc fails. The symptom was a heisen-abort that vanished
   under lldb, Guard Malloc and ASan and became an apparent hang whenever the
   huge allocation happened to succeed. YM2610 was stable from the start because
   ymfm's `clock/144` already equals fmopn's rate.
4. **YM2612 comes with it.** The GPGX core lives inside `fmopn.c`, so dropping
   that file moves the chip to Nuked OPN2.

The YM2608 rhythm section reads the chip-internal ADPCM-A ROM dump that ships
with libvgm (`fmopn_2608rom.h`); without it every PC-98 track loses its drums.
`-UOPN_YMFM_RHYTHM_ROM` omits it.

`opnintf.c` / `opnintf.h` carry a **local patch** adding the `EC_*_MAME` /
`EC_*_YMFM` selection — grep `chipmachine local patch`, `.orig` copies sit next
to them, re-apply on revendor. ymfm itself is unmodified.

### Verifying a core swap

Build a standalone probe from the same source list and defines that prints the
core libvgm actually selected (`GetSongDeviceInfo` → `PLR_DEV_INFO.core`, a FCC)
plus a CRC/RMS of rendered PCM, and diff the configurations. Notes:

- `PlayerA::Render()` fills **at most one internal buffer per call** and returns
  the byte count — pull in chunks and honour the return, or 95% of what you
  "rendered" is untouched silence and every measurement is wrong.
- `testmus/libvgm/pc98-opn.vgz` has only **4 SSG writes**, so it does not
  exercise the SSG path; use a VGMRips PC-98 OPNA rip for that.
- Pick A/B material by the registers the cores differ on, but always listen to
  ordinary game music too — register-extreme compo tracks are unrepresentative.

## Build notes

libvgm's chip cores share global C symbol names with GME / s98 / famitracker /
kss. As with dmf/goattracker/ned/mikmod, the slice is compiled
`-fvisibility=hidden` and partial-linked with `ld -r` to demote those externs to
locals; only `LibVGMPlugin.cpp` stays at default visibility (the exported plugin
surface). ymfm is C++17 and compiled with the same flags; `opn_ymfm.cpp`
deliberately does **not** include `ymfm_fm.ipp` (`ymfm_opn.cpp` already
instantiates those templates, and a second copy cannot be merged once `ld -r`
has made the weak symbols private externs).
