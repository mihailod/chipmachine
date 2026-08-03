# gsfplugin — Game Boy Advance `.gsf` / `.minigsf`

Engine: **mGBA** (Jeffrey Pfau / "endrift" and contributors), **MPL-2.0**,
vendored verbatim under `mgba/`. Container loader: **ours**, `GsfRom.cpp`.

Replaced a vendored **VisualBoyAdvance** (GPL-2.0-or-later, ~10.5k LOC under
`playgsf/VBA/`) on 2026-08-01. That was the last unconditional GPL dependency in
the Mac App Store binary, and it is the only console format on the GPL list that
had a permissive drop-in — so unlike 2SF/USF/SSF/DSF, **no songs were gated**.
`gsfplugin` builds identically in both variants and has no `CM_HAVE_*` switch.

## What is vendored

`mgba/` is a pruned copy of the upstream tree: `include/`, `src/{arm,core,
feature,gb,gba,util}`, `src/platform/posix`, `src/third-party/inih`, plus
`LICENSE`/`README.md`/`CHANGES`. The Qt/SDL frontends, the libretro core, the
other platform ports, `cinema/` (36 MB of test video) and `src/third-party/zlib`
(24 MB) are not imported. Upstream commit is recorded in `external/VENDORED.md`.

`mgba-version.c` is **ours**, not upstream: mGBA's CMake generates `version.c`
by shelling out to git, and a vendored snapshot has no git metadata.

## The trap that cost a whole session

`GBACoreCreate()` returns a valid pointer whose `init`/`reset` read as garbage,
and the first virtual call jumps into hyperspace. It looks exactly like an
ABI or LTO problem. It is not.

Several of mGBA's feature macros change the **layout** of `struct mCore`:

| defines | `sizeof(struct mCore)` | `offsetof(init)` |
|---|---|---|
| none | 1352 | 704 |
| `ENABLE_VFS` + `ENABLE_DIRECTORIES` + … | 2432 | 1784 |

`ENABLE_VFS && ENABLE_DIRECTORIES` adds `struct mDirectorySet dirs`;
`MINIMAL_CORE` removes `struct mInputMap inputMap`; `ENABLE_DEBUGGERS` pulls in
`mgba/debugger/debugger.h`. If a translation unit that includes
`<mgba/core/core.h>` disagrees with the compiled library, every function pointer
is read from the wrong offset.

The defence is structural: **one** `MGBA_DEFINES` list in `CMakeLists.txt`,
applied to the whole object library, so the engine and `GSFPlugin.cpp` cannot
diverge. Never compile anything that includes mGBA headers outside that target.

(The original spike hit this because it passed its defines through a shell
variable and the shell here is zsh, which does not word-split — the entire
`-D...` list arrived as a single argv token, so *none* of them applied. Its
"sizeof is 1352 either way" measurement was taken in that same broken TU.)

## Other things that are not optional

* **`mLogSetDefaultLogger()` before `core->init()`.** mGBA calls the default
  logger *during* init; unset is a wild jump, not a silent no-op.
* **A video buffer, even though this is audio-only.** mGBA's software renderer
  writes scanlines unconditionally and will store through a null pointer.
* **A `volume` config default.** `core->opts` is zero-initialised and
  `mCoreLoadConfig` only overwrites keys that exist, so the GBA core otherwise
  sets `masterVolume = 0` and every rip renders perfect silence.
* **`NDEBUG` on the vendored sources.** This project's Release build does not
  define it; mGBA asserts on states a malformed rip can reach.

## Cartridge vs. multiboot

`mCore::loadROM` dispatches to `GBALoadMB` or `GBALoadROM` via `GBAIsMB()`, a
heuristic: "no bigger than EWRAM, and the word at the multiboot magic offset
decodes to a branch into EWRAM". Measured over an 86-file corpus, it **misfires
on ordinary cartridge rips that happen to be small** — Radar Mission (128 KB)
and the GHX player samples (128 KB) were both being executed out of EWRAM.

The GSF program header states the real answer (load address `0x08xxxxxx` =
cartridge, `0x02xxxxxx` = multiboot), so `GsfRom.cpp` uses that and expresses it
in the only currency `GBAIsMB` reads — size. Cartridge images are padded to at
least 512 KB (the smallest real GBA mask ROM, comfortably over EWRAM); multiboot
images are never padded past EWRAM. `testmus/gsf/` carries a fixture for each.

## Licence hygiene

`GsfRom.cpp` is written from Neill Corlett's published `psf_format.txt` and the
GSF addendum. **Do not** lift anything from the old `playgsf/gsf.cpp` or from
VBA's `Util.cpp`, even for reference — keeping this file clean is the entire
reason the plugin can ship in the App Store build. (It is also simply better:
VBA's loader flattens the `_lib` chain and then `memcpy`s it to offset 0,
ignoring the load address, and it leaks state between songs so the image it
assembles depends on what was played before.)

## A/B against the engine it replaced (2026-08-01, 86 modland rips)

| | result |
|---|---|
| load success | 86/86 both engines |
| assembled cartridge image | byte-identical on 85/86 (the 86th is the multiboot rip, where the two engines load to different memory by design) |
| rendered silence | 1 file, silent under **both** (a genuinely silent track) |
| files losing signal coverage vs VBA | **0** |
| clipping | none introduced, 0 clipped samples either side |
| RMS ratio mGBA/VBA | median 1.128, p10 1.118, p90 1.525 |
| lag-aligned envelope correlation | median 0.937 |

The residual level differences are engine mixing, not a missed tag — only 2 of
the 86 files carry a PSF `volume` tag and none of the outliers do. The
low-correlation tail is a metric artefact: those files have near-flat envelopes
(median coefficient of variation 0.234 vs 0.401 for the rest), where Pearson
correlation carries almost no information.
