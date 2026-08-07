# tedcrplugin

Clean-room Commodore **264 series** (C16 / 116 / Plus/4) **TED** music, replacing
`tedplugin`/tedplay.

**Status: shipping in both builds. tedplay is gone.** Validated over the **whole
corpus**: of 1,052 files where a comparison against the engine is meaningful,
**1,037 match at cosine ≥ 0.90**, median 0.990.

Two caveats on that number, both worth knowing before trusting it. It is measured
over an **8-second window**, so it says nothing about tunes that break later — a
separate 60-second check found **75 files that settle into a stuck tone where the
engine keeps playing**. And the reference is tedplay, which is not ground truth:
it renders silence for 98 of these files and a drone for 8 more.

`tedplugin/` was deleted on 2026-08-06 — 61 files, including the 385 KB `roms.h`
holding the Commodore BASIC, KERNAL and 3-plus-1 ROM images. Both binaries were
checked afterwards: no tedplay symbols, and none of the four ROM images appears
anywhere in either one. `.prg` now has a single claimer.

| file | what it is |
|---|---|
| `ted_sound.c/.h` | the TED sound generator, written against a measured specification |
| `ted_cpu.c/.h` | the 7501, over the vendored public-domain `pd6502.h` |
| `ted_machine.cpp/.h` | memory map, TED register page, timers, raster, interrupts, banking, and a synthetic high page in place of 385 KB of Commodore ROM |
| `TEDCRPlugin.cpp/.h` | the plugin, and the `.prg` content gate |
| `tools/` | the verification and differential harnesses |

## Why this exists

`tedplugin` vendors **tedplay** (Attila Grósz). Upstream
(`github.com/calmopyrin/tedplay`) has **no licence file at all** — the only
licence statement anywhere in the project is in `win/tedplay.rc`, a Windows
resource file in a directory that is never compiled, and it reads *"free
software … under the terms of the GNU General Public License … either version 2
… or (at your option) any later version."* Strong copyleft at best, no grant at
worst, and it is linked into the shipped App Store binary.

Two things kept it hidden: that notice lives only in a non-compiled `.rc`, and
`Tedmem.cpp` is ISO-8859 encoded, so plain `grep` treats it as binary and prints
nothing — always sweep this tree with `grep -a`.

There is a second, independent problem. tedplay reaches the chip only through a
whole emulated machine, so it also `#include`s `roms.h`: 385 KB containing four
**Commodore-copyrighted ROM images** (`basic`, `kernal`, `plus4hi`, `plus4lo`,
16 KB each), undisclosed, shipped as live data in both build variants.

## No non-GPL TED player exists

Surveyed before starting: **plus4emu** GPL-2.0, **VICE** `xplus4` GPL-2,
**C16_MiSTer** (Verilog TED) GPL-3, **Furnace** GPL-2, **YAPE**/tedplay as above.
`davervw/simple-emu-c64` is MIT-ish but has no sound at all.

The single permissive TED implementation anywhere is MAME's
`src/devices/sound/mos7360.cpp` (**BSD-3-Clause**, Curt Coder) — but it is a
MAME `device_t` needing `emu.h`/`screen.h`/`sound_stream`, and its sound is a
host-sample-rate square approximation (`samples = sample_rate / TONE_FREQUENCY`)
rather than the real counters, so the D/A tricks TED music leans on would not
survive. Same position `vic_sound.c` was in: usable as a reference, not a
replacement.

## How `ted_sound.c` was derived

The register map is published — the C16/Plus4 memory map and Plus/4 World's TED
register reference. Everything else was **measured through the chip's own
register interface**, not read out of tedplay:

Tiny `.prg` programs were generated (the same shape every HVTC file has: load at
`$1001`, one-line BASIC `SYS` stub, machine code at `$100D`) that `SEI`, poke the
sound registers, and spin. Those were rendered through the engine being replaced
and the audio analysed. The core was then written from the resulting
specification, and re-verified by pointing the same measurement scripts at it.

What that established:

