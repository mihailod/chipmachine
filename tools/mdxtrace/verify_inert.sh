#!/usr/bin/env bash
#
# Prove the MDX_TRACE instrumentation is inert for the shipping builds.
#
# The hooks added to mdxmini sit inside #ifdef MDX_TRACE, which only
# tools/mdxtrace/build.sh defines. This script checks that claim two ways:
#
#   1. Preprocess each instrumented file the way the real build does (no
#      -DMDX_TRACE) and confirm the compiler never sees the word "mdxtrace".
#   2. Reconstruct the pre-instrumentation source by deleting the #ifdef
#      MDX_TRACE blocks, compile both it and the current source with -g0
#      (no debug info, so line numbers cannot leak in), and compare the
#      resulting object bytes.
#
# Check 2 is the load-bearing one: adding lines to a file shifts DWARF line
# tables, so a plain .o hash comparison shows a difference even when codegen
# is identical. -g0 removes that noise.

set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Canonicalise: a path spelled "tools/mdxtrace/../../external/..." would put the
# string "mdxtrace" into every #line marker and defeat check 1.
src="$(cd "$here/../../external/musicplayer/src/plugins/mdxplugin/mdxmini/src" && pwd)"

files=(mdx2151.c pcm8.c mdxmini.c)
fail=0

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

echo "1. the compiler never sees the hooks (no -DMDX_TRACE)"
for f in "${files[@]}"; do
  # Ignore #line markers; only real code matters.
  n=$(cc -E -I "$src" -I "$here" "$src/$f" 2>/dev/null \
        | grep -v '^#' | grep -ci "mdxtrace" || true)
  if [[ "$n" == "0" ]]; then
    echo "   PASS $f -- 0 references after preprocessing"
  else
    echo "   FAIL $f -- $n references survive preprocessing"
    fail=1
  fi
done

echo
echo "2. codegen is unchanged vs. the pre-instrumentation source"

# Strip every "#ifdef MDX_TRACE ... #endif" block to reconstruct the original.
strip_hooks() {
  python3 - "$1" "$2" <<'PY'
import sys
src, dst = sys.argv[1], sys.argv[2]
out, depth = [], 0
for line in open(src):
    s = line.strip()
    if s == "#ifdef MDX_TRACE":
        depth += 1
        continue
    if depth:
        if s.startswith("#if"):
            depth += 1
        elif s == "#endif":
            depth -= 1
        continue
    out.append(line)
open(dst, "w").write("".join(out))
PY
}

mkdir -p "$tmp/orig"
cp "$src"/*.c "$src"/*.h "$tmp/orig/"
for f in "${files[@]}"; do
  strip_hooks "$src/$f" "$tmp/orig/$f"
done

cflags=(-O2 -g0 -std=gnu99 -c
        -Wno-deprecated-non-prototype
        -Wno-implicit-function-declaration)

for f in "${files[@]}"; do
  cc "${cflags[@]}" -I "$src"      "$src/$f"      -o "$tmp/cur_$f.o"  2>/dev/null
  cc "${cflags[@]}" -I "$tmp/orig" "$tmp/orig/$f" -o "$tmp/orig_$f.o" 2>/dev/null
  if cmp -s "$tmp/cur_$f.o" "$tmp/orig_$f.o"; then
    echo "   PASS $f -- object bytes identical"
  else
    echo "   FAIL $f -- codegen differs"
    fail=1
  fi
done

echo
if (( fail )); then
  echo "INSTRUMENTATION IS NOT INERT -- do not ship"
  exit 1
fi
echo "instrumentation is inert: plus and mas are unaffected"
