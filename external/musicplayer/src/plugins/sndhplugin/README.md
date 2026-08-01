# sndhplugin — Atari ST/STE SNDH

Plays `.sndh` (and content-gated `.snd`) using **AtariAudio v1.01** by Arnaud
Carré (Leonard/Oxygene), vendored verbatim under `atariaudio/`.

## Why this exists

`.sndh` used to be decoded by `sc68plugin`, which drives **libsc68 / emu68 /
file68 / unice68 — GPL-3**. GPL-3 cannot ship on the Mac App Store, so the mas
variant had to lose the plugin. `.sndh` is **6,079 of the 7,976 rows**
sc68plugin owned (76%), so replacing just that one format keeps almost all of it.

AtariAudio is a natural fit rather than a lucky find: Arnaud Carré wrote the
original **StSound**, which this project already ships as `stsoundplugin`, and
AtariAudio is a from-scratch rewrite of that YM2149 work wrapped in a small ST
machine. It is the engine behind his own SNDH-archive player.

## What's in the box

| component | author | licence |
|---|---|---|
| AtariAudio (`SndhFile`, `AtariMachine`, `Ym2149c`, `Mk68901`, `SteDac`) | Arnaud Carré | MIT |
| Musashi 68000 core (`external/Musashi/`) | Karl Stenerud | MIT |
| ICE! 2.4 depacker (`external/ice_24.c`) | Hans Wessels | public domain |

No external dependencies, no float, no data files — an SNDH carries its own 68k
driver, so unlike `.sc68` there is no replay directory to ship. That is exactly
why `.sc68` could not follow it across (see below).

Dropped at vendor time: `m68kmake.c` (the opcode-table generator — `m68kops.c` is
committed prebuilt upstream), `m68kdasm.c` and `m68kfpu.c` (unreachable in a
68000-only config).

## Build notes

Three things in `CMakeLists.txt` are load-bearing, not cosmetic:

* **`NDEBUG` on the vendored sources.** This project's Release build does not
  define it, and AtariAudio `assert(false)`s on input it dislikes — unsupported
  GEMDOS/XBIOS calls, unknown `TRAP #n`, illegal opcodes. Without `NDEBUG` one
  odd file in a 6,079-file corpus would `abort()` the app instead of skipping a
  song. Real load failures still come back through `SndhFile::Load`'s return
  value, which is what `fromFile` keys on.
* **Hidden visibility + `ld -r`.** `aoplugin`'s Saturn core is *also* a Musashi
  and exports the same un-prefixed C entry points at global scope (`nm -g
  libaoplugin.a` shows `_m68k_init`, `_m68k_execute`, `_m68k_read_memory_8` … all
  `T`). Partial-linking demotes our copy's to local symbols. Same treatment
  `mikmodplugin` needs.
* **`priority() == 2`.** Above `SC68Plugin`'s 1, which is above AdPlug's 0 on the
  shared `.snd` extension.

## Relationship to sc68plugin

Both variants use AtariAudio for `.sndh`. In the **plus** build sc68plugin is
still linked and still claims `.sndh` at lower priority, so anything AtariAudio
declines falls through to libsc68 exactly as before — that build cannot regress.
In the **mas** build sc68plugin is not compiled at all (`CM_HAVE_SC68`).

`.sc68` does **not** move here and is not portable to AtariAudio as it stands:
the container names an external replay routine, and those live in
`data/sc68/Replay/` as 95 prebuilt 68k binaries built from GPL-3 sc68 sources.
There is no permissive equivalent and no published spec to clean-room. Those
1,894 rows stay plus-only and are dropped from the mas index by
`songHasNoPlayer()` in `MusicDatabase.cpp`.

A phase 2 is conceivable — many `.sc68` files embed their driver in an `SCDA`
chunk rather than naming an external replay, and those could in principle run on
the same Musashi. Nobody has measured what fraction that is.

## Known limits

* **No seek within a subsong.** The ST replay routine is driven tick by tick from
  its own init with no fast-forward, so `seekTo` accepts subsong changes (which
  restart the machine) and declines time seeks.
* **Single live instance.** Musashi keeps CPU state in file-scope globals and
  `AtariMachine.cpp` routes its bus callbacks through one `gCurrentMachine`
  pointer. `SNDHPlugin.cpp` serialises construction and rendering on a static
  mutex; two concurrently *rendering* players would still trample each other.
  The host plays one tune at a time, and libsc68 had the same constraint.
* **Length only when tagged.** `AudioRender`'s loop counter is only meaningful
  when the file declares a length (`TIME`/`FRMS`). Untagged tunes report length 0
  and are ended by the host's own song-length/silence logic, like any other
  plugin that reports 0.