| | |
|---|---|
| counter clock | CPU clock / 4 — PAL `886723.75/4 = 221681`, NTSC `894886.25/4 = 223722` |
| reload divisor | `D = (1022 - N) & 0x3FF` for the 10-bit register value `N` |
| square output | `f = clock / (2D)`, matched to five significant figures for `N` = 0…1023 |
| `N = $3FE` | `D = 0` — the source locks up, exactly as the register reference documents |
| `N = $3FF` | wraps to `D = 1023`, the *lowest* tone (108.35 Hz PAL) — the other documented oddity |
| a locked source | holds its output **high**, which is what makes D/A playback work |
| volume | linear over 0…8, completely flat for 9…15 |
| channels | sum linearly; `$FF11` bits 4/5 gate them, and the volume path is gated too |
| noise | 8-bit shift register, left shift, feedback `NOT(b7^b5^b4^b1)` into bit 0, output bit 0, natural period **255**, advancing once per counter reload |
| bit 6 + bit 5 both set | **noise** wins (the register reference claims the square does; the chip disagrees) |

The noise sequence was recovered a bit at a time from the audio — reading
transition *directions* at each step boundary, since the output is AC-coupled and
absolute levels decay — giving a clean 256-bit capture (zero ambiguous
boundaries, consensus over 16 repeats). An exhaustive search over 8-bit
registers, both shift directions, all tap masks, both feedback polarities, all
output bits and all seeds found the recurrence above to be the **only** one
consistent with it. It is generated at runtime, never tabulated.

One deliberate divergence: the measured audio repeats every **256** noise steps,
but the register's own cycle is 255. That extra step is an artifact of the engine
holding the sequence in a 256-entry table and indexing it `& 0xFF`, so the last
entry wraps into a discontinuity. This core free-runs the register at its natural
255, which is what the hardware does.

### Verification

`ted_sound.c` reproduces the whole measured specification — 40 checks covering
the frequency law across the register range, the two lock-up quirks, volume
linearity and clamping, channel gating and summing, the noise period/duty/step
rate, and D/A mode. A/B'd against the engine on identical settings:

* square wave harmonics at `N=896`: `0.344 / 0.186 / 0.146 / 0.097` vs the
  engine's `0.345 / 0.191 / 0.148 / 0.097` (3rd/5th/7th/9th, fundamental = 1)
* noise spectral envelope: within ~5% across eight octave bands

`TED_DAC_SCALE` sets the output level, calibrated so the plugin layer needs no
gain of its own — see the note on it in the source. `TED_HIGHPASS_HZ` must stay
low: D/A digis *are* low-frequency volume writes, and a lazy DC block eats them.

## The machine

`ted_machine.cpp` is a 264-series machine cut down to what music needs: 64K of
RAM, the TED register page, three interval timers, the raster counter and its
interrupt, ROM/RAM banking, and a synthetic high page. The 7501 is an ordinary
NMOS 6502 plus the `$00`/`$01` on-chip port, which lives in the memory hooks, so
`pd6502.h` needs no modification — the same vendored public-domain header
`victrackerplugin` uses, byte-identical, prefixed on the way in.

### Timing, also measured

A raster line is **114 master cycles**. The CPU runs at that same rate, one CPU
cycle per master cycle, and what varies is how many of the line's cycles the TED
leaves it. Measured by instrumenting the engine to count CPU cycles per raster
line directly:

| line | CPU cycles |
|---|---|
| blank (outside the display window) | **109** — DRAM refresh takes the rest |
| display, ordinary (6 of every 8) | **65** — refresh plus the video fetch |
| display, bad line (2 of every 8) | **22** — the character/attribute fetch |

Bad lines run in pairs aligned to the vertical scroll offset in `$FF06`. Sound
counters run at master/8, timers at master/2.

**The per-line split matters far more than the frame total.** An earlier model
spread the same total *uniformly* — 54.5 cycles on every display line — and
reproduced the frame to within 0.1% (22,890 against 22,860) while being wrong in
a way that broke tunes. A player that syncs to a raster line and does ~53 cycles
of work fits inside a real 65-cycle line and finishes while the beam is still on
it; in 54.5 it does not, and waits a whole extra frame. Aggregate agreement is
not timing agreement, and the frame-total calibration that "passed" was what hid
this for so long.

### No ROMs

Measured over a 1-in-7 sample of the 1,031 HVTC rows before any of this was
written: 72% of tunes execute no ROM at all once playing, and 16% enter ROM only
at `$FCB3`, the hardware IRQ vector, which is seven instructions. The synthetic
high page is `$8000-$FFFF` filled with `RTS` — so a stray `JSR` returns instead
of running into garbage — plus a handful of short routines at the addresses the
real KERNAL puts them, and the three vectors. Around a hundred bytes in place of
385 KB.

