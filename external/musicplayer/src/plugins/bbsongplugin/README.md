# bbsongplugin — Beepola

ZX Spectrum **1-bit beeper** music authored in **Beepola**.

Extensions: `.bbsong`

## How it works

Each `.bbsong` is compiled into its engine's data format, and that engine's
original Z80 player is run on an in-process Z80 core (48K ROM mapped, IM2
interrupts) while the 1-bit speaker (port `0xFE`) is sampled to PCM.

## Supported engines

* **SFX** (Special FX / Fuzz Click)
* **Phaser1** (`P1D`, `P1S`)
* **Music Box** (`TMB`)
* **Music Studio** (`MSD`)

For the Shiru engines the player is assembled in-repo from vendored Z80 source by
a small vendored Z80 assembler. For SFX, the player and its complete compiled
bytecode format (tone, sustain and percussion) are reproduced from Beepola itself
and validated byte-for-byte.

This covers ~92% of the Beepola songs on modland.

## Work in progress

The **Savage** engine, and Music Studio's low bass range / percussion.

## Related

Other 1-bit beeper work: [monotoneplugin](../monotoneplugin/README.md) (IBM PC
speaker). Picatune2 `.pt2` is parsed but has no shipped replayer (the MVP would
be Shiru's Z80 player) and collides with ProTracker 2 `.pt2`.
