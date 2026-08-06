# Modifications to fmgen

cisc's fmgen release 008 (2003-09-02) permits modification and redistribution
on four conditions, set out in `【著作権、免責規定】` of the accompanying
`readme.txt`. Two of them apply to distributing **source**, which this project
does:

> 3. 改変したソースコードを配布する際は改変内容を明示すること.
> 4. ソースコードを配布する際にはこのテキストを一切改変せずにそのまま添付すること．

*(3. when distributing modified source, state what was modified; 4. when
distributing source, attach this text without any alteration.)*

This file satisfies condition 3. `readme.txt` beside it satisfies condition 4:
it is cisc's original file, byte for byte, Shift-JIS with CRLF line endings,
MD5 `70c8b4544936bcd5957de29ef6c73e6c`, taken from
<http://retropc.net/cisc/sound/> (`dl/fmgen008.lzh`).

> **Do not replace it with the copy that circulates with px68k and its forks.**
> That one has been transcoded to EUC-JP, its line endings converted to LF, and
> its CVS keyword edited from `$Id:` to `$fmgen-Id:`. It is not the unaltered
> text condition 4 asks for.

See also the fmgen entry in the top-level `LEGAL`, which records that
compliance with condition 2 ("distribute as freeware") depends on ChipMachine
remaining free of charge.

## Baseline

Pristine fmgen release 008. Diffs below are against that release, comparing
text after normalising character encoding and line endings.

## Unchanged files

`diag.h`, `fmgen.cpp`, `fmtimer.cpp`, `fmtimer.h`, `misc.h`, `types.h`.

## Added files

| File | Why |
|---|---|
| `opna_rhythm_rom.cpp` | Built-in OPNA rhythm voices as 16-bit PCM. |
| `gen_rhythm_synth.c` | Offline generator for the above. Not part of the build. |
| `readme.txt` | cisc's original, restored (the vendored tree had dropped it). |
| `MODIFICATIONS.md` | This file. |

## Removed files

Nothing from the release itself.

Two files that this project had previously added were removed again: `need`,
an 8192-byte dump of the YM2608 ADPCM-A rhythm ROM (CRC32 `23c9e0d8`), and
`gen_rhythm_rom.c`, which existed only to decode it. That ROM is copyrighted
Yamaha firmware and is not distributable here. See "Rhythm samples" below.

## Changed files

Most of these predate this project and arrived with the KbMedia `m_s98`
lineage that fmgen was vendored through; they are listed because condition 3
asks what differs from cisc's release, not who changed it.

| File | +/- | What |
|---|---|---|
| `opna.cpp` | +284/-24 | `BUILD_OPN2` added; `SetPan`; rhythm-sample loader change (below) |
| `opna.h` | +110/-101 | Matching declarations; `LoadEmbeddedRhythm()` added |
| `file.cpp` | +61/-47 | Win32 file I/O ported to POSIX (`errno`, `snprintf`) |
| `psg.cpp` | +42/-23 | Stereo panning: `SetPan()` and per-channel mixing |
| `fmgeninl.h` | +14/-0 | Envelope-phase handling |
| `file.h` | +8/-3 | `<limits.h>`, `_WIN32` guards, signature changes for the above |
| `headers.h` | +4/-1 | `<tchar.h>` dropped; includes `types.h` |
| `fmgen.h` | +2/-2 | Include and friend-declaration adjustments |
| `psg.h` | +2/-0 | Declarations for the panning support |
| `opm.h` | +2/-3 | Comment and whitespace |
| `opm.cpp` | +1/-1 | Volume calculation expressed in floating point |

### Encoding

Most sources were transcoded from the release's Shift-JIS to UTF-8. `opna.h`
was transcoded **incorrectly** — read as Latin-1 and re-encoded — so its
Japanese comments are mojibake. Cosmetic, but it is a difference from the
original and is recorded as one.

### Rhythm samples

fmgen's OPNA plays its six hardware-rhythm voices (BD, SD, TOP, HH, TOM, RIM)
from PCM held in `rhythm[]`. Upstream fills that from an external
`2608_BD.WAV` .. `2608_RIM.WAV` set via `OPNA::LoadRhythmSample()`.

This tree adds `OPNA::LoadEmbeddedRhythm()`, called from `OPNA::Init()` only
when `LoadRhythmSample()` finds no such set, so drums work with no runtime
file. It fills `rhythm[]` from the tables in `opna_rhythm_rom.cpp`.

Those tables **are not Yamaha's**. They were previously a decode of the YM2608
ADPCM-A rhythm ROM; they are now synthesized from scratch by
`gen_rhythm_synth.c`, which matches the real voices in count, order, sample
length, peak and RMS — so arrangement and mix balance are preserved — but not
in timbre. The generator carries the full rationale and the reference
measurements.

The external-WAV path is untouched and still takes priority, so a user who
owns the real ROM gets the authentic drums.
