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
   `pl_PlayMode` byte) on the **fake6502** CPU core.
3. After each tick, latch `$900A`–`$900E` into the **VIC-I** sound core and
   render 44100 Hz samples.

## Vendored components / licensing

| Component | Source | License |
|-----------|--------|---------|
| `victracker/` player sources (`player.asm`, `playerdata.asm`, includes) | Daniel Kahlin, VIC-TRACKER 2.0 (`http://www.kahlin.net/daniel/victracker/`) | BSD (see `victracker/LICENSE.txt`) |
| `fake6502.c` / `.h` | Mike Chambers / omarandlorraine fork | GPLv2 |
| `vic_sound.c` / `.h` | VIC-I core extracted from VICE `vic20/vic20sound.c` | GPLv2 |

The combined plugin is therefore GPLv2, consistent with the other GPL plugins in
this tree (UADE, the VICE bridge, maxtrax).

## Regenerating the player blob

`vtplayer_bin.h` is Kahlin's player pre-assembled (BSD) as a position-fixed
6502 blob loaded at `$2000`, so the build needs no assembler. To rebuild it after
touching the vendored `.asm` sources, install `dasm`
(`brew install dasm`) and run:

```sh
cd victracker && ./build_player.sh
```