Those routines are re-implementations, not copies: an interrupt entry that saves
the registers and dispatches through the RAM vector, the raster-hook alternation,
the `SOUND` duration counters, and the RAM-under-ROM read helpers. Each is short,
mechanical, and has essentially one possible spelling on this machine. They sit
at the KERNAL's addresses because that is where tunes jump.

`$FFD2` (CHROUT) and `$FF4F` (PRIMM) are the only KERNAL jump-table entries the
corpus calls, both cosmetic title text, and both are host traps rather than 6502:
PRIMM has to walk the inline string after the `JSR` and rewrite the return
address, and doing that in 6502 would need zero-page scratch that is not ours.

The entry point comes from the BASIC `SYS` stub, parsed and jumped to directly.
tedplay instead injects `JSR $8BBE / JMP $8BDC` to run BASIC's RUN routine, which
is the only reason it needs a BASIC ROM at all.

### What the differential harness found

Every bug was the same shape: **state a real machine has because it booted**,
which this one does not have, because it jumps straight to `SYS`. None was
findable by reading code — each showed up only as `soundwr=0` with the PC parked
in a loop, and the bytes around that loop named what the tune was waiting for.
Between them they were worth 12% of the corpus.

1. **The raster interrupt is already enabled.** A booted KERNAL takes one every
   frame to scan the keyboard, so a large part of the corpus sets only the
   compare line in `$FF0B`, does `CLI`, and spins — never touching `$FF0A` at
   all. Starting with the mask clear left all of those silent.
2. **There is a second raster vector at `$0312`.** The KERNAL's handler
   alternates its compare line between `$A1` and `$CC` and, when bit 6 of the
   compare is set, dispatches through `$0312` rather than finishing. That is the
   264's user raster hook, and part of the corpus installs its player there and
   nowhere else (`STX $0312 / STY $0313 / CLI`).
3. **The KERNAL keeps `SOUND`'s duration timers ticking.** A pair of 16-bit
   counters at `$04FC`/`$04FE` is counted up every interrupt and switches a voice
   off on overflow — that is what BASIC 3.5's `SOUND` duration argument rides on.
   Tunes that never go near BASIC still use it, because a counter the KERNAL
   ticks for free is a ready-made frame timer: seed it, then wait for it to come
   back to zero.
4. **A rip harness hands over through BASIC's `RUN`.** It sets the end-of-program
   pointer, relinks, and does `JMP $8BDC`; the tune it actually wants is named by
   a `SYS` in the program `RUN` would then execute. Trapped and resolved to that
   `SYS`. It has to be allowed to fire more than once for the same address — the
   second pass differs because the stack is deeper, which those tunes read with
   `TSX` and use.
5. **The KERNAL builds RAM-resident helpers at `$0494`.** Six short routines that
   read a byte from the RAM underneath the ROM through a fixed zero-page pointer,
   plus one whose pointer the caller patches in.

Result on the 148-file sample the fixes were derived from: **143 at cosine
≥ 0.90, median 0.991**, and nothing between 0.75 and 0.90 — tunes either play or
they do not. Levels track the engine within a few percent.

Of the five that do not reach 0.90: three render silent under the engine too, so
they are not this machine's failures. `sire_fukwitteddy` is a genuine BASIC 3.5
program — `CHAR` and `SOUND` statements, no `SYS` at all — and is now **declined**
by `load()` rather than run as if the program text were 6502, which is what made
it emit noise before. That leaves **one hard failure**,
`gulyas_laszlo_sos_dolgozat`, which diverges on an `RTS` whose return address
comes from a stack BASIC populated; reproducing that needs the interpreter.

### The full-corpus run

The 148-file sample above is the one the fixes were derived from, so it proves
nothing about generalisation. The whole corpus was then run: **every TED-tagged
row in the library, 1,225 files** (1,231 rows less six whose HVTC links are dead
upstream — those 404 for any player).

| | |
|---|---|
| declined by the plugin's content gate | 75 |
| silent under tedplay as well | 98 |
| **compared** | **1,052** |
| cosine ≥ 0.90 | **1,037** (98.6%) |
| between 0.75 and 0.90 | 0 |
| below 0.75 | 15 |

median 0.990, mean 0.973, 5th percentile 0.962.

**Of the 75 declined, 74 are silent under tedplay too** — they are not 264 files
at all but VIC-20 (`$1201`, 37 of them), PET (`$0401`, 15), VIC-TRACKER (`$3300`,
13) and C64 (`$0801`, 5) programs mis-tagged TED in the library. tedplay accepts
them on extension and renders silence; declining them is the honest outcome. The
one real loss is `sire_fukwitteddy`, a BASIC 3.5 program.

