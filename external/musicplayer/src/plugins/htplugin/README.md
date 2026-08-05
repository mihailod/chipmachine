# htplugin — High Technology

Sega **Dreamcast** and **Sega Saturn** music.

Extensions: `.ssf` `.dsf` `.minissf` `.minidsf`

## Engine

Neill Corlett's SegaCore, as maintained in kode54's *Highly Theoretical*.

## Build gating

**ChipMachinePlus only.** The engine is not part of the Mac App Store build and
those 273 songs are hidden there (see [`LEGAL-PLUS`](../../../../../LEGAL-PLUS)).

A clean-room replacement is conceivable — the CPU halves are already available
permissively — but SCSP/AICA is a 32/64-channel Yamaha part with a real DSP,
i.e. an emulator project in its own right.

Saturn `.ssf`/`.minissf` are also declared by [aoplugin](../aoplugin/README.md)
at a lower priority; with this plugin gated out, Saturn is absent from the App
Store build entirely.
