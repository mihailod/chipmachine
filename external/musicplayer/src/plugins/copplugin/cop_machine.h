#pragma once

// Minimal SAM Coupé machine for "COP" / E-Tracker songs (the modland
// "Sam Coupe COP" corpus).
//
// A .cop file is a raw SAM Coupé memory image plus a Z80 replay routine: either
// the routine is compiled into the song itself, or the file is an "E-Tracker"
// data file whose player is the shared E-Tracker replayer (vendored here as
// etracker_bin). We run that original Z80 routine on the vendored GME Z80 core
// and route its sound-chip writes (OUT to the SAM's SAA1099 ports) into Dave
// Hooper's SAASound emulator.
//
// The load/patch/calling convention is a faithful re-implementation of
// Christopher O'Neill's SCPlayer (MIT), differing only in that we drive the
// commercial-clean GME Z80 core (instead of SCPlayer's non-commercial Fayzullin
// core) and Hooper's SAASound. The 32K image is mirrored across the whole 64K
// address space (the player's code lives at 0x8000+, which aliases the 0x0000+
// load), exactly as SCPlayer's `& 0x7fff` masking does.

#include "gme/Z80_Cpu.h"

#include <cstddef>
#include <cstdint>
#include <vector>

class CSAASound; // SAASound, opaque here

namespace musix::cop {

// SAM Coupé songs replay at 50Hz.
constexpr int PLAY_HZ = 50;

class CopMachine
{
public:
    explicit CopMachine(int sampleRate);
    ~CopMachine();
    CopMachine(const CopMachine&) = delete;
    CopMachine& operator=(const CopMachine&) = delete;

    // Loads the song image (applying SCPlayer's format detection / patches) and
    // runs the replayer's init routine. Returns false if the data is not a
    // recognisable COP image.
    bool init(const uint8_t* song, size_t songLen);

    // Renders `frames` interleaved-stereo sample pairs (2 int16 each). Returns
    // frames produced, or 0 once the song has played one full loop.
    int generate(int16_t* out, int frames);

    bool finished() const { return finished_; }

private:
    // CALL `entry`, running until it RETs to the idle trap (or a recognised
    // loop trap fires, or we run dry). Returns false only if the code ran away.
    bool call(uint16_t entry);
    bool runSlice(Z80_Cpu::time_t end);

    Z80_Cpu cpu_;
    std::vector<uint8_t> ram_; // 32K image, mirrored across 64K via map_mem
    CSAASound* saa_ = nullptr;
    int sampleRate_;
    int samplesPerTick_;
    int tickPhase_ = 0;

    uint8_t port_[256] = {0};
    uint8_t saaLatch_ = 0; // last value written to the SAA address port

    bool eTracker_ = false;
    uint16_t playEntry_ = 0x8000;

    bool finished_ = false;
    long ticksDone_ = 0;
};

} // namespace musix::cop
