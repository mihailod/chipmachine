# s98plugin

S98 register-log music for retro FM hardware, including the **OPNA
hardware-rhythm drums**.

Extensions: `.s98`

## Chips

| S98 device | Core |
|---|---|
| PSG, OPN, OPN2, OPNA, OPM | cisc's fmgen (`m_s98/device/fmgen`), shared with [fmpplugin](../fmpplugin/README.md) |
| OPLL | Okazaki's emu2413 |
| SNG (SN76489) | NEZplug tables (`s_sng.c`) |
| OPL, OPL2, OPL3 | ymfm (`m_s98/device/s98_ymfm.cpp`) |

Almost the whole corpus is OPNA — of 5,379 `.s98` songs only **11** use the OPL
devices at all: nine OPL3 (a Sound Blaster rip) and two OPL2 (the PC-9801
"Sound Orchestra" board).

## The ymfm OPL adapter (`m_s98/device/s98_ymfm.cpp`)

Replaced MAME's `fmopl.cpp` + `ymf262.cpp` (and the `s98mame.cpp` wrapper),
which were GPL-2.0+. Applied to **both** variants, not just `mas`, because it
is also a correctness fix.

**The OPL3 high bank was unreachable.** `m_s98.cpp` encodes a port-1 write as
`0x100|reg`, which for OPL3 selects the second register bank. `s98mame.cpp`
passed that straight to `YMF262Write()` as an 8-bit address, and MAME masks the
address latch with `v &= 0xff` — so every high-bank write landed in bank 0.
Registers `0x105` (OPL3 enable) and `0x104` (4-operator mode) never arrived,
and all nine OPL3 songs played as plain 2-op OPL2. In these files the high bank
is a third of all register traffic (15,835–20,622 writes each). ymfm has a
distinct `write_address_hi()` and the adapter uses it.

**Resampling is ours now.** MAME resampled internally — `YM3812Init(clock,
rate)` takes the output rate. ymfm does not: it generates at the chip's native
rate (`clock/72` for OPL2, `clock/288` for OPL3) and leaves conversion to the
caller. The adapter box-filters down to the output rate; both chips run above it
here (55466 Hz and 50000 Hz vs 44100 Hz), so plain decimation would alias.

**Output mapping.** OPL2 is mono and feeds both channels. OPL3 has four lanes;
`L = 0 + 2`, `R = 1 + 3`, matching what `s98mame.cpp` did.

**Its own ymfm copy.** Not shared with libvgmplugin: that target merges its
objects with `ld -r`, which turns its ymfm template instantiations into private
externs — invisible to other targets, and by the same token unable to collide
with this second copy.

A/B against the old core: OPL2 agrees within **1.6% RMS**; OPL3 differs, mostly
from the bank fix above.

## Licensing

Built identically in both variants — there is no `mas` gate. Full terms are in
[`LEGAL`](../../../../../LEGAL); in short:

* **ymfm** — BSD-3.
* **fmgen** — cisc's own terms, which require that distribution be as freeware.
  See `fmgen/readme.txt` (kept unaltered, as its licence requires) and
  `fmgen/MODIFICATIONS.md`.
* **The OPNA rhythm voices are synthesized, not Yamaha's.** The YM2608 ADPCM-A
  rhythm ROM has been removed; `fmgen/gen_rhythm_synth.c` generates the six
  voices from scratch, matched to the originals in count, order, length and
  level but not in timbre. An external `2608_BD.WAV .. 2608_RIM.WAV` set still
  takes priority, so a user with the real ROM gets authentic drums.
* **Open item** — the `m_s98` plugin body, emu2413 and the NEZplug tables carry
  no licence statement at all and have not been pinned to a quotable upstream
  release.