The run also found a gate bug worth recording: three tunes were being declined
for loading at **`$1000`** rather than `$1001`. That is the same 264 file saved
one byte lower, including the zero byte BASIC keeps below its program — the
program itself is still at `$1001`. Twenty-four files in the library are stored
that way. The gate now accepts both, `testmus/tedcr/load-at-1000.prg` guards it,
and the three play at 0.973 / 0.993 / 0.996.

Of the 15 below threshold, only about half are playback failures at all:

* **5 are tedplay droning, not playing** — `golf_royal`, `sos_dolgozat`,
  `totto!_intro`, `pogo_pete`, `terra_cognita`. Their peak envelopes barely move
  (coefficient of variation 0.004–0.09), which is the "tune fell over and left
  TED humming" pattern. This machine renders silence instead.
* **1 is a metric artifact** — `games_strangers` is a single 35 ms sound effect
  and nothing else. Both engines emit it (spectral cosine **0.998** once the
  bursts are aligned) but mine fires 60 ms earlier, and the envelope aligner
  cannot lock onto something that short.
* **2 start late** — `luca_2048_octaluxe` and `luca_the_it_crowd` play correctly
  from 10.2 s, at levels matching the engine to within a few percent, but the
  engine starts them at 0.7 s and 1.2 s. The main tune is right; an intro is
  being skipped or stalled.
* **7 are genuine silent failures**, and two of those are the same tune indexed
  twice, so **6 distinct**: `csabo_quadrillion`, `grumskiz_the_economic_universe`,
  `hrh_mozaik_hrh`, `kent_robert_torpedo_run`, `ko-ko_aliens_demo`,
  `tfss_kutato`.

**Six genuine failures in 1,225 files — 0.5%.** They share a signature that is
not yet root-caused: the tune runs and writes frequency registers at roughly one
write per frame but **never writes `$FF11`**, so the channels are never enabled.
On `csabo_quadrillion` the engine writes `$FF11 <- 31` and a volume ramp at the
point where this machine writes only `$FF12`, so the player is reached and a
branch inside it goes the other way.

### Two fixes that came out of listening, not measuring

Both were found because a tune sounded wrong, and neither showed up in the
8-second corpus score. They are the reason the timing table above reads the way
it does.

**Per-line CPU budget.** Described above: the frame total was right and the
per-line split was wrong. Found by tracing a tune that syncs to a raster line,
does its work, and waits for the same line again — in the engine the work
finished inside the line, here it spilled over and cost a whole frame. Verified
by instrumenting both machines to count CPU cycles per raster line, and by the
tune's two wait loops swapping back to the engine's balance (15,698 / 133,968
against 13,861 / 135,815; it had been inverted ten-fold).

**RAM comes up with a pattern, not zeroes.** `saboteur`'s last pattern pointer
aims *past* its own data, and the `$FF` sitting in unwritten memory is what
terminates the sequence so the song loops. Zero-filled RAM gave it an endless run
of `$00` to read as notes and it repeated its final note forever.

The pattern is **split at `$1000`**, which took two attempts. Applying the DRAM
pattern everywhere fixed `saboteur` and broke five tunes that read low memory and
OR the result into TED register values. Diffing the full 64K against the engine
at tune start showed why: below `$1000` a real machine's RAM is not raw at all —
the KERNAL and BASIC have written zero page, the stack and their workspace long
before a tune runs. This machine never boots, so it cannot reproduce that, but
zeroes are much closer to it than the DRAM pattern is. Above `$1000` the two
images already agreed.

Net effect on the 60-second drone count: 81 → 75. Small, and honestly reported:
most of the apparent movement in that metric is files sitting within noise of its
threshold.

## The plugin

`TEDCRPlugin` is named `TED`, builds in **both** variants with no gate, and is
registered in `plugin_register.cpp` **ahead of `tedplugin`** — both claim `.prg`
and ties break on registration order, so this one gets first refusal and tedplay
only ever sees what it declines. `cmtest priority_map` shows
`.prg : TED (P:0) -> Tedplay (P:0)`.

### The content gate

`.prg` is a bare Commodore executable: two bytes of load address and nothing else
saying which machine it is for. The gate requires the load address to be `$1001`,
where BASIC starts on the 264 series, **and** a `SYS` in the BASIC stub. That
rejects two things tedplay accepted:

