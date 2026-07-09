#pragma once

// Builds an assembleable Z80 program for a Music Box (.bbsong TMB) song: the
// vendored Music Box player plus its music_data. C++ port of the data half of
// musicbox.1te's Compile(), reading Beepola's channel-major pattern data.

#include "bbsong_parser.h"

#include <string>

namespace musix::bbsong {

std::string buildMusicBoxAsm(const Song& song);

} // namespace musix::bbsong
