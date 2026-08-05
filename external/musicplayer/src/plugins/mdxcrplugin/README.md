# mdxcrplugin — clean-room MDX (work in progress)

A **clean-room** engine for Sharp X68000 MDX, intended to replace the Plus-only
[mdxplugin](../mdxplugin/README.md) (mdxmini) and to bring the ~6.9k `.mdx` songs
into the Mac App Store build.

**Not yet wired into playback.**

## Why clean-room

Every off-the-shelf `.mdx` player is either GPL or a disassembly of the original
driver (GAMDX), so neither can be linked into the App Store build. The chip
emulation itself is not the problem — the YM2151 (OPM) core in the tree is
already BSD-3. What is missing is the *driver*: the MML sequencer, roughly 3,900
lines.

## Current state

`cmdx` is an OPM-only sequencer. The parser passes **897/897** corpus files and
reproduces **267 events exactly**. Because verification is done against a trace
oracle rather than by ear, no chip is needed to check correctness.

## Trace harness

The oracle lives at `tools/mdxtrace`. Notes:

* The NLG trace format cannot see ADPCM, so PCM-driven tunes are out of scope
  for trace comparison.
* `verify_inert.sh` requires `-g0`.
