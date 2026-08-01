# musplugin — Compute!'s Sidplayer (`.mus` / `.str`)

A **clean-room** sequencer for Compute!'s Sidplayer, driving cSID's SID chip
emulation. It exists so the Mac App Store build can play these ~6,478 songs,
which otherwise belong to the GPL VICE bridge and are therefore absent there.

## Provenance (why this is licensable permissively)

Written from two independent, freely-distributed format write-ups shipped inside
the Compute's Gazette SID Collection (`CGSC/00_Documents/`):

* **`MUS_format_A.txt`** — Peter Weighill. States outright that it was produced
  by hex-editing files made with a MUSic editor, *"without any reference to the
  book written by Craig Chamberlin."*
* **`MUS_format_B.txt`** — Dick Thornton. Supplies the bit-level layouts.

**No code was consulted** from libsidplayfp, sidplayfp or JSIDPlay2 (all GPL),
nor from any disassembly of the original 1980s Sidplayer routine. Where the
documents are silent, behaviour was derived by **black-box observation** of the
register stream VICE emits — output only, never its player source.

## Format summary (from the documents)

* Header: 2-byte PRG load address, then three 2-byte little-endian voice lengths,
  then the three voice streams back to back, then up to five PETSCII text lines.
  Try offset **2 first** and fall back to 0: a load address can itself look like a
  plausible length (`Muppet Show B` loads at `$0046`, and 70/664/528 fits the
  file), and trying 0 first mis-parses those into a tune that starts 97 ticks
  early.
* Every pair is two bytes. `byte1 & 3` partitions the stream: `00` note,
  `01` the big "first byte = $01" table, `11` portamento, `10` everything else.
* Note: `byte1` bit 6 tie, bits 4-2 length, bits 7+5 dot type. `byte2` bits 7-6
  accidental, bits 5-3 octave (111 = octave 0), bits 2-0 note (000 = rest).
* For `10` commands the LOW NIBBLE selects the family and the HIGH nibble is
  often part of the *value*, so these must be matched on the **full byte**.
  Masking with `0x3F` conflates `C6` (pulse-vibrato depth) and `86` (vibrato
  rate) with `06` (tempo), which silently rewrites the tempo mid-tune.
* ATK/SUS is the one entry with a **three-bit** tag (`?nnn n100`, value in bits
  6-3), so dispatching it on the low nibble drops every odd value.

## Behaviour derived by measurement

The documents give values but not semantics. Everything below was measured
against VICE's own SID register writes, on a 1 ms grid, and is the reason the
plugin sounds right. Several entries **overturn** an earlier reading that was
confidently wrong — those are flagged, because the wrong version looked right
for a long time.

| Behaviour | Finding |
|---|---|
| Tick rate | A CIA timer, not a video frame: `period = 16467 + JIF*64 + 580` cycles. A tune with `JIF=-72` really runs at 83 Hz, not 60. |
| Note table | **NTSC-derived and TRUNCATED**: `floor(Hz * 2^24 / 1022727)`. Floor reproduces 44/46 observed entries; rounding only 16/46. On PAL the tunes genuinely play ~0.65 semitones flat. |
| Note length | **Exactly** the raw formula. (Overturns "1.049x plus a tick" — that was a per-*tick* IRQ cost mis-modelled as a per-*note* one.) |
| Default tempo | **144** (= 100 QPM), not 128. Only visible on tunes that play a timed event before their first `TEM`. |
| Startup | The player idles **2 ticks** before sequencing. Only 36 ms, but vibrato moves every tick, so being 2 ticks early wrecks its phase. |
| Length code 1 | Not a note value: the length came from the preceding **`UTL`**, in ticks. This is how MIDI-converted tunes encode all rhythm; without it they run ~8x too fast. |
| Length code 0 | Splits on **bit 5**. `$20` (bit set) is the 64th. `$00` (bit clear) is a **zero-length** event — it sets pitch and retriggers but costs no time, so the next pair runs on the same tick. 28,859 occurrences across 1,508 tunes. |
| `HED` | A **total** play count, not a count of extra repeats: `HED 2 … TAL` plays the block twice. |
| Vibrato | Triangle: one `VDP` step per tick, reversing at `+/-VRT`, so amplitude `VRT*VDP` and period `4*VRT` ticks. Starts at **0 ascending** and **free-runs** — it is *not* re-phased at note-on. (Overturns "starts at +VRT descending".) |
| Modulation schedule | Vibrato and `P-S` advance for a note's whole duration **including its one-tick gate-release gap**, and **freeze through a rest**. The note-on tick itself **holds** the counters but still applies their current offset. |
| `P-S` | **Is** a signed per-tick delta on the pulse width, active only while a note is running. (Overturns "does not sweep" — that came from tunes whose `P-S` voices were resting, e.g. `Raistlin` voices 1 and 2 carry identical commands and only voice 1 sweeps.) |
| `F-S` | A cutoff sweep expressed as a *rate*: one step per `\|F-S\|` ticks. |
| `AUT` | The filter **tracks the note**: `cutoff = round(semitone * 5/3) + AUT`, recomputed at every note-on, then swept by `F-S`. A tune can drive the filter with no `F-C` at all. |
| Filter registers | Cutoff, resonance and mode are chip-wide and written **unconditionally** — they are *not* gated on whether a voice is routed. `La Donna e Mobile` routes nothing yet still drives cutoff 80/100/72/98, resonance 15 and low-pass. |
| `PNT` | Does **not** shorten the gate; the gate is held for the whole note. |
| Portamento | **Is** a glide: a per-tick delta on the *frequency register* (`byte1` bits 7-2 `<< 8 \| byte2`), sliding from the currently sounding pitch to the new note's and clamping on arrival. A zero-length `$00` event before the note sets the glide's starting pitch. (Overturns "produces no glide at all — do not fix it", which was concluded from tunes where the glide had already converged.) |

## Validation

Scored as **register match on a 1 ms grid** against a VICE oracle, 8 s per tune.
Spectral similarity was tried first and abandoned: it repeatedly rewarded
behaviour later proven wrong.

* **Full CGSC corpus (16,601 files):** 100% parsed and rendered, zero crashes,
  zero hangs, zero parse failures.
* **Random 25% sample:** see the memory note for the current tier numbers; the
  10% sample sits at **median 98.5%, mean 96.4%, 77.5% of tunes >= 95%**.
* Benchmark tunes: Kate_and_Allie 99.9%, Swan_Lake 99.8%, La_Donna 99.8%,
  Cheers 99.3%, Mozart 99.2%, Africa (Dan Barrett) 98.4%, Linus and Lucy 98.1%,
  Raistlin 97.7%.

## Known gaps

* **Dropped frames in the reference.** ~8% of tunes contain one or more ticks
  where the original 6502 player's IRQ overran and produced no register write at
  all. Everything after such a tick is permanently one tick out of phase against
  our (evenly clocked) output. Measured over 1,660 tunes: 0 drops -> median
  98.5%, 1 drop -> median 89.1%. This is an artifact of the reference, not a bug
  here, and reproducing it would need cycle-exact emulation of the very routine
  we may not read. Treat those tunes as at their ceiling.
* A residual population sits in the 90-95% band on fine-detail phase (pulse-width
  and frequency low bytes), which is below audibility.
* A handful of tunes (3 of 16,601 measured) still render silent.
