#!/usr/bin/env bash
#
# Trace every .mdx under a directory, one .trace file per tune, so two engines
# can be compared wholesale with `mdxdiff.py --batch`.
#
#   ./trace_corpus.sh <mdxdir> <outdir> [frames]
#
# Trace names are the tune's path relative to <mdxdir> with '/' replaced by
# '%', so the same name is produced from either engine's run and PDX banks
# sitting beside their .mdx are still found.

set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if (( $# < 2 )); then
  echo "usage: trace_corpus.sh <mdxdir> <outdir> [frames]" >&2
  exit 2
fi

mdxdir="$1"
outdir="$2"
frames="${3:-3600}"

[[ -x "$here/mdxtrace" ]] || { echo "build mdxtrace first: ./build.sh" >&2; exit 2; }

mkdir -p "$outdir"

n=0
failed=0
while IFS= read -r -d '' f; do
  rel="${f#"$mdxdir"/}"
  name="${rel//\//%}"
  if "$here/mdxtrace" -q -f "$frames" "$f" -o "$outdir/$name.trace" 2>/dev/null; then
    n=$((n + 1))
  else
    echo "  FAILED: $rel" >&2
    rm -f "$outdir/$name.trace"
    failed=$((failed + 1))
  fi
done < <(find "$mdxdir" \( -iname '*.mdx' \) -print0)

echo "traced $n tune(s) into $outdir"
(( failed )) && echo "$failed tune(s) failed to open"
exit 0
