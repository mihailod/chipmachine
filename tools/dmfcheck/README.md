# dmfcheck — DefleMask parser validator and A/B harness

Development scaffolding for `dmfcrplugin`, the clean-room DefleMask player for
SEGA Genesis and Master System. Standalone on purpose, like `tools/mdxtrace`: none of this goes through
the project's CMake, so it can never end up linked into ChipMachine or
ChipMachinePlus.

```bash
./build.sh                  # build dmfcheck + dmfrender
DMF_CORPUS=~/dmf ./build.sh --check
```

`dmfab` is built separately (see below) because it needs the plus build's
`libdmfplugin.a`.

## The tools

| | |
|---|---|
| `dmfcheck <dir>` | run the parser in **strict** mode over a tree and report, per DMF version, how many files consume their buffer exactly |
| `dmfrender <in> <out.wav>` | render through the clean-room player |
| `dmfrender --info <in>` | dump a module: instruments, per-channel note counts, effect census |
| `dmfab <file>` / `dmfab --list <list>` | A/B one or many files against Furnace, printing cosine similarity |
| `dmfab --spec <file>` | mean band-energy profile of both sides, side by side |

## Why strict parsing is the regression test

DMF has no length fields, no section markers and no trailing sentinel. A
mis-sized version gate therefore does not fail loudly — it silently shifts every
subsequent field, and the file still "parses". The one property that catches it
is that a correct parse lands **exactly** on the end of the inflated buffer.

That is what `dmfcheck` measures, and it is how the parts of the format
DefleMask never documented were pinned down: every combination of the six
ambiguous version gates was scored on exact consumption across all 833 Genesis
files, and one assignment wins outright. It also caught two places where the
published spec disagrees with what DefleMask actually wrote. See
`dmfcrplugin/README.md`.

`dmfcheck` tolerates 0 or 1 leftover bytes, because most files carry one
undocumented trailing byte — also measured, not assumed.

## Building dmfab

`dmfab` links **both** the clean-room sources and the plus build's
Furnace-based `libdmfplugin.a`, so it is GPL-encumbered and must never ship —
exactly the standing of `tools/mdxtrace`, which links GPL `mdxmini` as a
reference oracle.

```bash
P=../../external/musicplayer/src/plugins/dmfcrplugin
Y=../../external/ymfm
c++ -O2 -std=c++17 -I$P -I$Y/src -I/opt/homebrew/include dmfab.cpp \
    $P/dmf_file.cpp $P/dmf_player.cpp $P/sn76489.cpp \
    $Y/src/ymfm_opn.cpp $Y/src/ymfm_adpcm.cpp $Y/src/ymfm_ssg.cpp $Y/src/ymfm_pcm.cpp \
    ../../../build/plugins/dmfplugin/libdmfplugin.a \
    -L/opt/homebrew/lib -lz -o dmfab
```

## Why audio, and not a register trace

For a sequencer driving a register-level chip the right oracle is a
register-write trace: equivalence becomes a `diff` rather than a judgement call,
which is the whole argument for `tools/mdxtrace`.

It is deliberately not done here. Getting that trace out of Furnace would mean
instrumenting its Genesis platform code — precisely the code the clean-room
argument requires nobody to have read. So Furnace is driven only through the
public `ChipPlugin`/`ChipPlayer` interface and the comparison is spectral.

The two sides use different YM2612 cores and different resamplers and will never
be sample-identical; cosine similarity over log band-energy spectra asks the
question that actually matters — do the same notes, instruments and effects land
at the same times — and is the same measure used for VICE→cSID and the VIC-I
replacement.

## Running the full corpus

`dmfab` is single-threaded; split the list and fan out.

```bash
split -l 75 genesis_playable.txt /tmp/abchunk_
for f in /tmp/abchunk_*; do
  ( ./dmfab --list "$f" 15 2>/dev/null | grep -E "^(OK|SKIP)" > "/tmp/abfull_$(basename $f).txt" ) &
done
wait
cat /tmp/abfull_*.txt > /tmp/ab_full.txt
```

Output columns are `OK <cosine> <rms-furnace> <rms-cleanroom> <path>`. The two
RMS figures are there because a player that is sequenced correctly but 20 dB
down would still score well on cosine alone.

## Regenerating the index allow-list

`data/misc/dmfcr_playable.txt` lists the DefleMask rows the mas build can play.
Rebuild it after a collection update by probing each file's system byte and DMF
version and keeping systems `0x02`/`0x03`, versions `0x11`–`0x18`, checksum
intact — which is what `dmfcheck --system 0x02,0x03` reports on.
