#!/bin/sh
# Regenerate ../vtplayer_bin.h from the vendored Kahlin player sources.
# Requires dasm (https://dasm-assembler.github.io/), e.g. `brew install dasm`.
set -e
cd "$(dirname "$0")"
dasm harness.asm -oplayer.bin -splayer.sym -f3
python3 gen_bin_header.py player.bin player.sym ../vtplayer_bin.h
rm -f player.bin player.sym
echo "wrote ../vtplayer_bin.h"
