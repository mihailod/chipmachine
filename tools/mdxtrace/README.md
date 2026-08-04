# mdxtrace — MDX driver equivalence harness

Verification scaffolding for replacing `mdxplugin`'s GPL-2 `mdxmini` with a
non-GPL MDX engine in the **mas** build.

MDX is a sequencer driving a register-level chip, so a correct reimplementation
should emit a **bit-identical YM2151 register stream** — not merely a similar
spectrum. That makes equivalence a `diff`, not a judgement call, and it is the
main reason this replacement is worth attempting at all. This harness exists so
that decision is made on evidence.

## Why not literal NLG

`mdxmini` already ships `nlg.c`, a timestamped register-log writer, gated behind
`USE_NLG` (never defined by either build). It was the obvious oracle, and this
harness reuses its exact instrumentation points — but not its file format:

* **NLG cannot represent ADPCM.** It logs `CMD_PSG` / `CMD_OPM` only. About 39%
  of our 6,913 MDX rows reference a `.pdx` sample bank, so an NLG-only harness
  would structurally not see the hardest part of the port and would give false
  confidence.
* NLG's tempo is encoded through a Z80 CTC approximation (`tempo_us / 256`,
  emitted only on change) — lossy, and tied to BouKiCHi's player rather than to
  what the MDX driver actually did.

So the trace here is line-oriented text covering **both** the OPM writes and the
PCM8 events, diffable with ordinary tools.

## Licence boundary

`mdxtrace` links `mdxmini`, which is **GPL-2** (`mdxmini/COPYING`; `mdxmini.txt`
states outright that linking to it statically or dynamically imposes GPL2).

It is a **development oracle and is never distributed.** Running GPL code to
generate reference output is fine; only shipping it is not. That is why this
tool builds via its own `build.sh` rather than through the project's CMake — it
cannot be accidentally linked into ChipMachine or ChipMachinePlus.

## The shipping builds are unaffected

The hooks added to `mdxmini` sit inside `#ifdef MDX_TRACE`, which only
`build.sh` defines. `./verify_inert.sh` proves this two ways:

1. Preprocessing each instrumented file the way the real build does (no
   `-DMDX_TRACE`) leaves **zero** references to the hooks.
2. Reconstructing the pre-instrumentation source and compiling both at `-g0`
   yields **byte-identical objects**.

Check 2 is the load-bearing one: adding lines shifts DWARF line tables, so a
plain `.o` hash comparison shows a difference even when codegen is identical.
`-g0` removes that noise. Verified for `mdx2151.c`, `pcm8.c` and `mdxmini.c`,
including the one change made outside an `#ifdef` (braces around the `tie`
early-return in `pcm8_note_on`, separately confirmed codegen-neutral).

## Usage

```bash
./build.sh --check                      # build + validate the harness itself
./verify_inert.sh                       # prove plus/mas are untouched

./mdxtrace -f 2000 song.mdx -o ref.trace
./mdxdiff.py ref.trace new.trace        # exit 0 identical, 1 differs

./trace_corpus.sh /corpus out_ref 3600  # whole-corpus sweep
./mdxdiff.py --batch out_ref out_new
```

Useful flags:

| flag | effect |
|---|---|
| `mdxtrace -c N` | render block size; the trace must not depend on it |
| `mdxdiff --opm-only` | compare YM2151 writes only, ignoring PCM8 |
| `mdxdiff --ignore-redundant` | collapse repeated PCM8 state writes that don't change the value |

`--ignore-redundant` matters: `mdxmini` calls `pcm8_set_master_volume` on *every
frame*, so a raw trace is ~25% redundant `P mvol` lines. An engine that only
writes on change would diverge everywhere despite behaving identically. The raw
trace stays lossless; the comparison is what gets relaxed.

## Trace format

```
F <frame> <tempo>          sequencer tick boundary
O <adr> <val>              YM2151 register write (hex)
P on <ch> <hash> <bytes>   PCM8 voice start
P tie <ch>                 note-on absorbed as a tie (no restart)
P off <ch>                 voice stop
P freq <ch> <hz>           playback rate
P vol <ch> <val>           per-channel volume
P mvol <val>               master volume
P pan <val>                master pan
```

Samples are identified by an FNV-1a content hash (first 4 KB), never by
pointer — pointers move under ASLR and would defeat diffing.

Two ordering facts the format depends on:

* Driver initialisation writes registers **during `mdx_open`**, before any
  frame boundary. Those writes are part of the trace, so the header comment is
  emitted before `mdx_open` is called.
* The frame limit is a hard cutoff applied inside the hooks. Rendering happens
  in chunks, so the last chunk would otherwise carry the sequencer past the
  requested count by a chunk-dependent amount, making trace length depend on
  block size.

## What the self-tests cover

`./build.sh --check` validates the *harness*, not any engine — if these fail, a
trace diff means nothing:

1. **Determinism** — same input twice gives an identical trace.
2. **Chunk invariance** — `-c 441` and `-c 4096` agree (confirmed for both
   OPM-only and ADPCM tunes).
3. **Sensitivity** — a single flipped register bit is caught.
4. **ADPCM coverage** — warns when no fixture starts a PCM8 voice.

## Fixtures

`testmus/mdx/` originally held four tunes, **none of which start a PCM8 voice**
(`alp.pdx`/`pha.pdx` are orphans — the `.mdx` that reference them aren't
present), so a green run certified only the OPM path.

`ab2_fin.mdx` + `ab2.pdx` (After Burner, X68000) were added to close that: 108
`P on` events with stable, repeating sample hashes, and chunk invariance holding
across `-c 441` / `-c 4096`. `testmus/` is local test data — it is not bundled
into either `.app` and not distributed.

Extra fixtures can also be supplied without committing anything:

```bash
MDXTRACE_FIXTURES=~/mdx ./build.sh --check
```

Note that **referencing a `.pdx` is not the same as using ADPCM** — `dra20.mdx`
names `dra00.pdx` yet starts no PCM8 voice in 2,000 frames. Test 4 reports
actual voice starts, not header references.

## The engine needs no bundled data

Worth recording, because it shapes the replacement: mdxmini opens exactly two
files — the `.mdx` and its `.pdx` — and both arrive over the network like any
other song (`MDXPlugin::getSecondaryFiles` reads the bank name out of the
header). There is no ROM, BIOS, driver blob or data directory, so a clean-room
MDX engine is **pure code** with nothing to ship alongside it.

That is not true of everything here: VICE needs C64 ROMs at `dataDir/c64`,
AdPlug has `adplug.db`, and PSF's `hebios.bin` turned out to be real Sony
firmware that had to be deleted outright. MDX has no equivalent problem.

## A measurement note

39% is the share of tunes that *reference* a `.pdx` bank, sampled over 120
random Vampi files. Referencing a bank is not the same as using ADPCM:
`dra20.mdx` names `dra00.pdx` yet starts no PCM8 voice in 2,000 frames. Actual
ADPCM usage is therefore somewhere at or below 39%, and this harness is the
tool that can finally measure it exactly — run `trace_corpus.sh` over the corpus
and count traces containing `P on`.
