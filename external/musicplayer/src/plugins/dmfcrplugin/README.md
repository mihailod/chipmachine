# dmfcrplugin — clean-room DefleMask `.dmf` player

Plays DefleMask modules that target the **SEGA Genesis / Mega Drive** (YM2612 +
SN76489) and the **SEGA Master System** (SN76489 alone). Written from
DefleMask's own published documentation so that the Mac App Store build can play
`.dmf` at all.

## Why it exists

`.dmf` is DefleMask's own format. DefleMask is closed source, and the only other
engine that reads `.dmf` is **Furnace** (tildearrow), which is GPL-2.0-or-later
*structurally* — 76 of 77 engine files, all 80 `DivPlatform*` wrappers, and a
dozen sound cores carry the GPL header. There is no file to patch out and no
second implementation to swap in, so `dmfplugin` (which wraps a headless slice of
Furnace) is gated out of the `mas` variant entirely by `CM_HAVE_DMF`.

That left 2,071 DefleMask rows hidden from the mas index. This plugin is the
first step at getting them back, one chip target at a time.

## The clean-room boundary

This is the part the licence argument depends on, so it is stated precisely.

**Sources used.** Everything in `spec/`, archived verbatim as fetched:

| File | What it gave us |
|---|---|
| `DMF_SPECS_0x11/12/13/15/16/18.txt` | the binary layout, per format version (`https://www.deflemask.com/`) |
| `EFFECTS_standard_legacy_manual.txt` | the `0xy`–`Fxx` effect table and the `Exx` extended commands |
| `EFFECTS_genesis_legacy_manual.txt` | the YM2612 effects `10xy`–`1Dxx` and the SN76489 `20xy` |
| `deflemask_manual_legacy.pdf`, `deflemask_manual.pdf` | the manuals those two extracts come from |

Public hardware documentation was used for the two chips: the YM2612 register
map and F-number/block pitch encoding, and the SN76489 counter/LFSR/attenuation
description.

**Sources deliberately NOT used.** Furnace's `src/engine/fileOps/dmf.cpp`, its
`DivPlatformGenesis*` code, or any other Furnace source. None of it was read at
any point while writing the parser, the sequencer or the chip glue.

That also decided how this player is *verified*. The natural oracle for a
sequencer driving a register-level chip is a register-write trace — that is what
`tools/mdxtrace` does for MDX, and it turns equivalence into a `diff` rather
than a judgement call. Here it would have meant instrumenting Furnace's Genesis
platform code, i.e. reading exactly the code that must stay unread. So Furnace
is driven strictly as a **black box** through the public `ChipPlugin` interface
and the two sides are compared as **audio**. See `tools/dmfcheck/`.

## Chip cores

| Chip | Core | Licence |
|---|---|---|
| YM2612 | Aaron Giles' **ymfm**, `external/ymfm` | BSD-3 |
| SN76489 | `sn76489.cpp`, written here | — |

ymfm is the same vendored copy `libvgmplugin` already drives for its OPN
adapters. It is **not** Nuked-OPN2: that is LGPL, and static linking into an App
Store binary cannot satisfy the relink requirement.

The SN76489 was written rather than vendored. libvgm's `sn76496.c` is BSD-3 and
would have been fine on licence, but it is welded to libvgm's device framework
(`EmuStructs` / `DEV_DEF` / `SoundDevs`), which is a lot of scaffolding for four
counters and a shift register. Same call as `victrackerplugin`'s VIC-I core.

## What it covers

**SYSTEM_GENESIS (`0x02`)** and **SYSTEM_SMS (`0x03`)**, at DMF versions
`0x11`–`0x18`. 696 songs: 598 Genesis and 98 SMS.

The SMS was nearly free. It is the same SN76489 the Genesis PSG already used,
with no FM side at all, so it needed only a channel-layout change — the PSG
channels start at 0 instead of 6, and no YM2612 is created.

Declined, and why:

* **EXT.CH3** (`0x42`, and `0x12` as the older specs label it) — 57 files. It
  splits FM channel 3 into four independently-pitched operators, which is a
  different channel model rather than an extra effect.
