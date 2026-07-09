# goattrackerplugin

Plays **GoatTracker v2** native song files (`.sng`) — the C64 SID tracker by
Lasse Öörni (Cadaver). GoatTracker's own playback core is vendored and driven
directly, with no SDL / BME / editor layer:

- `gplay.c` / `gplay.h` — the GoatTracker v2 sequencer (`playroutine`), verbatim,
  patched only to include `gt2_engine.h` instead of the heavy `goattrk2.h`.
- `gsid.cpp` / `gsid.h` — GoatTracker's reSID interface (`sid_init`,
  `sid_fillbuffer`), trimmed to plain reSID only (the reSID-fp path removed).
- `resid/` — Dag Lem's reSID 6581/8580 emulator, vendored verbatim (minus
  `version.cpp`, which only exposes the autoconf `VERSION` string).
- `gt2_load.c` — the loader. GoatTracker's `loadsong` is vendored **verbatim**
  (as `loadsong_impl`), adapted only by swapping its editor-model calls
  (`clearsong` → a local `clear_arrays`, `songchange` and the table-view resets
  dropped) so it needs none of the editor modules. It handles **every** song
  version — `GTS!` (v1), `GTS2` (3-table) and `GTS3`/`GTS4`/`GTS5` — including
  the intricate v1 wave/pulse/filter/arpeggio conversion and the version-gated
  pulse-speed (<v2.4) and legato/nohr (<v2.5) fixups. This file also owns the
  song arrays, the player-config globals (fixed at GoatTracker's PAL / 6581 / 1×
  defaults), `makespeedtable` (from gtable.c) and no-op stubs for the BME timing
  / SDL flush hooks.
- `gt2_engine.h` — lightweight umbrella header exposing exactly what the
  sequencer needs.

## Format dispatch

`.sng` is heavily overloaded, so the format is claimed **by content magic
only** (`GTS!`/`GTS2`/`GTS3`/`GTS4`/`GTS5`). UADE (tried first for `.sng`)
declines non-Richard-Joseph `.sng`, and SCC-Musixx rejects `GTS`, so the three
coexist.

## Playback / end detection

reSID renders mono; the plugin drives `playroutine()` once per 50 Hz PAL frame,
fans the output out to interleaved stereo, and detects end-of-song when every
channel's order-list position has wrapped once (one full pass), on an explicit
`stopsong`, or at a 600 s cap.

## Symbols

GoatTracker and reSID export many generic un-prefixed globals (`pattern`,
`instr`, `sid`, `sidreg`, `sound_flush`, …) that collide with other vendored
plugins, so the library is compiled with hidden visibility and partial-linked
(`ld -r`) to localize everything except `musix::GoatTrackerPlugin` /
`goattrackerplugin_register` — the same approach as `mikmodplugin`.

## Credits / license

- **GoatTracker** © Lasse Öörni (Cadaver) — GNU GPL v2.
- **reSID** © 2004 Dag Lem `<resid@nimrod.no>` — GNU GPL v2.

Both components are GPL v2; this plugin is therefore GPL v2. See `LICENSE`.
