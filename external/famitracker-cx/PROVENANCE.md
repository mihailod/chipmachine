# famitracker-cx (vendored slice)

Source: **FamiTracker CX** by Dan Spencer (nukep) — a cross-platform fork of
jsr's FamiTracker. Upstream: https://github.com/nukep/famitracker-cx
License: mixed **GPL v2 / New BSD** (see `core/COPYING` and the per-file headers
in `famitracker-core/`).

This is the engine slice only, used by `musicplayer/src/plugins/famitrackerplugin`
to play FamiTracker `.ftm` modules (NES 2A03 + VRC6 / VRC7 / MMC5 / FDS). The
upstream GUI (`qt-gui/`), the ncurses/console frontends, the dynamic platform
sound-sink modules (`sound/`, ALSA/JACK/DSound), the boost `threadpool`, and the
WAV exporter were all dropped.

## Layout
- `famitracker-core/` — document loader (`FtmDocument`), sound driver
  (`SoundGen`), channel handlers, instruments, and `APU/` (NES sound emulation,
  incl. blargg's `Blip_Buffer`).
- `core/` — a **boost-free** minimal shim of the upstream `core` library: fixed
  `types.hpp`, the original `io`/`ringbuffer`, and a thread-free `soundsink.cpp`.

## chipmachine patches (grep `chipmachine port`)
- `core/types.hpp` — `boost/cstdint` → `<cstdint>` (removes the boost dependency).
- `core/soundsink.cpp` — rewritten as a thread-free / boost-free no-op shim; the
  player is driven synchronously, not by a real-time sink thread.
- `famitracker-core/Common.h` — `uint32`/`int32` were `(unsigned) long` (64-bit
  under LP64, corrupting APU register structs) → pinned to stdint; added the
  missing `__APPLE__` branch.
- `famitracker-core/SoundGen.{cpp,hpp}` — boost mutex/condition → `std`; added a
  synchronous, pull-model render API (`beginRender` / `renderSamples` /
  `isHalted`) so a frame can be rendered on demand without the sink thread.
- `famitracker-core/FtmDocument.cpp` — `FILE_VER` 0x0430 → 0x0440 so modules
  from the final official FamiTracker 0.4.x line load (the block readers already
  handle those block versions; only the top-level gate rejected them).

## Known gaps
Namco 163 (N163) and Sunsoft 5B modules are not driven — upstream never wired
their channel handlers (`ChannelMap.cpp`, "TODO - dan"), even though the APU can
emulate N106. Such modules fail to load rather than play incorrectly.
