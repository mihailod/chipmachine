# vgmstreamplugin

**Streamed console/PC game audio** — the hundreds of container formats decoded by
**vgmstream** (Adam Gashlin, bnnm, Christopher Snowhill and contributors).

This covers ripped in-game streams such as CRI **ADX** / **HCA**, FMOD **FSB**,
Microsoft **XWB** / **XMA**, and the many platform PCM/ADPCM wrappers (Nintendo
**DSP**, PlayStation **VAG**, Sony **AT3** / **AT9**, …).

## Extensions

`.adx` `.hca` `.fsb` `.xwb` `.xma` `.dsp` `.vag` `.at3` `.at9` `.acb` `.awb`
`.bcstm` `.bfstm` `.brstm` `.genh` `.txth` and roughly 700 more (see vgmstream's
full [extension list](vgmstream/formats.c)).

## Build

The core decode library is vendored under `vgmstream/` here and driven through
its `libvgmstream` API. It is built **without** any of vgmstream's optional
external codec libraries, so only the self-contained decoders are compiled.

## Routing

vgmstream claims a very large extension set, much of which overlaps formats
already handled elsewhere in the build. `canHandle` therefore **hard-declines**
the extensions owned by other plugins (OpenMPT trackers, GME/console chips,
FFMpeg streaming audio, the ZX AY players, …) and content-validates the rest, so
vgmstream only picks up genuinely new game-audio formats.

## Gotcha

vgmstream's `_fill` returns a **result code**, not a sample count.
