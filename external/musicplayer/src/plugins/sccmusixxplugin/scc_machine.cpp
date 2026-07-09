#include "scc_machine.h"

#include "replay_bin.h"

#include "gme/blargg_endian.h"
#include "gme/blargg_source.h"

extern "C" {
#include "scc_emu_names.h" // rename SCC_* to our private copy's symbols
#include "emu2212.h"
}

#include <algorithm>
#include <cstring>

namespace musix::sccmusixx {

namespace {
// A "JR $" self-loop we use as the return target of a CALL: when the replayer's
// routine RETs, it lands here and spins until the cycle slice runs out, and we
// detect completion via PC == IDLE. Parked in free RAM (above the song image,
// below the replayer, outside the SCC window).
constexpr uint16_t IDLE = 0xBFF0;

// Don't end before this many ticks (some songs are tiny loops) and never run
// past the hard cap.
constexpr long MIN_TICKS = PLAY_HZ * 2;
constexpr long MAX_TICKS = PLAY_HZ * 600;
// Some songs don't loop back -- they sustain/freeze on their last sequence
// position. If POSIT hasn't advanced for this long, treat the song as ended.
// (Observed inter-position gaps are a few seconds at most, so 15s is safe.)
constexpr long STALE_TICKS = PLAY_HZ * 15;
} // namespace

SccMachine::SccMachine(int sampleRate)
    : ram_(0x10000 + Z80_Cpu::cpu_padding, 0), sampleRate_(sampleRate),
      samplesPerTick_(sampleRate / PLAY_HZ)
{
    cpu_.reset(ram_.data(), ram_.data());
    cpu_.map_mem(0, 0x10000, ram_.data());
    scc_ = SCC_new(SCC_CLOCK, (uint32_t)sampleRate);
    SCC_set_type(scc_, SCC_STANDARD);
    SCC_reset(scc_);
    ram_[IDLE] = 0x18; // JR $
    ram_[IDLE + 1] = 0xFE;
    positAddr_ = REPLAY_LOAD_ADDR + 7; // POSIT byte (sequence position)
}

SccMachine::~SccMachine()
{
    if (scc_ != nullptr) {
        SCC_delete(scc_);
    }
}

void SccMachine::poke(uint16_t a, const uint8_t* d, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        ram_[(a + i) & 0xFFFF] = d[i];
    }
}

bool SccMachine::init(const uint8_t* song, size_t songLen)
{
    poke(0x0000, song, songLen);                          // song image at 0x0000
    poke(REPLAY_LOAD_ADDR, REPLAY_BIN, sizeof(REPLAY_BIN)); // replayer body
    ram_[0x0024] = 0xC9;                                  // ENASLT stub = RET

    if (!call(REPLAY_START)) {
        return false;
    }
    handler_ = ram_[0xFDA0] | (ram_[0xFDA1] << 8); // installed H.TIMI handler
    if (handler_ == 0) {
        return false;
    }
    startPosit_ = ram_[positAddr_];
    lastPosit_ = startPosit_;
    return true;
}

// CALL `entry`, running until it RETs to the idle loop (or we run dry).
bool SccMachine::call(uint16_t entry, long maxCycles)
{
    cpu_.r.pc = entry;
    cpu_.r.sp = 0xCF00;
    ram_[--cpu_.r.sp & 0xFFFF] = IDLE >> 8;
    ram_[--cpu_.r.sp & 0xFFFF] = IDLE & 0xFF;
    cpu_.set_time(0);
    long done = 0;
    while (done < maxCycles) {
        constexpr long chunk = 50000;
        runSlice(chunk);
        cpu_.adjust_time(-chunk);
        done += chunk;
        if (cpu_.r.pc == IDLE) {
            return true;
        }
    }
    return false; // ran away
}

int SccMachine::generate(int16_t* out, int frames)
{
    if (finished_) {
        std::memset(out, 0, frames * sizeof(int16_t));
        return 0;
    }
    for (int i = 0; i < frames; i++) {
        if (tickPhase_ == 0) {
            // One player tick (the 50Hz H.TIMI handler).
            call(handler_);
            ticksDone_++;
            int p = ram_[positAddr_];
            if (p != startPosit_) {
                positMoved_ = true;
            }
            // A backwards jump in the sequence position is the loop restart:
            // we've played through once.
            if (positMoved_ && p < lastPosit_ && ticksDone_ > MIN_TICKS) {
                finished_ = true;
            }
            if (p != lastPosit_) {
                lastChangeTick_ = ticksDone_;
            } else if (positMoved_ && ticksDone_ - lastChangeTick_ > STALE_TICKS) {
                finished_ = true; // sequence position frozen -> song ended
            }
            if (ticksDone_ > MAX_TICKS) {
                finished_ = true;
            }
            lastPosit_ = p;
        }
        int16_t s = SCC_calc(scc_);
        // One-pole DC blocker (emu2212's SCC mix is not centred on zero):
        // y[n] = x[n] - x[n-1] + R*y[n-1]. A modest gain brings the level into
        // line with the other plugins; the clamp guards loud outliers.
        float x = (float)s;
        float y = x - dcX_ + 0.999f * dcY_;
        dcX_ = x;
        dcY_ = y;
        int v = (int)(y * 1.8f);
        out[i] = (int16_t)std::clamp(v, -32768, 32767);

        if (++tickPhase_ >= samplesPerTick_) {
            tickPhase_ = 0;
        }
        if (finished_) {
            return i + 1;
        }
    }
    return frames;
}

void SccMachine::writeMem(uint16_t a, uint8_t d)
{
    if (a == 0x9000) {
        last9000_ = d;
        SCC_write(scc_, 0x9000, d);
        return;
    }
    if (a >= 0x9800 && a <= 0x98FF) {
        SCC_write(scc_, a, d);
        return;
    }
    ram_[a] = d;
}

uint8_t SccMachine::readMem(uint16_t a)
{
    // Satisfy the replayer's SCC search. It writes 0 then requires a nonzero
    // read-back, then writes (readback^0xFF) and requires the next read to
    // differ. On real hardware that's the megarom bank window returning ROM
    // bytes; we just need read(v) = v^0x01: write 0 -> 1, write 0xFE -> 0xFF.
    if (a == 0x9000) {
        return (uint8_t)(last9000_ ^ 0x01);
    }
    return ram_[a];
}

// --- GME Z80 core run body ---------------------------------------------------
// Same integration pattern as bbsong's z80_machine.cpp / the GME *_Cpu.cpp
// files: define the access macros, then #include the core's run body. Memory
// accesses route through writeMem/readMem so the SCC window hits emu2212; the
// Z80_Cpu methods themselves resolve at final link against gmeplugin's
// Z80_Cpu.o (we only need its headers here).
#define CPU cpu_
#define OUT_PORT(addr, data) (port_[(addr) & 0xFF] = (uint8_t)(data))
#define IN_PORT(addr) (port_[(addr) & 0xFF])
#define READ_MEM(addr) readMem((uint16_t)(addr))
#define WRITE_MEM(addr, data) writeMem((uint16_t)(addr), (uint8_t)(data))
#define IDLE_ADDR IDLE
#define RST_BASE 0

#define CPU_BEGIN                                          \
    bool SccMachine::runSlice(Z80_Cpu::time_t end)         \
    {                                                      \
        bool idle = false;                                 \
        CPU.set_end_time(end);

#include "gme/Z80_Cpu_run.h"

    idle = (CPU.r.pc == IDLE);
    return idle;
}

} // namespace musix::sccmusixx
