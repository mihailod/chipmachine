# adplugin — AdPlug

OPL2/OPL3 (AdLib / Sound Blaster) hardware-simulation formats, via **AdPlug
2.4** (vendored).

## Formats

AdLib Tracker 2 by subz3ro, AdLib MIDI Music Format by Ad Lib Inc., AdLib
MIDIPlay File, AdLib MSCplay, AdLib Visual Composer, AMUSIC Adlib Tracker by
Elyssis, Apogee IMF, Beni Tracker (PIS), Bob's Adlib Music Format, BoomTracker
4.0 by CUD, Coktel Vision AdLib Music, Creative Music File Format, DeFy Adlib
Tracker, Digital-FM by R. Verhaag, DOSBox Raw OPL (v0.1 and v2.0), Easy AdLib
1.0 by The Brain (BMF), eXotic ADlib Format by Riven the Mage (incl. Flash,
Hybrid, Hypnosis, PSI, rat), eXtra Simple Music by Davey W Taylor, God of Thunder
Music by Roy Davis, Herbulot AdLib System / HERAD, HSC Adlib Composer by Hannes
Seifert, HSC-Tracker by Electronic Rats, HSC Packed by Number Six / Aegis Corp.,
JBM Adlib Music Format, Ken Silverman's Music Format, LOUDNESS Sound System,
LucasArts AdLib Audio File Format, Master Tracker, MIDI Audio File Format,
MKJamz by M\K Productions, Mlat Adlib Tracker, MPU-401 Trakker by SuBZeR0, Note
Sequencer by Lee Ho Bum (sopepos), Origin AdLib Music Format (Ultima 6), Packed
EdLib by Vibrants, PALLADIX Sound System, RdosPlay RAW by RDOS, Reality ADlib
Tracker (incl. RAD v2), ScreamTracker 3 by Future Crew, Sierra's AdLib Audio
File Format, Softstar RIX OPL Music Format, Surprise! Adlib Tracker 1 & 2, Twin
TrackPlayer by TwinTeam, Westwood ADL File Format, XMS-Tracker by MaDoKaN/E.S.G.

## Extensions

`.a2m` `.a2t` `.adl` `.adlib` `.agd` `.amd` `.as3m` `.bam` `.bmf` `.cff` `.cmf`
`.d00` `.dfm` `.dmo` `.dro` `.dtm` `.got` `.ha2` `.hsc` `.hsp` `.hsq` `.imf`
`.jbm` `.ksm` `.laa` `.lds` `.m` `.mad` `.mdi` `.mdy` `.mid` `.mkf` `.mkj`
`.msc` `.mtk` `.mtr` `.pis` `.plx` `.rac` `.rad` `.raw` `.rix` `.rol` `.sa2`
`.sat` `.sci` `.sdb` `.snd` `.sop` `.sqx` `.wlf` `.xad` `.xms` `.xsm`

## Routing notes

* `.s3m` is exposed as **`.as3m`** (the AdLib variant) so it does not clash with
  [openmptplugin](../openmptplugin/README.md).
* `.sng`, `.ims`, `.mus` and `.vgm`/`.vgz` are intentionally routed to UADE / the
  Compute!'s Sidplayer player / GME instead.
* `.mad` is shared with PlayerPRO; AdPlug's Mad Tracker 2 loader is content-gated
  on magic `MAD+`, so `MADG`/`MADF`/`MADK` files route to
  [playerproplugin](../playerproplugin/README.md).
* `.dtm` is two unrelated formats — **DeFy Adlib Tracker** (ours) vs **Digital
  Tracker**; routing is by content.
* `.imf` is two unrelated formats — Apogee IMF (ours) vs Imago Orpheus
  (OpenMPT's); routing is by content.
* `.snd` — Westwood `.snd` routes to AdPlug's `CadlPlayer`.
* `.d01` (EdLib, packed) was **tested and rejected**: AdPlug's D00 loader rejects
  it two independent ways, even renamed. It is listed in
  `data/misc/not_supported_extensions.txt`.

## Vendoring

AdPlug 2.4 carries **three local patches** that must be re-applied on any future
re-vendor; see `external/VENDORED.md`.
