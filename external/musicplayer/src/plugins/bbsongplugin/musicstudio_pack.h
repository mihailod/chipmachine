#pragma once

// Builds an assembleable Z80 program for a Music Studio (.bbsong MSD) song: the
// vendored Music Studio player plus its music_data. C++ port of the data half
// of musicstudio.1te's Compile(), reading Beepola's channel-major data.

#include "bbsong_parser.h"

#include <string>

namespace musix::bbsong {

std::string buildMusicStudioAsm(const Song& song);

} // namespace musix::bbsong
