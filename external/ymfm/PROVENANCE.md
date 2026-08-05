# ymfm (vendored)

Source: **ymfm** by Aaron Giles — https://github.com/aaronsgiles/ymfm
(fetched from the upstream `main` branch tarball, 2026-08-04).
License: **BSD 3-Clause** (`LICENSE`, and a copy of the notice in every source
file).

Yamaha FM chip emulation. Used by `musicplayer/src/plugins/libvgmplugin` as the
**non-GPL OPN family core** for the Mac App Store variant: libvgm's own
YM2203 / YM2608 / YM2610 emulation lives in `fmopn.c`, which is MAME-derived and
GPL-2.0-or-later. The adapter that presents these chips through libvgm's C
`DEV_DEF` interface is `libvgmplugin/opn_ymfm.cpp` (chipmachine code, not
upstream ymfm).

## Why upstream and not Furnace's copy
A copy of ymfm already sits under
`musicplayer/src/plugins/dmfplugin/furnace/src/engine/platform/sound/ymfm/`.
It is NOT used here. ymfm is BSD-3 at origin and Furnace's own LICENSE says its
bundled cores keep their own terms, but Furnace is GPL-2+ and modifies what it
vendors, so its copies carry GPL-licensed modifications. Taking the sources from
upstream keeps the licence chain clean. Same reasoning as the StSound re-vendor
and the MIT Musashi copy under sndhplugin.

## Layout
- `src/` — upstream `src/` verbatim, unmodified. Only the OPN + SSG + ADPCM
  translation units are compiled (`ymfm_opn.cpp`, `ymfm_ssg.cpp`,
  `ymfm_adpcm.cpp`, `ymfm_pcm.cpp`); the OPL/OPM/OPQ/OPZ/misc cores are present
  but not built, since libvgm already has permissive or LGPL cores for those.
- `LICENSE`, `README.md`, `GeneralInfo.md` — upstream.

## No local patches
The tree is unmodified. All chipmachine-specific glue lives in the adapter.
If you re-vendor, just replace `src/` and re-check `opn_ymfm.cpp` against
ymfm's API (the `ssg_override`, `ymfm_interface::ymfm_external_read` and
`opn_fidelity` interfaces are the ones it depends on).