* **DMF versions `0x0D`, `0x10` and `0x19`–`0x1B`** — 11 files. DefleMask never
  published a spec for these and they desynchronise; declining beats mis-parsing.
* **Corrupt files** — 171 fail their zlib checksum and really are damaged
  (see below).

`canHandle()` content-gates on all of that, so in the **plus** build anything
this plugin cannot play falls through to Furnace instead of being claimed.

## Routing

Ships in **both** variants. `plugin_register.cpp` registers `dmfplugin`
(Furnace) **first**, and both declare priority 1 — so `stable_sort` leaves them
in registration order and Furnace wins every `.dmf` in the plus build. Plus
playback is therefore **unchanged**, and stays available as the A/B reference.
In mas, `dmfplugin` is compiled out and this plugin is the only claimant. Same
arrangement as `musplugin` behind `vicepluginbridge`.

## Pattern blocks are stored per ORDER POSITION

DMF writes one pattern block per channel per order row. When the same pattern
number appears at several positions the *content is duplicated* into each, so
the block to play is selected by the order index, not by the matrix value.

Verified structurally rather than inferred: over 200 files, of 21,290 duplicate
matrix entries **all 21,290** had byte-identical content at those positions and
**none** differed. Both readings therefore select the same notes — except that a
matrix value larger than the block count made the by-value form fall back to
pattern 0 and play the wrong bar. Indexing by position cannot go out of range.

## Three places the corpus contradicts the spec

All three were established by measurement, not guesswork. DMF has no length
fields, no section markers and no trailing sentinel, so a mis-sized version gate
does not fail loudly — it silently shifts every later field. The one property
that catches it is that a correct parse consumes the inflated buffer **exactly**.
`tools/dmfcheck` runs that check over the whole corpus, and every combination of
the six ambiguous gates was scored on it.

1. **Arpeggio Tick Speed disappears at `0x14`, not `0x15`.** The specs say it
   was "REMOVED since ver 11.1" and `DMF_SPECS_0x15` *is* the 11.1 document, so
   the byte should survive through `0x14`. All 37 v`0x14` Genesis files parse
   exactly without it and none with it.

2. **Macro `LOOP_POSITION` is always conditional.** `DMF_SPECS_0x11` lists it
   unconditionally, with the `IF ENVELOPE_SIZE > 0` guard appearing only from
   `0x12`. In the corpus the guard is already in force at `0x11`: all 94 v`0x11`
   Genesis files parse exactly with the conditional read, none with the
   unconditional one.

3. **There is an undocumented trailing byte.** 606 of the 643 well-formed
   Genesis files carry one spare byte after the PCM sample block; the other 37
   end exactly. Its value varies, so it is not a sentinel, and nothing
   references it.

The two versions with no published spec at all (`0x14`, `0x17`) were resolved
the same way.

## The bad-checksum files are genuinely corrupt

Worth recording, because the opposite conclusion is very tempting and an earlier
draft of this plugin acted on it.

199 of the 833 Genesis files fail their zlib Adler-32 while still producing a
*complete* deflate stream (zlib reports end-of-stream) whose first bytes are a
perfectly well-formed DMF header — correct magic, plausible version, correct
system byte, intact song name. It looks exactly like a writer bug worth
inflating past.

It is not. Parsed through to the end, **182 of those 199 desynchronise**, almost
always inside the PCM sample block, where one sample's data runs an odd number
of bytes and the next sample's name arrives with its first character missing.
The corruption is real and merely happens to fall after the header. Of the 190
total parse failures across the Genesis corpus only 8 are checksum-clean, and
all 8 are the unsupported DMF versions above.

So the strict check is right, and `dmfplugin`'s "corrupt DefleMask file" verdict
is right. This plugin rejects them identically.

## Verification

Automated A/B against Furnace, 15 s each, cosine similarity over log
band-energy spectra — the method already used in this repo for VICE→cSID and for
the VIC-I replacement:

