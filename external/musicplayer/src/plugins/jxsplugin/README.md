# jxsplugin

Plays **JayTrax** music (`.jxs`) — a cross-platform software synthesizer +
tracker by Reinier "Rhino" van Vliet (the engine was originally called
*Mugician*; the desktop/PocketPC app is *JayTrax* / *Syntrax*). Each instrument
is either a sample or a set of synth waveforms driven by AM / FM / pan /
arpeggio modulators, mixed across up to six stereo channels with a stereo echo.
A module may contain several subsongs.

This is **not** an Amiga format and does **not** go through UADE.

## Detection

The format has no string magic. A song begins with a 16-bit little-endian
`mugiversion` tag at offset 0, followed by a 16-bit zero pad. `canHandle()`
gates on the `.jxs` extension and then confirms the header is **3456** or
**3457** (the revisions the replayer supports — `3458` is reserved and
unimplemented upstream) with the pad word `== 0`. The extension check keeps it
from claiming unrelated `.jxs` files (e.g. JPEG XS).

## Playback

The whole song is rebuilt from the file buffer with `jxsfile_readSongMem()` and
rendered with `jaytrax_renderChunk()`, which mixes interpolated int16 stereo at
an arbitrary frequency — we render at **44100 Hz**. `getLength()` (which walks
the song to its first loop point) gives the `length` metadata; subsongs are
exposed via `songs` / `seekTo()` → `jaytrax_changeSubsong()`. There is no file
or hardware emulation in the plugin itself; it just feeds bytes and pulls
samples.

## Provenance

The decode engine is the public C port of Rhino's own replayer
(<https://github.com/pachuco/jaytrax>, itself a port of
<https://bitbucket.org/rhinoid/crossx>), vendored at repo-root `jaytrax/` —
see `jaytrax/PROVENANCE.md`. Only the `lib_oldjaytrax/` library is built
(`jaytrax.c`, `jxs.c`, `mixcore.c`); the upstream live-audio CLI harness is not.

One local change to the vendored source: three file-scope lookup tables in
`jaytrax.c` were made `static` to avoid a link-time symbol collision with
`eupplugin` (details in `jaytrax/PROVENANCE.md`).
