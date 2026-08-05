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
(NSFPlay wins) and `ym2612.c` (GPGX, inside `fmopn.c`, wins). `fmopl.c`'s
YM3812/OPL2 path is dead the same way (AdLibEmu is listed first), and its YM3526
path is now dead too — see the table. A sixth, `pwm.c`, is no longer compiled in
either build for a different reason: it was **replaced** rather than out-ranked
(see the 32X PWM section).

Two chips changed core in **both** variants, so there is no divergence to
justify — one for sound, one because the GPL core was replaced outright:

| chip | plus and mas | why |
|---|---|---|
| YM3526 (OPL1) | **ymfm**, via `opl_ymfm.cpp` | preferred by ear over MAME |
| Sega 32X PWM | **`pwm_32x.c`**, written here | replaces the Gens core |
| Virtual Boy VSU | **`vsu_vb.c`**, written here | replaces the Mednafen core |

With those in place the **App Store build contains no GPL-licensed chip core at
all** — checked with `nm` over `liblibvgmplugin.a`, not just the build graph.

One chip is simply **absent from mas**: the **MSM5232** (`msm5232.c`, MAME, GPL).
It needs no replacement and no index gate, because it is unreachable — libvgm
reads its clock from header offset **0xF4**, one of its own extension slots that
the VGM format has never defined. Even a v1.71 header ends at 0xD0, and VGMRips
has no MSM5232 pack at all, so no file can select it. `SNDDEV_MSM5232` is the one
deliberate omission from the chip whitelist; the chip cannot be gated apart from
its core, because this is one of the few chips whose `DEV_DECL` lives inside the
core file rather than a separate `*intf.c`.

Six more differ per variant. `plus` keeps what it always played:

| chip | plus | mas |
|---|---|---|
| YM2203 / YM2608 / YM2610 | `fmopn.c` (MAME) | **ymfm**, via `opn_ymfm.cpp` |
| YM2612 | GPGX (in `fmopn.c`) | `ym3438.c` (Nuked OPN2) |
| YM2151 | `ym2151.c` (MAME) | `nukedopm.c` (Nuked OPM) |
| HuC6280 | `Ootake_PSG.c` | `c6280_mame.c` |
| Y8950 | `fmopl.c` (MAME) | **ymfm**, via `opl_ymfm.cpp` |
| YMF278B / OPL4 wavetable | `ymf278b.c` (openMSX) | **ymfm**, via `opl_ymfm.cpp` |
| RF5C164 (Sega/Mega CD) | `scd_pcm.c` (Gens) | `rf5c68.c` (MAME) |

The OPL4's *FM* half is a separate linked YMF262 device and is AdLibEmu in both.

Measured against the MAME/openMSX/Ootake cores on real game music, RMS ratios
are 1.00 (OPN), 1.12 (YM2612), 1.00 (YM2151), 0.98 (HuC6280), 1.00 (Y8950,
99% of 70 MSX tracks within ±3%), 0.97 (OPL4) and 1.006 (RF5C164, 117 Sega CD
tracks, all inside 1.004–1.015). Caveats:

- **Nuked OPM is slower** than the MAME YM2151. It matters on the X68000 corpus.
- **The MAME HuC6280 runs hot on noise/DDA-heavy material** and clips where
  Ootake does not (one Battle of the Bits rip: 1.49x, 785 clipped samples in
  30 s against Ootake's 3). Cause: libvgm's `_CHIP_VOLUME` table in
  `player/vgmplayer.cpp` holds **one volume per chip type**, hand-corrected for
  K051649 and C140/C219 only, so it is calibrated for whichever core is listed
  first. A per-core correction there (~0.7x for MAME) would fix it.
- **The ymfm YM3526 renders OPL rhythm mode differently** from MAME. 25 of 28
  arcade tracks land within ±3%; the three that do not are all Athena (SNK),
  which leans on the percussion channels — "Theme of Titan" measures 0.71x.
  A/B'd by ear, ymfm was **preferred** there, which is why this chip moved in
  both variants rather than only in `mas`.
- **The ymfm OPL4 does not interpolate between wavetable samples** and
  `ymf278b.c` does (linear, between adjacent samples). Real OPL4 hardware does
  not interpolate either, so ymfm is arguably the more faithful of the two, but
  it is audibly brighter. Across 33 arcade tracks that carry their own sample
  ROM (Strikers 1945 II, Gunbird 2) the ratio stays inside 0.91–1.03 with no
  clipping. Closing the gap would mean patching ymfm's `pcm_channel::fetch_sample`,
  which would end the "vendored unmodified" guarantee in
  `external/ymfm/PROVENANCE.md`; it was left alone deliberately.

## The 32X PWM (`pwm_32x.c`)

Not an adapter — an own implementation, written against the documented registers
and replacing libvgm's Gens-derived `pwm.c` in **both** variants (the VIC-I
precedent in `vtplugin`). The 32X's sound hardware is a PWM DAC, not a
synthesiser: the pulse width *is* the sample, so the work is timing and centring.

