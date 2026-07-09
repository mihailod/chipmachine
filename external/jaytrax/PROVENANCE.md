# Vendored JayTrax replayer slice

Source: **pachuco/jaytrax** (https://github.com/pachuco/jaytrax) — a port from
C++ to C of Reinier "Rhino" van Vliet's own JayTrax/CrossX replayer
(https://bitbucket.org/rhinoid/crossx, `CrossX/Audio/`).

Vendored at commit `1c2a999014058ef04973c4f3fac8bdf61a3fa9a4`.

License: **No explicit license stated upstream.** The replayer source was
publicly released by the original author (Rhino) and is reused by other players
(e.g. kode54's foobar2000 input, arnaud-neny/rePlayer). It is vendored here on
the same basis. If a definitive license surfaces it should be recorded here.

This is the `lib_oldjaytrax/` library only — the minimal set needed to load a
`.jxs` module from memory and render it to int16 stereo PCM offline. The
upstream `clitools/` (WinMM/CoreAudio live-output harness) is **not** vendored.
It is driven by chipmachine's `jxsplugin`.

## Files (from upstream `lib_oldjaytrax/`)

- `jaytrax.c` / `jaytrax.h` — the replayer: song/voice state, effects, the
  `jaytrax_*` API (`init`, `loadSong`, `changeSubsong`, `renderChunk`,
  `getLength`, `free`).
- `jxs.c` / `jxs.h` — the `.jxs` file loader. We use `jxsfile_readSongMem()`
  (load from a memory buffer); the `mugiversion` tag at offset 0 selects the
  format revision (3456 / 3457 supported; 3458 reserved/unimplemented upstream).
- `mixcore.c` / `mixcore.h` — the interpolating sample mixer.
- `ioutil.h` — the small `enum loadErr` used by the loader.

`UPSTREAM-README.MD` is the upstream README, kept for reference.

## Local modifications

- **`jaytrax.c`**: the three file-scope lookup tables `frequencyTable`,
  `sineTab` and `isStaticInit` were made `static`. They are private to this
  translation unit, but as external symbols `frequencyTable` collided at link
  time with `eupplugin`'s own `frequencyTable` in the full chipmachine link.
  No behavioural change.
