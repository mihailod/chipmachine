#pragma once

// Minimal MSX-flavoured machine for the SCC-Musixx replayer (REPLAY.BIN).
//
// SCC-Musixx songs are raw MSX memory images that the Tyfoon-Software replay
// routine interprets, driving Konami's SCC (wavetable) sound chip. We run that
// original Z80 routine on the vendored GME Z80 core; the SCC memory window
// (0x9000 bank-enable + 0x9800-0x98FF registers) is routed into emu2212 (the
// SCC emulator vendored with libkss). No MSX BIOS is emulated: the handful of
// touch points the replayer needs are stubbed (see scc_machine.cpp).
//
// The song image loads at 0x0000; the replayer body loads at 0xD000. We CALL
// its START routine (which finds the SCC, inits, and installs a 50Hz H.TIMI
// handler), then CALL that handler once per 1/50s tick, rendering SCC audio
// between ticks.

#include "gme/Z80_Cpu.h"

#include <cstddef>
#include <cstdint>
#include <vector>

struct __SCC; // emu2212 SCC, opaque here

namespace musix::sccmusixx {

// MSX system clock that feeds the SCC.
constexpr uint32_t SCC_CLOCK = 3579545;
// SCC-Musixx PAL songs (the modland corpus is tagged [50Hz]) tick at 50Hz.
constexpr int PLAY_HZ = 50;

class SccMachine
{
public:
    explicit SccMachine(int sampleRate);
    ~SccMachine();
    SccMachine(const SccMachine&) = delete;
    SccMachine& operator=(const SccMachine&) = delete;

    // Loads the song image at 0x0000 and the replayer at its load address, then
    // runs START. Returns false if the replayer failed to install its handler.
    bool init(const uint8_t* song, size_t songLen);

    // Renders `frames` mono samples. Returns frames produced, or 0 once the song
    // has played one full sequence loop (so the host can advance/fade).
    int generate(int16_t* out, int frames);

    bool finished() const { return finished_; }

private:
    bool call(uint16_t entry, long maxCycles = 4000000);
    bool runSlice(Z80_Cpu::time_t end);
    void writeMem(uint16_t a, uint8_t d);
    uint8_t readMem(uint16_t a);
    void poke(uint16_t a, const uint8_t* d, size_t n);

    Z80_Cpu cpu_;
    std::vector<uint8_t> ram_;
    __SCC* scc_ = nullptr;
    int sampleRate_;
    int samplesPerTick_;
    int tickPhase_ = 0; // samples rendered into the current 50Hz tick

    uint8_t last9000_ = 0;
    uint8_t port_[256] = {0};
    uint16_t handler_ = 0;

    // Loop/end detection via the replayer's POSIT byte (sequence position).
    // A song ends when the sequence jumps backwards (POSIT decreases) -- that is
    // the loop restart, which may target an arbitrary position, not just 0
    // (e.g. an intro that isn't replayed). One play-through, then we stop.
    uint16_t positAddr_ = 0;
    int startPosit_ = -1;
    int lastPosit_ = -1;
    bool positMoved_ = false;
    bool finished_ = false;
    long ticksDone_ = 0;
    long lastChangeTick_ = 0; // tick of the last POSIT change (stall detection)

    // High-pass (DC blocker): emu2212's SCC mix is not centred on zero.
    float dcX_ = 0.0f;
    float dcY_ = 0.0f;
};

} // namespace musix::sccmusixx