- `rate = clock / cycle` (register 1), pushed to libvgm through
  `SetSampleRateChangeCallback` when a log writes the cycle register.
  `Resampler.c` wires that callback for any device that offers one.
- `sample = width - cycle/2`, scaled to fill 16 bits. Silence is mid-period.
- Three-entry FIFO per channel, one pop per output sample, holding the last
  value on underrun.
- **`PWM_CTRL` is deliberately ignored.** Its low bits nominally select an
  output mode per side, but real 32X logs write it once as `0x100`/`0x200`/
  `0x300` — mode bits clear — and still expect stereo, so honouring a
  "mode 0 = mute" reading would silence the whole platform.

Measured against the Gens core over 40 PWM logs (VGMRips 32X packs), aggregate
**1.014**, median **1.000**, no clipping, 36/40 within ±1%. The outliers are all
files whose cycle is ~1045 rather than 1475, plus one homebrew test track that
feeds the PWM through **DAC Stream Control at 11010 Hz** into a ~22 kHz device
and measures 1.19. Worth knowing when comparing: **the Gens core reports a fixed
22020 Hz regardless of the cycle register**, so on underrun it loses energy where
this one holds the previous sample.

`CM_LIBVGM_LEGACY_PWM=ON` builds the old GPL core instead, purely so a future
change here can be A/B'd with this file as the single variable. It is refused
when `CM_HAVE_LIBVGM_GPL_CORES=OFF`.

## The Virtual Boy VSU (`vsu_vb.c`)

The other own implementation, replacing the Mednafen `vsu.c` in both variants.
Written from **Nintendo's Virtual Boy Development Manual**, Part 6 chapters 2–4
— every constant in the file traces to a numbered section there, which is why
the file cites them. The manual is in the SDK-manuals archive on archive.org
(item `virtual-boy.-7z`, `Technical_Informations.pdf`, pages 134–152); it is a
scan with no text layer.

Six sources: 1–5 play a 32-word, 6-bit waveform from one of five RAM banks, 5
adds sweep/modulation, 6 is noise. `f = clock / ((2048 - F) × 32)`, noise
`f = (clock/10) / (2048 - F)`, output level `((L/R × envelope) >> 3) + 1` and
zero if either input is zero. The timer periods land on exact sample counts at
the real clock — 3.84 ms is 160 samples, 15.36 ms is 640, 0.96/7.68 ms are
40/320 — but are derived from the reported rate so an odd header clock cannot
desynchronise them.

Two things cost real debugging time:

1. **The VGM offset is a word index, not the manual's byte address.** Every VSU
   register is four-byte aligned, so command `0xC7` carries `address >> 2`:
   0–159 waveform RAM, 160–191 modulation RAM, 256–351 channel registers, 352
   `SSTOP`. Decode it as a byte address and the chip renders **pure silence**.
2. **A decay envelope reaching 0 must not disable the channel.** The manual says
   the output "is stopped", but it also states plainly that "when the envelope
   value is 0, the sound is still being output at level 0, and the sound output
   is not considered to have stopped". Games drive `SxEV0` as a real-time volume
   with automatic enveloping on, so a channel that has decayed to zero has to
   come back on the next `SxEV0` write. Gating it made one track render at
   **0.40x** while all 352 others measured 1.00.

