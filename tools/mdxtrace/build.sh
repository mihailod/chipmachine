#!/usr/bin/env bash
#
# Build the mdxtrace reference dumper.
#
# Standalone on purpose: it does not go through the project's CMake, so it can
# never end up linked into ChipMachine or ChipMachinePlus. mdxmini is GPL-2 and
# this binary is a development oracle only.
#
# Output: tools/mdxtrace/mdxtrace
#
#   ./build.sh          build
#   ./build.sh --check  build, then run the self-consistency tests

set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
mdxmini="$here/../../external/musicplayer/src/plugins/mdxplugin/mdxmini"
testmus="$here/../../testmus/mdx"

# zsh/bash both: keep flags in an array, never a bare string.
cflags=(
  -O2 -g
  -std=gnu99
  -DMDX_TRACE
  -I "$here"
  -I "$mdxmini/src"
  -Wno-deprecated-non-prototype
  -Wno-implicit-function-declaration
)

srcs=(
  "$here/mdxtrace.c"
  "$mdxmini/src/mdxmini.c"
  "$mdxmini/src/mdx2151.c"
  "$mdxmini/src/mdxmml_ym2151.c"
  "$mdxmini/src/mdxfile.c"
  "$mdxmini/src/pdxfile.c"
  "$mdxmini/src/pcm8.c"
  "$mdxmini/src/ym2151.c"
)

echo "building mdxtrace..."
cc "${cflags[@]}" "${srcs[@]}" -lm -o "$here/mdxtrace"
echo "ok: $here/mdxtrace"

[[ "${1:-}" == "--check" ]] || exit 0

# ---------------------------------------------------------------------------
# Self-consistency tests. These validate the HARNESS, not any engine: if these
# fail, a trace diff means nothing.

fail=0
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

shopt -s nullglob
mdxfiles=("$testmus"/*.mdx "$testmus"/*.MDX)

# Extra fixtures beyond testmus/, without committing anything:
#
#   MDXTRACE_FIXTURES=~/mdx ./build.sh --check
#
if [[ -n "${MDXTRACE_FIXTURES:-}" ]]; then
  if [[ -d "$MDXTRACE_FIXTURES" ]]; then
    while IFS= read -r -d '' f; do mdxfiles+=("$f"); done \
      < <(find "$MDXTRACE_FIXTURES" -iname '*.mdx' -print0)
    echo "using extra fixtures from $MDXTRACE_FIXTURES"
  else
    echo "WARN: MDXTRACE_FIXTURES=$MDXTRACE_FIXTURES is not a directory" >&2
  fi
fi

if (( ${#mdxfiles[@]} == 0 )); then
  echo "SKIP: no test files in $testmus"
  exit 0
fi

echo
echo "1. determinism (same input twice -> identical trace)"
for f in "${mdxfiles[@]}"; do
  "$here/mdxtrace" -q -f 600 "$f" -o "$tmp/a.trace"
  "$here/mdxtrace" -q -f 600 "$f" -o "$tmp/b.trace"
  if cmp -s "$tmp/a.trace" "$tmp/b.trace"; then
    echo "   PASS $(basename "$f")"
  else
    echo "   FAIL $(basename "$f") -- trace is not reproducible"
    fail=1
  fi
done

echo
echo "2. chunk invariance (render block size must not shift the trace)"
for f in "${mdxfiles[@]}"; do
  "$here/mdxtrace" -q -f 600 -c 441  "$f" -o "$tmp/c1.trace"
  "$here/mdxtrace" -q -f 600 -c 4096 "$f" -o "$tmp/c2.trace"
  if cmp -s "$tmp/c1.trace" "$tmp/c2.trace"; then
    echo "   PASS $(basename "$f")"
  else
    echo "   FAIL $(basename "$f") -- trace depends on render block size"
    fail=1
  fi
done

echo
echo "3. sensitivity (a perturbed trace must be caught by mdxdiff)"
f="${mdxfiles[0]}"
"$here/mdxtrace" -q -f 600 "$f" -o "$tmp/ref.trace"
# flip the low bit of one OPM value in the middle of the trace
python3 - "$tmp/ref.trace" "$tmp/bad.trace" <<'PY'
import sys
src, dst = sys.argv[1], sys.argv[2]
lines = open(src).read().splitlines()
opm = [i for i, l in enumerate(lines) if l.startswith("O ")]
i = opm[len(opm) // 2]
adr, val = lines[i].split()[1:3]
lines[i] = f"O {adr} {int(val, 16) ^ 1:02x}"
open(dst, "w").write("\n".join(lines) + "\n")
PY
if python3 "$here/mdxdiff.py" "$tmp/ref.trace" "$tmp/bad.trace" >/dev/null 2>&1; then
  echo "   FAIL -- mdxdiff reported a perturbed trace as identical"
  fail=1
else
  echo "   PASS -- perturbation detected"
fi

echo
echo "4. ADPCM coverage (does the corpus exercise the PCM8 path at all?)"
covered=0
for f in "${mdxfiles[@]}"; do
  # NOT `| grep -q`: it exits on the first match, mdxtrace takes SIGPIPE, and
  # `set -o pipefail` then reports the whole pipeline as failed -- which made
  # this test claim "no ADPCM coverage" for a tune that plainly had it.
  # grep -c consumes all input, so there is no early close.
  n=$("$here/mdxtrace" -q -f 1500 "$f" 2>/dev/null | grep -c "^P on " || true)
  if (( n > 0 )); then
    echo "   PASS $(basename "$f") -- $n PCM8 voice starts"
    covered=1
  fi
done
if (( ! covered )); then
  echo "   WARN -- no test file starts a PCM8 voice, so the tunes with a .pdx"
  echo "        bank (up to ~39% of the real corpus) are UNVERIFIED here."
  echo "        Add an MDX+PDX pair to testmus/mdx/, or point"
  echo "        MDXTRACE_FIXTURES at a directory holding one."
fi

echo
if (( fail )); then
  echo "HARNESS CHECKS FAILED"
  exit 1
fi
echo "harness checks passed"
