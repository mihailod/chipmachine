# csidplugin — cSID

Commodore **C64** SID music (6581/8580, including 2SID/3SID tunes), via **cSID**
(WTFPL).

Extensions: `.sid` `.rsid`

## Why cSID

cSID replaced VICE as the SID engine in **both** builds. Being permissively
licensed, it keeps ~59.9k of the ~62k `.sid` tunes playing in the Mac App Store
build, and it removes the dependency on Commodore's copyrighted KERNAL / BASIC /
chargen ROM images that VICE needs.

## The silent-RSID gate (MAS only)

The exception is an RSID whose header play address is `$0000`: it installs its
own IRQ/NMI handler and expects a real C64 (KERNAL banked in, CIA and raster
running), which cSID does not emulate — so it renders as dead air.

Those files are enumerated **by measurement** in
`data/misc/csid_silent_sids.txt` and dropped at index time in the MAS build only,
so no unplayable row ever surfaces. Plus is unchanged: it plays all of them
through VICE and never reads the list.

The list is measured rather than inferred, because the obvious rule would be
badly wrong. Every `play=$0000` file was rendered through the same engine and the
same 3 s / peak>64 test the player uses at runtime:

| | files | wholly silent | wholly fine | mixed subtunes |
|---|---|---|---|---|
| RSID `play=$0000` | 3708 | 2363 | 1242 | 103 |
| PSID `play=$0000` | 109 | 19 | 90 | — |

Hiding every `play=$0000` RSID would have hidden **1,345 files that play
perfectly well**. Only the 2,382 wholly-silent ones are listed; the mixed files
stay indexed and the runtime probe in `CSIDPlugin` skips their silent subtunes
one at a time.

## Related

* [vicepluginbridge](../vicepluginbridge/README.md) — VICE, Plus only.
* [musplugin](../musplugin/README.md) — the clean-room Compute!'s Sidplayer
  sequencer that runs on top of this chip emulation.
* [goattrackerplugin](../goattrackerplugin/README.md) — C64 `.sng`.
