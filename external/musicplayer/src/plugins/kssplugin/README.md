# kssplugin — MSX

MSX music, via **libkss**.

Extensions: `.mgs` `.bgm` `.opx` `.mpk` `.mbm` `.mus`

## MGS and friends

`.mgs` (MGSDRV) and the other driver formats are played by libkss directly.

## `.mus` — FAC SoundTracker

`.mus` here is **FAC SoundTracker** (Federation Against Commodore, 1990/1991),
the PSG-plus-sampled-drums MSX tracker. The song is converted on the fly into a
KSS image carrying FAC's own Z80 replay routine and played through libkss.

Drummed songs pull in their `<DRUMKIT>.SM1` / `.SM2` sample-bank companions from
the same folder. Those `.sm1`/`.sm2` files are drumkit banks, **not** standalone
tunes — a bare `.sm1`/`.sm2` is declined (they are MSX BSAVE images) and the FAC
DRUMKIT case skips gracefully.

The `.mus` extension is shared with libopenmpt and with Compute!'s Sidplayer;
routing is by content.

## Related

`.SNG` (uppercase) is Konami SCC music and belongs to
[sccmusixxplugin](../sccmusixxplugin/README.md).
