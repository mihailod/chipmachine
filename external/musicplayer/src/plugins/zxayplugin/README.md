# zxayplugin — ZX Spectrum AY

ZX Spectrum **AY-3-8912** tracker music. At 67,305 songs this is the largest
single format family in the catalog.

Extensions: `.psg` `.asc` `.stc` `.psc` `.sqt` `.stp` `.stp2` `.pt1` `.pt2`
`.pt3` `.vtx` `.vt2` `.zxs` `.st13` `.fxm` `.amad` `.ftc` `.psm` `.gtr` `.st11`

This engine was written for this project, with no copyleft anywhere in the chain,
and is present in **both** builds. It replaced **Ayfly** as the primary ZX AY
decoder.

## Three playback routes

1. **The tracker's own ZX Spectrum replay routine**, run on an emulated Z80 (the
   same GME core the Beepola and Sam Coupé players use) with its `OUT`s to
   `#FFFD`/`#BFFD` fed into **Ayumi**, Peter Sovietov's AY-3-8910 emulation.
   This is how `.pt1`, `.pt2`, `.pt3`, `.vt2`, `.psc` and `.ftc` play.

   Running the original code is the point: each of these formats has exactly one
   authoritative definition — the author's own player — and that is where the
   version-gated fixups, portamento variants and table quirks actually live. The
   routines are Sergey Bulba's published players; see `players/PROVENANCE.md`.

2. **A sequencer written from the published format description**, where no
   redistributable ZX player exists: `.stc` (and the `.zxs` / `.st13` files that
   are the same format under another name), `.asc`, `.stp` / `.stp2`, `.sqt`,
   `.psm`, `.gtr`, and the `.fxm` / `.amad` bytecode.

3. **No player at all** for `.vtx` and `.psg`, which are not modules but recorded
   AY register streams — the file *is* the register writes. `.vtx` is LH5-packed,
   the same packing the `.ym` files in
   [stsoundplugin](../stsoundplugin/README.md) use.

   `.st11` needs no player either, for a different reason: it is an *uncompiled*
   Sound Tracker module, so it is compiled into the ordinary `.stc` layout on
   load and handed to the Sound Tracker sequencer above — which is precisely what
   the tracker's own "ST COMPILE" did on the Spectrum.

## Formats reclaimed from ZXTune

Four formats were reclaimed once this machine existed — **Pro Sound Maker**
(`.psm`), **Fast Tracker** (`.ftc`), **Sound Tracker 1.1** uncompiled (`.st11`)
and **Global Tracker** (`.gtr`), 378 songs — so they play in both builds. That is
every ZXTune format that is actually AY music; see
[zxtuneplugin](../zxtuneplugin/README.md) for the two that are not.

## `.vt2` got better, not merely portable

Ayfly claimed the extension and then threw on every one of the catalog's 551
rows, which also stopped anything else from trying. Vortex Tracker II's binary
save is really a PT3 module wearing a different identifier, so it now plays
through Bulba's PTxPlay; the editor's rarer ini-style text export goes to Arkos
Tracker 3 via [sksplugin](../sksplugin/README.md). Both builds gain those rows.

## `.fxm` and `.amad`

`.fxm` is **Fuxoft AY Language** — František Fuka's compiled AY music format
("FXSM" files) — and `.amad` is **AY Amadeus**, the same bytecode in the `ZXAY`
container with an `AMAD` type tag, by František Fuka and Patrik Rak. Both rebuild
a 64K Spectrum image from the file's origin address and interpret over it as the
original Z80 playroutine does.

## Priority / routing

**Ayfly** and **ZXTune** both remain in the Plus build only (see
[`LEGAL-PLUS`](../../../../../LEGAL-PLUS)), registered *ahead* of this plugin, so
that build routes every ZX AY song exactly where it always did. The App Store
build ships without either.

`.ay` — the ZXAYEMUL container of raw Z80 rips — belongs to
[gmeplugin](../gmeplugin/README.md) in both builds, which plays the Amstrad CPC
rips Ayfly renders silent.

Spectrum AY labels from zxart are normalized at index time.
