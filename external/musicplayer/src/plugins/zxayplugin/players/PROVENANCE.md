# ZX Spectrum AY replay routines -- where they came from

Every Z80 replay routine in this directory is the *original* ZX Spectrum player
for its format, obtained from Sergey Bulba's AY-3-8910 / YM2149 site --
<https://ay.strangled.net/progr_e.htm> (the historical
`bulba.untergrund.net`, which now redirects there and serves a frozen
2024-12-15 snapshot).

## Terms

Bulba states his terms on the Vortex Tracker II distribution:

> You can use and distribute sources freely, simply credit me somewhere in your
> projects, where you include all or part of the sources and (or) my algorithms.

That is an attribution grant, and it is the same footing on which this project
already ships the Fuxoft AY Language (`.fxm`) and AY Amadeus (`.amad`) players
-- see the Ayfly entry in `LEGAL`, which has carried that notice since those
formats were added. The credit is discharged in `LEGAL`, `README.md` and
`data/misc/Credits.rtf`.

The routines Bulba published as *disassemblies* (PT1, PSC, SQT, STP) are the
work of the tracker authors themselves; Bulba is the person who disassembled
and published them. They are credited individually below and in `LEGAL`.

## Inventory

| header | format(s) | org | bytes | upstream archive |
|---|---|---|---|---|
| `ptxplay_bin.h` | `.pt2` `.pt3` | `#C000` | 2546 | `PTxTools.7z` -> `PTxPlay/PTxPlay` |
| `pt1_bin.h` | `.pt1` | `#8000` | 2049 | `PT1Player.rar` -> `pt1.txt` |
| `psc_bin.h` | `.psc` | `#C000` | 3329 | `PSCPlayer.rar` -> `psc.txt` |
| `ftc_bin.h` | `.ftc` | `#C000` | 3329 | `FTCPlayer.7z` -> `ft.txt` |

`ptxplay_bin.h` is the author's own assembled binary, byte for byte. Its Z80
source (`PTxPlay.asm`) and the author's documentation of the entry points
(`PTxPlay.readme.txt`) are vendored beside it, unmodified.

`pt1_bin.h`, `psc_bin.h` and `ftc_bin.h` were rebuilt from the MONS4D
disassembly listings in `source/`, which carry each instruction's address *and*
its bytes:

```
82A2  EDB0           LDIR
```

`source/mons2bin.py` parses the address and hex columns and writes the byte
image. All three listings reconstruct to a fully contiguous image with zero
gaps and zero unparsed lines, which is the check that the hex column is
complete and self-consistent. (Fast Tracker's needed one extra rule: MONS4D
writes an orphaned index prefix -- a `DD` with no instruction after it -- as
`DD*`, where the `*` is the disassembler flagging it rather than a byte. The
prefix is real and belongs in the image, so `mons2bin.py` accepts the marker;
without it that listing showed 11 one-byte "gaps".) To regenerate:

```bash
python3 source/mons2bin.py source/pt1.mons4d.txt /tmp/pt1.bin
```

Sound Tracker Pro is the exception. Bulba published its player as a *symbolic*
listing (`source/stp.listing.txt`, decompiled by VfNG/NEW in 1997) with no byte
column, so there is nothing to rebuild an image from -- it would have to be
assembled, in a dialect none of this repo's tooling speaks. For one format that
is not worth it, so `.stp`/`.stp2` are sequenced natively instead
(`../zxay_stp.cpp`), like `.stc` and `.asc`. The listing stays vendored as the
cross-reference.

## Format documentation

`source/` also holds the format descriptions the native (non-Z80) players in
this plugin were written from:

| file | describes | author |
|---|---|---|
| `ST11FMT.txt` | Sound Tracker v1.1, compiled (`.stc`) **and uncompiled (`.st11`)** | RAMSOFT, 1993; sent to Bulba by Roman Scherbakov |
| `PT2.txt` | Pro Tracker 2 module layout | S.V. Bulba |
| `stp.listing.txt` | Sound Tracker Pro -- description *and* player listing | sent to Bulba by Roman Scherbakov |
| `gtr.listing.txt` | Global Tracker 1.0 -- player listing | Doctor Max / Global Corporation, 1998 |
| `ftc.readme.txt` | which Fast Tracker player version is which | S.V. Bulba |

`gtr.listing.txt` is symbolic with no byte column, exactly like Sound Tracker
Pro's, so `.gtr` is sequenced natively (`../zxay_gtr.cpp`) rather than run on
the Z80. Pro Sound Maker has no published listing at all and is likewise
native (`../zxay_psm.cpp`), following AY_Emul.

`.st11` has no player of its own. It is the UNCOMPILED form of the same music
`.stc` holds, so `../zxay_st11.cpp` compiles it into the compiled layout and
reuses the Sound Tracker sequencer -- which is what the tracker's own "ST
COMPILE" did on the Spectrum. Both layouts are in `ST11FMT.txt`, but two parts
of the compilation are conventions rather than consequences of the format and
follow AY_Emul's `ST12STC` under the attribution grant: the note-name table
(`A B C D E F G` -> semitones `9 11 0 2 4 5 7`), and the original compiler's
habit of emitting code `0x82` for ornament 0 where `0x70` would be the obvious
encoding -- Bulba's own source calls this "strange behaviour of the native
compiler" and reproduces it for compatibility, so this does too.

## What is deliberately NOT here

No code from `ayflyplugin` (GPL-2) or `zxtuneplugin` (GPL-3) was copied, read for transcription, or used
as a reference for any file in this plugin. Ayfly's own `players/*.h` are
C++ transliterations of Bulba's AY_Emul; where this plugin needs the same
behaviour it goes back to Bulba's published sources, which is what the
attribution grant covers. The three formats reclaimed from ZXTune (`.ftc`,
`.psm`, `.gtr`) were written the same way -- from Bulba's disassembly and from
AY_Emul -- and ZXTune's own sources were not consulted either.
