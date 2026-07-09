#pragma once

// Builds a runnable image for a Beepola SFX (.bbsong "SFX") song: the vendored
// SFX player (sfx_player.h) plus a song data block compiled into the player's
// format. This reproduces Beepola's SFX compiler output (validated byte-exact
// for tone + sustain against real compiled binaries). Percussion is emitted as
// empty (rest) patterns for now -- songs play their melody; real drums are a
// later pass. The returned bytes load at SFX_ORG and run from there.

#include "bbsong_parser.h"

#include <cstdint>
#include <vector>

namespace musix::bbsong {

std::vector<uint8_t> buildSfxImage(const Song& song);

} // namespace musix::bbsong