Measured against Mednafen over the whole **353-file** corpus (all 17 VGMRips
VSU packs): median **1.000**, aggregate **1.0018**, 87% within ±1%, 96% within
±3%, **100% within ±10%**, and slightly *less* clipping than the reference
(146,275 vs 146,338 samples; one track clips where the reference does not).

`CM_LIBVGM_LEGACY_VSU=ON` rebuilds the old GPL core for single-variable A/B, and
like its PWM counterpart is refused when `CM_HAVE_LIBVGM_GPL_CORES=OFF`.

**The RF5C164 is not a core-list swap but a *default* swap.** `rf5cintf.c`
already offers both cores for `DEVID_RF5C68`; what picks Gens for the RF5C164 is
one line in `player/vgmplayer.cpp` (`case DEVID_RF5C68`), and only when nothing
else asked. That line now follows `EC_RF5C68_GENS` — a **local patch**, `.orig`
next to it. It has to follow the build, because `SndEmu_StartCore` does **not**
fall back: a requested-but-absent core returns `EERR_NOT_FOUND` and VGMPlayer
then plays that chip silent. The MAME core ignores `cfg->flags` entirely, so it
treats RF5C68 / RF5C164 / RF5C105 alike; the Gens core uses the flag only for a
Cosmic Fantasy Stories MCD workaround.

**Beware MSX MoonSound rips when measuring the OPL4.** They carry only the
small user-sample RAM upload (VGM data block `0x87`) and no ROM block (`0x84`):
the instruments live in the MoonSound's internal 2 MB YRW801 ROM, which VGM logs
do not embed. Both variants render those files as noise, and an early A/B of
this swap was run almost entirely on them before that was spotted. Check for a
`0x84` block before trusting a number.

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

## The ymfm OPL adapter (`opl_ymfm.cpp`)

Same shape, for the three chips whose only libvgm core is GPL: the **YM3526**
and **Y8950** (`fmopl.c`, MAME) and the **YMF278B** OPL4 wavetable
(`ymf278b.c`, an openMSX port). Easier than the OPN in two ways — ymfm's OPL
sample rate is `clock/72`, exactly what `oplintf.c` reports, and its `ymf278b`
returns `clock/768`, exactly what `ymf278b.c` reports, so nothing needs the
rate folding the OPN required and nothing goes near the resampler's ceiling.

The OPL4 is the one that needed thought. **libvgm does not model it as one
chip**: `ymf278b.c` is the wavetable half only, and the FM half is a separate
linked `DEVID_YMF262` that VGMPlayer hard-requests `FCC_ADLE` for. ymfm's
`ymf278b` contains its own FM and offers no `fm_override` hook, so either

- (a) let ymfm do the FM and drop the link — one device instead of two, which
  moves the FM onto a different `_CHIP_VOLUME` entry *and* swaps AdLibEmu for
  ymfm's OPL3 at the same time: two changes at once, for no gain; or
- (b) keep libvgm's structure, forward ports 0–3 to the linked YMF262 and emit
  ymfm's PCM alone.

(b) is what shipped, so both variants share the same FM core and the same two
volume-table entries and only the wavetable engine changes. Three traps in it:

1. **ymfm's PCM writes are gated on the FM register file.** `write_data_pcm()`
   returns early unless NEW2 is set, and NEW2 arrives through an *FM* port. So
   ports 0–3 must reach ymfm too, not just the linked device — feed it only
   ports 4/5 and the chip is silent.
2. **Which means ymfm's FM output has to be discarded**, or the FM renders
   twice. `ymf278b::generate()` mixes FM and PCM into lanes 4/5 before the
   caller sees them, so `opl4_pcm` subclasses it and reimplements the loop:
   both engines still clock identically (the FM drives timers and the BUSY/LD
   status bits, and skipping it desynchronises the PCM envelopes), only the FM
   *output* is dropped.
3. **The FM mix level's power-on value differs.** `ymf278b.c`'s reset sets
   `fm_l = fm_r = 3`; ymfm's register file defaults to 0, which is +6 dB. That
   level is passed to the linked YMF262 as a device volume, so reading it back
   out of ymfm ran the FM 2.7x hot on every log that never writes register
   0xF8 — most MSX MoonSound music. The adapter tracks it itself.

