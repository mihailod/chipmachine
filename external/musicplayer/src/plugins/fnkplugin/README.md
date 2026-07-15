# fnkplugin

Plays **Funktracker** modules (`.fnk`) — Elias Ehlin's MS-DOS tracker (1994-96),
released as *FunktrackerGOLD* and *Funktracker DOS32*. A sample tracker aimed at
funk/hiphop, 4–32 channels. Files begin with the ASCII magic `Funk`, followed by
a creation year, CPU/soundcard nibbles and a stored file size.

Covers modland's `Funktracker/` dir (7 tunes).

## Engine: libxmp `fnk_loader`

Playback uses **libxmp**'s Funktracker loader (`fnk_load.c`), already vendored in
the tree under `zxtune/3rdparty/xmp/`. Rather than build the full ~58-format
libxmp or depend on the shared zxtune libxmp target, this plugin compiles a
**minimal single-loader slice** directly into `libfnkplugin.a` — the same shape
as `musxplugin` (see its README for the rationale):

* core libxmp (player/mixer/scan/effects/…) + `loaders/{common,iff,sample,
  mmd_common,voltable,fnk_load}.c`
* same trimmed build defines zxtune uses
  (`NO_COMPOSITE_LOADER`, `NO_PROWIZARD`, `NO_EXTERNALFILES`, `DECRUNCH_MAX=0`, …)
* `MusxPlugin`-style `xmp_load_typed_module_from_memory(..., &fnk_loader)` drives
  the one loader directly, so `format.c`'s auto-detect table (which references
  every other loader) is never pulled in by the linker.

`canHandle` gates on the `.fnk` extension **and** the `Funk` magic. `fnk_test()`
checks more (creation year, CPU/card nibbles, and a stored size that must equal
the real file size); a file that passes the magic but fails `fnk_test` simply
throws in `fromFile`, so the host reports an error instead of a silent play.

## Verification

All 7 modland tunes were rendered through the loader and checked for actual
audio (not just a successful load): **7/7 play, none silent, none rejected**.
This was done with libxmp's asserts **live**, which matters — this tree
overrides `CMAKE_C_FLAGS` and never defines `NDEBUG`, so a libxmp assert
`abort()`s the app. Funktracker trips none of them.

> Contrast **Liquid Tracker** (`.liq`), triaged at the same time and *rejected*:
> its libxmp loader desyncs on 5 of 13 tunes and aborts the process. It is
> documented in `data/misc/not_supported_extensions.txt`.

Fixtures: `testmus/fnk/` (`battleship.fnk` = FunktrackerGOLD,
`encountered.fnk` = Funktracker DOS32, so both header variants stay covered),
driven by `TEST_CASE("Funktracker")` in `test.cpp`.
