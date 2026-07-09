# Vendored PlayerPRO MADDriver slice

Source: **MaddTheSane/PlayerPRO** (a maintained mirror of Antoine Rosset's
SourceForge project), directory `MADDriver.source/`.

License: **Public Domain** — *"The code 'PlayerPRO' has been released to the
Public Domain by the original author, Antoine ROSSET."* (upstream `LICENSE.txt`).

This is the minimal slice needed to load and render PlayerPRO modules offline.
It is driven by chipmachine's `playerproplugin` with the `NoHardwareDriver`
output mode + `MADDirectSave()` pull loop (no live audio backend, no GUI).

## Files

Core software-synth engine + driver (unmodified upstream):
`DelayOutPut.c Effects.c Interrupt.c MADDriver.c MainDriver.c OutPut8.c
realft.c TickRemover.c MyDebugStr.c MIDI-Hardware-Stub.c stub-VSTPlugIn.c
FileUtils.c` and the headers `MAD.h MADDefs.h MADFileUtils.h MADPlug.h
MADPrivate.h RDriverInt.h RDriverCarbon.h VSTFunctions.h PlayerPROCore.h
embeddedPlugs.h`.

Format loader (unmodified upstream, from `Import-Export/MADfg/`):
`MADfg.c MADfg.h` — handles the older `MADF`/`MADG` PlayerPRO modules, which is
what the Modland `PlayerPro/` tree contains. Compiled with `-DEMBEDPLUGS=1` so
its entry point is the embedded `mainMADfg()` (not a dlopen'd `PPImpExpMain`).

## Local additions (chipmachine-specific, not from upstream)

- **`PPEmbeddedPlugs.c`** — replaces the stock `Lin-PlugImport.c`/`OSX-PlugImport.m`
  loader registry. Instead of scanning a folder and `dlopen()`ing `.so`/bundle
  plugins, it registers the compiled-in `mainMADfg` loader statically. The
  generic dispatch helpers (`PPImportFile`, `PPTestFile`, `CheckMADFile`, ...)
  are copied verbatim from `Lin-PlugImport.c`.
- **`PPStubs.c`** — trivial stubs for `initCoreAudio`/`closeCoreAudio`/`SetOSType`.
  On `__APPLE__` the engine auto-`#define`s `_MAC_H` (keeping its well-tested Mac
  IO/struct paths), which references these Mac live-audio / Finder-metadata
  symbols. chipmachine never starts CoreAudio (offline render) and never writes
  modules, so the stubs are inert and avoid linking AudioToolbox/AudioUnit and
  the Objective-C `CocoaFuncs.m`.

## Local patches to upstream files (grep `PPRO_PORTABLE_PLUG`)

- **`MADDriver.h`** — `#include <PlayerPROCore/RDriver.h>` (framework-style) is
  switched to a local `#include "RDriver.h"` under `PPRO_PORTABLE_PLUG`.
- **`RDriver.h`** — the portable (non-Carbon) `PlugInfo` struct selector is
  widened from `__ELF__` to also fire under `PPRO_PORTABLE_PLUG`. (Currently
  moot because `_MAC_H` selects the CoreFoundation `PlugInfo` on Apple, but it
  keeps the slice buildable on a non-Apple/non-ELF target too.)

Build defines: `-DEMBEDPLUGS=1 -DPPRO_PORTABLE_PLUG`; link `CoreFoundation`.

**Required flag: `-fsigned-char`.** The engine reads 8-bit PCM sample data
through plain `char`, assuming it is signed. The umbrella project builds with
`-funsigned-char`, under which playback is badly clipped/garbled (a DC offset +
~3.5x amplitude). The plugin CMakeLists forces `-fsigned-char` on these sources.
