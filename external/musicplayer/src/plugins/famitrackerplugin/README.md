# famitrackerplugin — FamiTracker (NES / Famicom)

**FamiTracker** modules (`.ftm`), jsr's tracker for the Nintendo NES / Famicom
2A03 (RP2A03) and its expansion chips — the dominant modern tool for new NES
chiptunes (the modland `FamiTracker/` corpus).

Extensions: `.ftm` (FamiTracker; Face The Music `.ftm` routes to OpenMPT)

## Engine

A vendored, boost-free slice of the cross-platform **FamiTracker CX** engine
(nukep), driven synchronously at 44100 Hz. The NES APU + VRC6 / VRC7 / MMC5 / FDS
emulation renders mono, duplicated to stereo for the host. See
`famitracker-cx/PROVENANCE.md`.

Namco 163 (N163) and Sunsoft 5B modules are **not** driven — upstream never wired
their channel handlers — and decline gracefully (Skip).

## Routing

The `.ftm` extension is shared with the Atari **Face The Music** format (magic
`FTMN`), which [openmptplugin](../openmptplugin/README.md) handles. FamiTracker
is content-gated to its own magic (`FamiTracker Module`) so the two coexist.

## Build gating

Not in the Mac App Store build — the engine is GPL-2-or-later end to end, see
[`LEGAL-PLUS`](../../../../../LEGAL-PLUS) — so 1,597 rows are dropped from that
index. The 95 **Face The Music** `.ftm` rows are OpenMPT's and keep playing in
both builds, which is why the drop is keyed on the **format name**
(`formatPlayer` in `MusicDatabase.cpp`, two keys) rather than on the extension.

## Why nothing can be swapped in

Every other engine that reads `.ftm` is GPL (0CC-FamiTracker, Dn-FamiTracker,
FamiTracker CX itself, Furnace). FamiStudio is MIT but is a C# *importer* with its
own engine rather than a player — its own docs call the conversion lossy.

Rerouting was measured and rejected too: only 42 of the 1,597 rows have a
title+composer twin anywhere else in the catalog.

A replacement would have to be a **driver**, not a core swap: the chips
themselves are all available permissively in-tree already (the NSFPlay/xgm cores,
`emu2413`, `emu2149`), and those cover the Namco 163 and Sunsoft 5B channels
FamiTracker CX never wired — so a from-scratch driver would also pick up the ~175
files that play in neither build today.
