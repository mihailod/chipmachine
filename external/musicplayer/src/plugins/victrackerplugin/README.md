# victrackerplugin

Plays the modland **Vic-Tracker** corpus: Commodore VIC-20 music in Daniel
Kahlin's **VIC-TRACKER** (`.vt`) format.

## How it works

A `.vt` file is a Commodore VIC-20 PRG — a 2-byte `$3300` load address followed
by Kahlin's `T1` (or legacy `T0`) tune struct. The tracker's own 6502 replay
routine interprets that struct in place and writes the VIC-20 sound registers
`$900A`–`$900E`. We reproduce the VIC-20 IRQ loop exactly:

1. Load the tune image at `$3300` (`vt_machine.cpp`).
2. Run `pl_Init`, then `pl_Play` once per interrupt tick (rate from the tune's
   `pl_PlayMode` byte) on the **MyLittle6502** CPU core (`vt_cpu.h`).
3. After each tick, latch `$900A`–`$900E` into the **VIC-I** sound core and
   render 44100 Hz samples.

## Vendored components / licensing

| Component | Source | License |
|-----------|--------|---------|
| `victracker/` player sources (`player.asm`, `playerdata.asm`, includes) | Daniel Kahlin, VIC-TRACKER 2.0 (`http://www.kahlin.net/daniel/victracker/`) | BSD (see `victracker/LICENSE.txt`) |
| `pd6502.h` (6502, behind `vt_cpu.h`) | MyLittle6502 | public domain / CC0 |
| `vic_sound.c` (VIC-I) | written here | — |

Nothing is GPL, so the plugin ships in **both** build variants and all 11 `.vt`
songs stay in the mas index.

### Why these cores, and not the obvious ones

This plugin originally ran on the **omarandlorraine fake6502 fork** (GPLv2) and on
a VIC-I core **lifted from VICE's `vic20/vic20sound.c`** (GPLv2). That made it
unshippable on the Mac App Store, but — unlike uadeplugin, vicepluginbridge and
goattrackerplugin — the GPL was internal rather than structural: the plugin is
ours and only its two engine cores were the problem. Both were replaced in place,
and the replacements were A/B'd by ear against the originals before the GPL ones
were deleted, so there is no variant gate and nothing hidden from the index.

**`pd6502.h`** is the same code the GPL fork descends from — Mike Chambers'
Fake6502 v1.1 (2011), released into the public domain — continued by gek169 as
[MyLittle6502](https://github.com/C-Chads/MyLittle6502) with decimal-mode, `BIT`
and interrupt-masking fixes. Vendored unchanged; `vt_cpu.c` prefixes its
non-static symbols (`read6502`, `step6502`, `callexternal`, …) before including
it, since those names are far too generic for a link with ~60 other plugins.

**`vic_sound.c`** avoids the two things that made VICE's file a licensing
problem, both of them data rather than logic:

* VICE ships a **1024-byte dump** of the noise generator's output. The generator
  is documented (Asger Alstrup Nielsen): a 23-bit shift register seeded
  `0x7ffff8` whose new bit 0 is bit 22 XOR bit 13. It is *generated* here, with
  the register geometry and the `128 - ((reg + 1) & 0x7f)` divider mapping taken
  from MAME's `src/devices/sound/mos6560.cpp` (BSD-3-Clause, Peter Trauner).
* VICE ships a **~350-entry table of measured output voltages** — the authors'
  own measurement data. This uses a parametric saturating DAC instead, followed
  by a 2.5 kHz one-pole low-pass and a 20 Hz DC block.

The rest is the chip itself: three tone channels, each an 8-bit shift register
clocked at `clock/(divider << 4|3|2)` with the inverted MSB fed back (so it emits
a square wave at 1/16 of its shift rate), a noise channel on the same ladder, and
a 4-bit master volume.

The one place the two cores measurably differed was the shape of that DAC curve.
VICE's measured table rises steeply and then saturates hard; `VIC_DAC_GAIN` /
`VIC_DAC_LIMIT` are a soft knee fitted to land in the same place rather than to
trace the same path. Across the five tunes in `chipmachine/testmus/victracker`
they agreed on RMS to within about 1 dB (0.89–0.97×), with peaks 8% under full
scale, and were indistinguishable by ear. Retune those two constants if the
output ever needs adjusting — do not reconstruct VICE's table.

## Regenerating the player blob

`vtplayer_bin.h` is Kahlin's player pre-assembled (BSD) as a position-fixed
6502 blob loaded at `$2000`, so the build needs no assembler. To rebuild it after
touching the vendored `.asm` sources, install `dasm`
(`brew install dasm`) and run:

```sh
cd victracker && ./build_player.sh
```