* **C64 files**, which load at `$0801`. Worth being precise about, because
  `MusicDatabase.cpp` carries a comment saying the 126 C64-tagged `.prg` rows
  "play fine" under tedplay. They do not — three fetched from the library render
  **exact silence** through tedplay for six seconds. Once tedplay is gone those
  rows will be declined outright, which is the honest outcome, but it does mean
  126 rows stop being offered rather than being offered and silent.
* **264 BASIC programs**, whose music is BASIC 3.5 `SOUND` statements that only
  an interpreter can play. Running the program text as if it were 6502 (which is
  what happens without the gate) makes noise, which is worse than silence.

What the gate *cannot* separate is the VIC-20: an unexpanded VIC-20 also starts
BASIC at `$1001`. Those are still handled further up, by `MusicDatabase` matching
on the DB format string.

### Coverage

`testmus/tedcr/` has one fixture per thing the machine had to get right — each
was silent until the corresponding piece existed — plus two that must be
declined. The `TEST_CASE("TED declines what it cannot play")` guard was verified
by disabling the gate and watching it go red, per the project convention that a
guard never seen failing is not known to guard anything.

## Remaining work

Ordered by how much is known about each.

* **75 tunes settle into a stuck tone** where the engine keeps playing, over a
  60-second window. The 8-second corpus score above cannot see them. What is
  known, from diagnosing one (`spotlessmind1975_totto!_ending`) in full:
  - They are **not** the ROM-dependent tail. Only **6 of 64** stable cases
    execute real ROM code; the rest run entirely in RAM.
  - Execution is **identical for 710,493 instructions** (~2.5 s) before diverging,
    and the tune's player entry points then run the same number of times in both
    machines. What diverges is zero-page pointer state: a pointer helper runs 164
    times in the engine against 68 here.
  - It is **not** the KERNAL housekeeping our IRQ stub skips — every write to that
    pointer comes from the tune's own code, none from ROM.
  - So the code path is right and something *read* differs. The next step is to
    log TED register reads either side of the divergence and compare; a register
    sampled a moment early or late is the most likely candidate.
* **Six tunes render silent** with a distinct signature: the player runs and
  writes frequency registers about once per frame but never writes `$FF11`, so
  the channels are never enabled. `csabo_quadrillion`,
  `grumskiz_the_economic_universe`, `hrh_mozaik_hrh`, `kent_robert_torpedo_run`,
  `ko-ko_aliens_demo`, `tfss_kutato`. Two more (`luca_2048_octaluxe`,
  `luca_the_it_crowd`) play correctly but start nine seconds late, skipping an
  intro.
* **Two regressions from the RAM pattern** that were fine when RAM was zeroed:
  `drive_demo` and `luca_adventures_in_time_game_over`. Both read unwritten
  memory and now get `$FF` where they wanted `$00`.
* **`$FF07` is stored but never acted on.** Bit 6 is the documented NTSC/PAL
  select — it does not change the crystal, but it does change the TED's vertical
  counter wrap (312 lines against 262) and therefore the frame rate and the tempo
  of every raster-locked player. Bit 5 (TEDOFF) stops the TED's counters
  entirely. `TED_MASTER_NTSC` and `LINES_NTSC` exist in the header and are unused.
  Nobody has yet counted how much of the corpus sets either bit.
* **The 6502 read-modify-write double write is not modelled.** A real `INC`/`DEC`
  writes the old value before the new one. Harmless for RAM, but it would matter
  for a write-1-to-clear register such as `$FF09`.
* **Subtune selection** — the corpus selects between tunes by keypress; `$FF08`
  reports nothing held down and `pressKey()` is a stub without a keyboard matrix.
  The plugin reports one song per file.
* **No SID card.** tedplay enabled one unconditionally, so its unattributed
  `Sid.cpp` was mixed into every render. Only 11 of 148 sampled files touch
  `$FD40`–`$FD5F`, and 6 of those do exactly 4,096 writes, which is a blind
  `$FDxx` sweep rather than a SID player. cSID (WTFPL) is already in the tree if
  Plus/4 SID-card tunes ever turn up.

### A note on measuring this

Three windows have now been too short, each time hiding real failures that
listening found immediately: 8 seconds for the corpus score, 2 seconds for the
ROM-dependence scan, and 2 seconds for a trace diff whose divergence was at 2.5.
The drone metric's own threshold (tail coefficient of variation below 0.02) is
also loose enough that a dozen files flip across runs, so it cannot resolve
changes of only a few files. Tighten it before using it to judge a fix.
