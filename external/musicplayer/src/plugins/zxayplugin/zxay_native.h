#pragma once

// The routes that do NOT run a Z80.
//
//   * .stc / .asc -- Sound Tracker and ASC Sound Master. No redistributable ZX
//     replay routine was published for either, so these are sequencers written
//     from the format documentation in players/source/ driving the same Ayumi
//     chip the Z80 players drive.
//   * .fxm / .amad -- the Fuxoft AY Language is an interpreted bytecode, not a
//     tracker; there is nothing to sequence, only to interpret.
//   * .vtx / .psg -- recorded AY register streams. There is no player involved
//     at all: the file IS the register writes.
//   * .vt2 -- Vortex Tracker II's TEXT module, read by the Arkos Tracker 3
//     importer this repo already vendors for STarKos.

#include "zxay_format.h"
#include "zxay_source.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace musix::zxay {

// stc / asc / fxm / amad / vtx / psg. Returns nullptr if the data turns out not
// to be the format after all.
std::unique_ptr<Source> createNativeSource(Format f,
                                           const std::vector<uint8_t>& data,
                                           int sampleRate);

// vtx / psg (zxay_dump.cpp).
std::unique_ptr<Source> createDumpSource(Format f,
                                         const std::vector<uint8_t>& data,
                                         int sampleRate);

// stc (zxay_stc.cpp), asc (zxay_asc.cpp), stp (zxay_stp.cpp) and sqt
// (zxay_sqt.cpp).
std::unique_ptr<Source> createStcSource(const std::vector<uint8_t>& data,
                                        int sampleRate);
std::unique_ptr<Source> createAscSource(const std::vector<uint8_t>& data,
                                        int sampleRate);
std::unique_ptr<Source> createStpSource(const std::vector<uint8_t>& data,
                                        int sampleRate);
std::unique_ptr<Source> createSqtSource(const std::vector<uint8_t>& data,
                                        int sampleRate);
// The ZXAY container (zxay_container.cpp), shared by .amad and .st11. See
// that file for why the pointer arithmetic is worth centralising.
struct ZxayEntry
{
    long songData = 0;  // start of this song's data block
    long songName = 0;  // NUL-terminated title
    long author = 0;    // NUL-terminated author
};
bool parseZxayContainer(const std::vector<uint8_t>& d, const char* typeTag,
                        ZxayEntry* out);
std::string zxayString(const std::vector<uint8_t>& d, long at);

// psm (zxay_psm.cpp), gtr (zxay_gtr.cpp) and st11 (zxay_st11.cpp) -- reclaimed
// from ZXTune. st11 has no player of its own: it compiles the editor's format
// into the compiled one and reuses createStcSource().
std::unique_ptr<Source> createSt11Source(const std::vector<uint8_t>& data,
                                         int sampleRate);
std::unique_ptr<Source> createPsmSource(const std::vector<uint8_t>& data,
                                        int sampleRate);
std::unique_ptr<Source> createGtrSource(const std::vector<uint8_t>& data,
                                        int sampleRate);
// fxm / amad (zxay_fxm.cpp).
std::unique_ptr<Source> createFxmSource(Format f,
                                        const std::vector<uint8_t>& data,
                                        int sampleRate);

} // namespace musix::zxay
