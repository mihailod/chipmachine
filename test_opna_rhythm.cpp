// Isolated translation unit for the OPNA hardware-rhythm regression test.
// fmgen's opna.h pulls in global int8/int16/int32/uint typedefs, so it is kept
// out of test.cpp (which includes the rest of the chipmachine headers) to avoid
// type clashes.
//
// Verifies that the OPNA rhythm sample ROM embedded via opna_rhythm_rom.cpp is
// decoded and audible: a fresh OPNA (no FM/SSG notes) keys on all six rhythm
// voices and must produce non-zero output. Before the embedded soundbank
// existed, rhythm[i].sample was null and RhythmMix produced pure silence.

#include "opna.h"
#include <cstdlib>
#include <vector>

bool opna_rhythm_plays_sound()
{
    FM::OPNA opna;
    if (!opna.Init(7987200, 44100, false, nullptr)) {
        return false;
    }
    opna.Reset();
    opna.SetReg(0x29, 0x9f);                  // enable channels
    opna.SetReg(0x11, 0x3f);                  // rhythm total level = loudest
    for (int r = 0x18; r <= 0x1d; ++r) {
        opna.SetReg(r, 0xdf);                 // each drum: L+R pan, loudest
    }
    opna.SetReg(0x10, 0x3f);                  // KEY ON all six rhythm voices

    std::vector<int32_t> buf(4096 * 2);
    long long energy = 0;
    for (int blk = 0; blk < 30; ++blk) {
        std::fill(buf.begin(), buf.end(), 0);
        opna.Mix(buf.data(), 4096);
        for (int32_t s : buf) {
            energy += std::llabs(static_cast<long long>(s));
        }
    }
    return energy > 0;
}
