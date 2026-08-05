# aoplugin — AudioOverload (AOSDK)

Sega **Saturn**, Capcom **QSound** and **PlayStation 1 & 2** music.

Extensions: `.ssf` `.minissf` `.qsf` `.miniqsf` `.spu` `.psf` `.minipsf` `.psf2`
`.minipsf2`

## The PSF family arrived here on 2026-08-01

…when the Sony PS2 BIOS image that *Highly Experimental* needed was deleted from
the tree. AOSDK's `eng_psf` / `eng_psf2` were already compiled into this plugin
and simply unreachable — no extension pointed at them — and unlike Highly
Experimental they are **HLE**: `psx_hw.c` emulates the PS1 BIOS `A0`/`B0`/`C0`
vectors and the PS2 IOP kernel in software, so no BIOS image is involved at any
point.

This is now the **only** PSF decoder in the tree.

### Highly Experimental — removed

PlayStation 1 & 2 music used to be decoded by *Highly Experimental*. That plugin
has been **deleted**. It could not start without a real Sony PlayStation 2 BIOS
image, and the copy this project was shipping (`data/hebios.bin`, 512 KB of Sony
Computer Entertainment firmware) was not ours to redistribute; it is gone from
the tree and from both bundles. With no BIOS the engine decoded nothing, and it
carried no licence of its own, so there was nothing worth keeping. The full
write-up is in [`LEGAL-PLUS`](../../../../../LEGAL-PLUS).

**No songs were lost.** All four extensions moved here. Checked over ~100 modland
rips with `cm --dump-metadata`: every file Highly Experimental could load, AOSDK
loads.

## Build gating

Not in the Mac App Store build — its emulator cores are neither permissively nor
commercially licensed (see [`LEGAL-PLUS`](../../../../../LEGAL-PLUS)) — so 685
rows are dropped from that index: 668 PlayStation plus the 17 `.spu`/`.miniqsf`
that were AOSDK-only anyway.

Saturn `.ssf`/`.minissf` was initially unaffected, because
[htplugin](../htplugin/README.md) declares a higher priority and owned those —
but that plugin has since been gated too, so Saturn is absent from the App Store
build as well.

## Known issue: `.spu`

Pre-existing and unrelated to the PSF work: `.spu` (raw SPU RAM + register dumps,
9 songs) loads and steps its register stream but renders **silence**.

Until 2026-08-01 it appeared to work only because the signature test never
matched — `.spu` rips say `SPU1`, the test looked for `SPU\0` — so no engine ran
and the decoder returned the caller's buffer untouched, which in the live app is
the audio fifo's scratch buffer, i.e. the previous song's tail. It now correctly
returns silence.
