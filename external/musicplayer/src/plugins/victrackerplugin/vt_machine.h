#pragma once

// Minimal VIC-20 machine for VIC-TRACKER (.vt) tunes.
//
// A .vt file is a Commodore VIC-20 PRG: a 2-byte $3300 load address followed by
// Daniel Kahlin's VIC-TRACKER "T1" (or legacy "T0") tune struct. The tune is
// interpreted by the tracker's own 6502 replay routine (player.asm), which we
// vendor pre-assembled as a position-fixed blob (vtplayer_bin.h, BSD) and run on
// the fake6502 CPU core. The player's writes to the VIC sound registers
// ($900A-$900E) are fed into the VIC-I sound core lifted from VICE (vic_sound.*).
//
// We drive the player exactly as the VIC-20 IRQ would: load the tune at $3300,
// call pl_Init once, then call pl_Play once per interrupt tick (rate set by the
// tune's pl_PlayMode byte) and clock the sound chip for that many VIC cycles.

#include <cstddef>
#include <cstdint>
#include <vector>

namespace musix::victracker {

class VTMachine
{
public:
    explicit VTMachine(int sampleRate);

    VTMachine(const VTMachine&) = delete;
    VTMachine& operator=(const VTMachine&) = delete;

    // Loads a .vt image (with its 2-byte load address) and runs pl_Init for
    // `subsong`. Returns false if the data is not a VIC-TRACKER tune.
    bool init(const uint8_t* data, size_t len, int subsong = 0);

    // Renders `frames` interleaved-stereo sample pairs. Returns frames produced,
    // or 0 once the play cap is reached.
    int generate(int16_t* out, int frames);

    int subsongCount() const { return subsongs_; }

    // Access for the fake6502 memory hooks (single active machine at a time).
    uint8_t* mem() { return mem_.data(); }

private:
    void callVector(uint16_t entry);
    void refill();

    std::vector<uint8_t> mem_; // full 64K address space
    int sampleRate_;

    int interval_ = 0;   // VIC cycles between pl_Play calls
    long ticks_ = 0;
    long maxTicks_ = 0;
    bool finished_ = false;
    int subsongs_ = 1;

    // pending mono samples produced by the last pl_Play tick
    std::vector<int16_t> mono_;
    int pending_ = 0;
    int monoPos_ = 0;
    int deltaCycles_ = 0;
};

} // namespace musix::victracker