```
Genesis (598 files)
  mean 0.934   median 0.953   min 0.318
  >= 0.90: 513 (85.8%)     >= 0.80: 574 (96.0%)     < 0.70: 9 (1.5%)

SMS (98 files)
  mean 0.867   median 0.894   min 0.602
  >= 0.90: 44 (44.9%)      >= 0.80: 80 (81.6%)      < 0.70: 5 (5.1%)
```

Spectral rather than sample-exact on purpose: the two sides use different YM2612
cores and different resamplers, so they are never bit-identical even when the
sequencing is exactly right. What this measures is whether the same notes,
instruments and effects land at the same times.

Rebuild the harnesses and re-run with `tools/dmfcheck/build.sh`.

### Operators are stored in register order, not the interleaved order

The YM2612's four operator slots sit at `+0`/`+4`/`+8`/`+12` and are
conventionally labelled "operator 1, 3, 2, 4" — the well-known interleave — so
the obvious mapping for a file that numbers its operators 1..4 is
`{0, 8, 4, 12}`. That is wrong for DMF: DefleMask writes its operator records in
**register order**, so its "operator 2" is simply the slot at `+4`.

The interleaved reading is plausible enough that it survived a long time, and
it does not fail loudly — it produces plausible-sounding but wrong timbres, and
occasionally silences an instrument outright by putting the loud operators on
the modulators. An ALG 4 patch reading TL `28/127/0/127` comes out with *both*
carriers at 127 under the interleave, i.e. silent, which is what led to it.

Measured over 120 files: interleaved `{0,8,4,12}` scores mean 0.903 / median
0.925; register order `{0,4,8,12}` scores 0.927 / 0.947. It also makes the
carrier table coherent — with register order, DefleMask's operator *N* is the
*N*th slot and the algorithm definitions read straight off the manual.

Fixing this moved the Genesis mean from 0.909 to 0.929 and the median from
0.933 to 0.948.

### Retriggering needs a clock between key-off and key-on

The single largest correctness bug found, and worth recording because the
symptom points nowhere near the cause.

Retriggering a note is key-off then key-on, and the YM2612's envelope generator
only restarts when it *observes* that off->on edge. Issuing both writes back to
back, with no chip clock between them, leaves the register reading "on" and the
envelope never restarts — the note already sounding just carries on decaying.
The first note on a channel is fine (a genuine off->on), every retrigger after
it silently does nothing.

What that sounds like is a track that starts correct and then fades to nothing
while the sequencer keeps running perfectly — note-on traces looked flawless,
the pattern data was right, and the output was ymfm's idle DC. Measured
directly: a retrigger with no clock between the two writes leaves the amplitude
at 1029, with one clock between it returns to the full 5440.

The key-off now goes out immediately and the key-on is deferred until after the
next chip clock. Fixing this moved the Genesis mean from 0.878 to 0.909, the
median from 0.912 to 0.933, and cut the files below 0.70 from 39 to 13.

### The DAC, and the effects the first pass skipped

Effect `17xx` (DAC enable) turns out to be the third most common effect in the
corpus — 367 of 598 files, 61% — and the first implementation got three things
wrong: it picked the sample by the `EBxx` bank register (so nearly always sample
0) instead of by the note, it read every sample as signed when the 8-bit ones
are stored **unsigned 0..255** in their 16-bit words (measured: the 8-bit range
is exactly 0..255, the 16-bit range the full -32768..32767), and it ignored the
per-sample "amp" percentage.

**The note does not pitch a DAC sample.** It always plays at its recorded rate,
which is what the hardware does -- the Mega Drive's DAC is fed a byte at a time
by the CPU at a fixed rate, so there is nothing to resample against, and
DefleMask does not do it in software either. Measured across the 363 DAC files,
and the trend is unambiguous: treating the note as a pitch against a C-5
reference scores mean 0.9325, C-4 0.9345, C-3 0.9369, not pitching at all
**0.9416**.

Getting this wrong is expensive and nearly invisible. A DefleMask sample is
often mostly silence with a single burst in the middle, so playing it at a
quarter speed means the pattern loops and retriggers before the burst is ever
reached and the drum track is simply *absent* rather than wrong. On the file
that led here it cost about 40 dB: the first 21% of the sample -- all this
player ever reached -- has RMS 75, against 3291 for the sample as a whole.

