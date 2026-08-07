#!/usr/bin/env bash
#
# Build the dmfcheck / dmfrender development harnesses.
#
# Standalone on purpose, like tools/mdxtrace: these never go through the
# project's CMake, so they can never end up linked into ChipMachine or
# ChipMachinePlus. They exercise the clean-room dmfcrplugin sources directly.
#
#   ./build.sh          build both
#   ./build.sh --check  build, then validate the parser over a corpus
#                       (set DMF_CORPUS to the directory to scan)

set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
plug="$here/../../external/musicplayer/src/plugins/dmfcrplugin"
ymfm="$here/../../external/ymfm"

# zsh/bash both: flags in an array, never a bare string.
cflags=(
  -O2 -g
  -std=c++17
  -I "$plug"
  -I "$ymfm/src"
  -I /opt/homebrew/include
)
ldflags=(-L/opt/homebrew/lib -lz)

common=(
  "$plug/dmf_file.cpp"
)

player=(
  "$plug/dmf_player.cpp"
  "$plug/sn76489.cpp"
  "$ymfm/src/ymfm_opn.cpp"
  "$ymfm/src/ymfm_adpcm.cpp"
  "$ymfm/src/ymfm_ssg.cpp"
  "$ymfm/src/ymfm_pcm.cpp"
)

echo "building dmfcheck..."
c++ "${cflags[@]}" "$here/dmfcheck.cpp" "${common[@]}" "${ldflags[@]}" -o "$here/dmfcheck"

echo "building dmfrender..."
c++ "${cflags[@]}" "$here/dmfrender.cpp" "${common[@]}" "${player[@]}" \
    "${ldflags[@]}" -o "$here/dmfrender"

echo "ok: $here/dmfcheck  $here/dmfrender"

[[ "${1:-}" == "--check" ]] || exit 0

corpus="${DMF_CORPUS:-}"
if [[ -z "$corpus" || ! -d "$corpus" ]]; then
  echo "SKIP: set DMF_CORPUS to a directory of .dmf files to validate"
  exit 0
fi

echo
echo "validating parser over $corpus (Genesis only)"
"$here/dmfcheck" "$corpus" --system 0x02,0x42,0x12
