# vicepluginbridge — VICE

**Compute!'s Sidplayer** — a C64 *note* format with its own player, not a SID-chip
dump. A stereo tune is a `.mus` (voices 1–3) plus a `.str` companion (voices
4–6), which the plugin loads together.

Extensions: `.mus` `.str`

## Build gating

**ChipMachinePlus only** (see [`LEGAL-PLUS`](../../../../../LEGAL-PLUS)). VICE is
GPL and ships Commodore's copyrighted KERNAL / BASIC / chargen ROM images.

The Mac App Store build plays the same ~6.5k songs through the clean-room
[musplugin](../musplugin/README.md), which is registered *after* this plugin —
so the Plus build's routing is unchanged and keeps using VICE.

VICE is also no longer the `.sid` engine in either build; see
[csidplugin](../csidplugin/README.md).

## Vendoring

The bridge targets VICE **3.10** (`external/vice310/`). The bridge API is only
valid up to VICE 3.1-era internals; the 3.10 migration required reentrant
`state_*` handling.
