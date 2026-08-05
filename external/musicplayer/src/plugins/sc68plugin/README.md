# sc68plugin

Atari 16-bit music in the `.sc68` container format.

Extensions: `.sc68` `.sndh` `.snd` `.4v`

## Build gating

**ChipMachinePlus only.** SC68 is not part of the Mac App Store build (see
[`LEGAL-PLUS`](../../../../../LEGAL-PLUS)) and the 1,894 `.sc68` songs are hidden
there.

`.sndh` is unaffected: [sndhplugin](../sndhplugin/README.md) (AtariAudio, MIT)
plays it in **both** builds and declares a higher priority, leaving SC68 only as
a fallback. That swap also fixed STE DMA-sound tunes SC68 played as a single
block followed by silence.
