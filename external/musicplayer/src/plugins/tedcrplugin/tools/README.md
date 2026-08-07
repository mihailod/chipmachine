# tedcrplugin verification harness

Not built by CMake — run by hand when the core or the machine changes.

## Sound core

`ted_test.c` drives `ted_sound.c` and dumps raw 16-bit mono 44.1 kHz PCM;
`verify_core.py` renders through it and checks the whole measured specification
(frequency law across the register range, both lock-up quirks, volume linearity
and clamping, channel gating and summing, noise period/duty/step rate, D/A mode).

```sh
cc -std=c11 -O2 -Wall -Wextra -I.. -o ted_test ted_test.c ../ted_sound.c -lm
python3 -m venv .venv && .venv/bin/pip install numpy
.venv/bin/python verify_core.py
```

Exits non-zero on any failure, prints `all checks passed` otherwise.

## Machine

`ted_run.cpp` plays a `.prg` through the whole machine to stdout as raw mono PCM:

```sh
cc  -std=c11   -O2 -w -c ../ted_cpu.c   -o ted_cpu.o
cc  -std=c11   -O2 -w -c ../ted_sound.c -o ted_sound.o
c++ -std=c++17 -O2 -Wall -Wextra -I.. -o ted_run ted_run.cpp ../ted_machine.cpp \
    ted_cpu.o ted_sound.o -lm
./ted_run tune.prg 8 > tune.raw
```

`TED_DIAG=1 ./ted_run tune.prg 3 >/dev/null` prints a diagnosis instead: the
instruction, interrupt and sound-write counts, the raster compare and interrupt
mask, where the PC has been, and the bytes around wherever it ended up. Every
machine bug found so far showed up as `soundwr=0` plus a PC parked in a loop,
with the bytes around it naming exactly what the tune was waiting for.

## Differential comparison

`compare.py` renders each file through both the tedplay engine and this machine,
aligns them (the engine spends about a second booting and running BASIC's RUN
first) and scores the mean cosine similarity of their log spectra:

```sh
.venv/bin/python compare.py <dir-of-prg-files> 8
```

It needed a `probe/render` binary built from the tedplay sources. **tedplay was
deleted on 2026-08-06, so this script no longer runs** — it is kept because it
documents how the whole-corpus result in `../README.md` was produced. What
remains usable is `ted_run` (on its own, for listening and for `TED_DIAG`) and
the fixtures.

The scripts that produced the sound specification in the first place —
generating `.prg` programs that poke the registers, and solving for the noise
shift register — were scratch work against that same engine and are not kept.
`../README.md` records what they measured.
