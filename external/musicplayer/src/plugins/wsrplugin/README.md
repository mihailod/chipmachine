# wsrplugin — WonderSwan

Bandai **WonderSwan** / WonderSwan Color music (`.wsr`), played by emulating the
machine.

Extensions: `.wsr`

| directory | what | licence |
|---|---|---|
| `v30mz/` | NEC V30MZ CPU, vendored from **ares** | **ISC** — see `v30mz/PROVENANCE.md` |
| `wswan/` | the machine and the APU, written for this project | chipmachine's own |

It replaces the **in_wsr** replayer that used to live here — Mamiya's Winamp
plugin, cut from OSWAN 0.70 and GPL-2-or-later, the same code Kodi's
`audiodecoder.wsr` ships. See `wswan/README.md` for how the machine works, what
the documentation says, and the four things that cost real debugging time.

`wswan/ws_apu.c` is **shared with `libvgmplugin`**, which plays the same chip
inside VGM logs.
