#pragma once

// A minimal ZX Spectrum-flavoured Z80 machine for running beeper music
// engines. It is engine-agnostic: it provides 64K of flat RAM, runs a routine
// on the vendored GME Z80 core, and samples the 1-bit speaker (bit 4 of port
// 0xFE) into PCM.
//
// Beeper engines work by disabling interrupts and busy-looping, toggling the
// speaker in cycle-counted loops; they only return (to our injected idle
// address) when the song ends. So we don't drive a 50Hz interrupt -- we just
// run the CPU in bounded cycle slices and integrate the speaker level over each
// output sample's worth of cycles.

#include "gme/Z80_Cpu.h"

#include <cstdint>
#include <vector>

namespace musix::bbsong {

// ZX Spectrum 48K CPU clock.
constexpr double Z80_CLOCK = 3500000.0;

class Z80Machine
{
public:
    Z80Machine();

    // Direct access to the 64K address space (e.g. to place player code and
    // packed song data before running).
    uint8_t* ram() { return ram_.data(); }
    void poke(uint16_t addr, const uint8_t* data, size_t n);

    // Maps the ZX Spectrum 48K ROM at 0x0000. Beeper engines call ROM routines
    // (e.g. KEY-SCAN), so this must be loaded after the player/data image.
    void loadSpectrumRom();

    // Sets PC to `entry` with our idle address pushed as the return address, so
    // a routine that RETs (rather than looping forever) lands on `finished()`.
    void start(uint16_t entry);

    bool finished() const { return finished_; }

    // Runs the engine and renders `frames` mono samples at `sampleRate` into
    // `out`, integrating the speaker level over each frame's cycles. Returns
    // the number of frames produced (always `frames` unless the song ended).
    int generate(int16_t* out, int frames, int sampleRate, int16_t amplitude);

private:
    // Runs the CPU until its slice clock reaches `endCycle`, recording speaker
    // edges. Defined in z80_machine.cpp via Z80_Cpu_run.h. Returns true if the
    // routine returned to the idle address (song finished).
    bool runSlice(Z80_Cpu::time_t endCycle);

    // A speaker transition: the slice-relative cycle at which the level became
    // `level` (0 or 1).
    struct Edge
    {
        Z80_Cpu::time_t cycle;
        uint8_t level;
    };

    void out(Z80_Cpu::time_t time, int port, int data);

    // Raises a maskable interrupt (if enabled): pushes the resume PC and vectors
    // to the handler. Engines like SFX run their playback from a 50Hz IM2 ISR;
    // engines that `di` and never `ei` leave iff1=0, so this is a no-op for them.
    void injectInterrupt();

    Z80_Cpu cpu_;
    std::vector<uint8_t> ram_;
    std::vector<Edge> edges_;
    uint8_t speakerLevel_ = 0;
    uint16_t idleAddr_ = 0xFFFF;
    bool finished_ = false;
    double cycleRemainder_ = 0.0; // fractional cycles carried between slices
    long frameCounter_ = 0;       // cycles since last 50Hz interrupt
    Z80_Cpu::time_t edgeBase_ = 0; // offset added to edge timestamps within a block
};

} // namespace musix::bbsong