The PCM level also needs a **x3/4** calibration, measured rather than derived:
the two engines carry different internal headroom (libvgm applies a documented
-15 dB trim per slot, ymfm scales inside its PCM channel) and they do not
cancel. Unscaled, ymfm rendered 1.315x libvgm.

`oplintf.c` / `oplintf.h` carry the same kind of **local patch** as `opnintf`,
introducing `EC_YM3526_*` and `EC_Y8950_*` — upstream has no `EC_` macros for
those two chips at all and reaches into `fmopl` directly under `SNDDEV_*`.
`.orig` copies sit next to them. `ymf278b.c` needed no patch: it is a
single-core file, so `opl_ymfm.cpp` simply supplies the `sndDev_YMF278B`
`DEV_DECL` in the build where that file is absent.

The `ymfm_interface` that serves sample memory out of libvgm's
`DEVRW_MEMSIZE` / `DEVRW_BLOCK` calls is shared by both adapters and lives in
`ymfm_rom_intf.h`.

## Verifying a core swap

`vgm_core_probe.cpp` is that tool, kept in the tree. It is **off by default**;
build it per variant and diff the two:

```
cmake -B build-probe-plus -S chipmachine -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCM_VARIANT=plus -DCM_BUILD_VGM_PROBE=ON
ninja -C build-probe-plus vgm_core_probe
build-probe-plus/plugins/libvgmplugin/vgm_core_probe -t 30 [-w wavdir] file.vgz...
```

It links the OBJECT library directly, *before* the `ld -r` visibility pass, so
it sees exactly that variant's source list and `SNDDEV_`/`EC_` selection. For
each file it prints the core libvgm actually chose (`GetSongDeviceInfo` →
`PLR_DEV_INFO.core`, a FourCC) including linked devices, plus RMS, peak and
clipped-sample count; `-w` dumps WAVs so a swap can be judged by ear too.
Notes:

- `PlayerA::Render()` fills **at most one internal buffer per call** and returns
  the byte count — pull in chunks and honour the return, or 95% of what you
  "rendered" is untouched silence and every measurement is wrong.
- Call `Start()` **before** `GetSongDeviceInfo()`, or every core reads 0 and
  every rate reads 0 — the devices do not exist yet.
- `testmus/libvgm/pc98-opn.vgz` has only **4 SSG writes**, so it does not
  exercise the SSG path; use a VGMRips PC-98 OPNA rip for that.
- Pick A/B material by the registers the cores differ on, but always listen to
  ordinary game music too — register-extreme compo tracks are unrepresentative.
- **Ratios on near-silent tracks are noise.** Filter by absolute RMS (≥100 or
  so) before summarising, or a jingle going from RMS 20 to RMS 57 reports as a
  2.8x regression.
- Check the file actually contains the sample data the chip needs — see the
  MoonSound warning above. Aggregate over a corpus, not one track.

OPL fixtures in `testmus/libvgm/`: `arcade-ym3526.vgz` (Karnov),
`arcade-ym3526-rhythm.vgz` (Athena — the rhythm-mode divergence),
`msx-y8950.vgz` (Gorby no Pipeline Daisakusen),
`arcade-ymf278b-opl4.vgz` (Gunbird 2, with its ROM block) and
`megacd-rf5c164.vgz` (Willy Beamish — deliberately a track with **no** YM2612,
so it isolates the RF5C164 instead of also measuring the OPN2 swap).
`virtualboy-vsu.vgz` covers the VSU and `32x-pwm.vgz` the 32X PWM.

## Build notes

libvgm's chip cores share global C symbol names with GME / s98 / famitracker /
kss. As with dmf/goattracker/ned/mikmod, the slice is compiled
`-fvisibility=hidden` and partial-linked with `ld -r` to demote those externs to
locals; only `LibVGMPlugin.cpp` stays at default visibility (the exported plugin
surface). ymfm is C++17 and compiled with the same flags; `opn_ymfm.cpp` and
`opl_ymfm.cpp` deliberately do **not** include `ymfm_fm.ipp` (`ymfm_opn.cpp` /
`ymfm_opl.cpp` already instantiate those templates, and a second copy cannot be
merged once `ld -r` has made the weak symbols private externs). Calling a
template member from an adapter is fine — without the `.ipp` the compiler emits
an ordinary out-of-line call, which resolves against that instantiation.
