#pragma once

// Builds an assembleable Z80 program for a Phaser1 (.bbsong P1D/P1S) song:
// the vendored Phaser1 player plus a music_data block generated from the parsed
// song. This is a C++ port of the data-generation half of phaser1.1te's
// Compile(), reading Beepola's channel-major pattern data instead of 1tracker's
// song model. The result is fed to Z80Assembler.

#include "bbsong_parser.h"

#include <string>

namespace musix::bbsong {

// Returns Z80 assembly text (player + music data). `synthDrums` selects the
// Synth (P1S) vs Digital (P1D) drum section.
std::string buildPhaser1Asm(const Song& song, bool synthDrums);

} // namespace musix::bbsong