Sample selection is `bank * 12 + note-within-octave`, which is what the
manual's "a max of 12 sample banks ... from 0 to 11" implies against 12 notes to
an octave, and which measured best (0.9299 against 0.9293 for `note-1` and
0.9291 for always-first).

A census of the corpus also turned up effects that were being parsed and then
silently ignored: `E1`/`E2` note slide (93 and 39 files — 22% between them),
`Cxx` retrig (23 files), `E0` arpeggio tick speed, and `E4` fine vibrato depth.
All are implemented now. `EAxx` (Set Legato Mode), which appears only in the
newer manual and not the legacy one, is documented but occurs in **no** file in
the corpus, so it is deliberately not implemented.

### What was ruled out

Four plausible suspects were investigated against the corpus and cleared. They
are recorded because each looks like an obvious culprit and re-investigating
them is wasted effort:

* **Channel-volume mapping.** Carriers-only vs operator-4-only vs all four
  operators spans 0.9027-0.9035 over 120 files, and 0.500-0.522 over the worst
  ones.
* **Portamento domain.** The manual defines 1xx/2xx/3xx as changing the
  *frequency*, so they are implemented in F-number units rather than semitones —
  but over 153 portamento-heavy files the two readings are indistinguishable
  (0.9447 vs 0.9452). The documented reading is kept on principle, not evidence.
* **DAC mix level.** Flat from 0.25 to 1.0 (0.9404-0.9415) over the 363 DAC files.
* **PSG mix level.** Lower values nudge the cosine up (0.9342 -> 0.9357 at 0.15)
  but only by making everything quieter — the overall level match degrades from
  1.13 to 0.84. That is fitting the metric, not the hardware, so 0.25 stands.

Two corrections did land from the same pass, both on principle rather than
because the corpus moved: a DAC-enabled channel 6 no longer sounds its FM voice
underneath the sample, and a channel whose rows never carry an instrument column
now plays instrument 0 rather than an unwritten chip (every operator at TL 0,
i.e. full output).

### Known gaps

**When a one-shot stops.** Furnace ends some modules that this player keeps
looping — typically short jingles whose last row carries a `Bxx` pointing at the
order it is already in. The exact rule is not recoverable without reading
Furnace's source, so this player stops only when the order list runs off its end
with no jump taken, and otherwise repeats. Erring towards playing on is the
safer half: a jingle that repeats is a nuisance, one cut short loses music. The
A/B scores over the reference's active span so this disagreement is not counted
as a note error (see `tools/dmfcheck`).

**Eleven files remain below 0.70.** The channel-volume mapping was investigated
as a suspect and cleared: attenuating carriers-only versus operator-4-only versus
all four operators spans just 0.9027–0.9035 over 120 files, and only 0.500–0.522
across the worst files. It is not where the remaining error is.

**A caution about near-silent files and this metric.** Where a render is almost
silent the cosine sits right on the "one side is silent" threshold and becomes
unstable — `BotB 31762 sound system.dmf` read 0.90, then 0.02, while its
measured RMS only moved from 0.0012 to 0.0006 against Furnace's 0.0405. The
score swing was noise; the file was badly wrong throughout. Read the two RMS
columns alongside the cosine, which is why `dmfab` prints them.

Ordinary timbral agreement is good: the score does not decay with render length,
so tempo and sequencing are right in the common case.

## Files

| File | |
|---|---|
| `dmf_file.{h,cpp}` | parser; version gates and their evidence |
| `dmf_player.{h,cpp}` | sequencer, effect column, YM2612 + PSG register glue |
| `sn76489.{h,cpp}` | the PSG |
| `DMFCRPlugin.{h,cpp}` | `ChipPlugin` surface and the content gate |
| `spec/` | the archived DefleMask documentation this was written from |

Set `DMFCR_DEBUG=1` for a note-on trace, `DMFCR_REGS=1` for every chip register
write.
