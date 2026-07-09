#include "z80_machine.h"

#include <algorithm>
#include <cstring>

// The GME Z80 core's run body (Z80_Cpu_run.h, #included below) expects the same
// preamble its sibling *_Cpu.cpp files get: the `byte` type and `check` macro
// from blargg_source.h, and the little-endian word macros from blargg_endian.h.
#include "gme/blargg_endian.h"
#include "gme/blargg_source.h"

#include "spectrum_rom.h"

namespace musix::bbsong {

Z80Machine::Z80Machine() : ram_(0x10000 + Z80_Cpu::cpu_padding, 0)
{
    // Flat 64K of RAM, everything readable and writable (self-modifying engine
    // code relies on this). Unmapped accesses fall back to the same array.
    cpu_.reset(ram_.data(), ram_.data());
    cpu_.map_mem(0, 0x10000, ram_.data());

    // Park a tiny "JR $" (0x18 0xFE) at the idle address; when an engine's
    // routine RETs at end-of-song it lands here and spins, and we detect it via
    // PC after the slice.
    ram_[idleAddr_] = 0x18;
    ram_[(idleAddr_ + 1) & 0xFFFF] = 0xFE;
}

void Z80Machine::poke(uint16_t addr, const uint8_t* data, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        ram_[(addr + i) & 0xFFFF] = data[i];
    }
}

void Z80Machine::loadSpectrumRom()
{
    poke(0x0000, SPECTRUM48_ROM, sizeof(SPECTRUM48_ROM));
    // The SFX IM2 vector trick puts a JR at 0xFFFF whose offset byte must be
    // read from 0x0000 (16-bit address wrap). The GME core fetches that byte
    // from the flat buffer's padding past 0xFFFF instead of wrapping, so mirror
    // the low bytes (ROM, fixed) into the padding region.
    for (size_t i = 0; i < Z80_Cpu::cpu_padding; i++) {
        ram_[0x10000 + i] = ram_[i];
    }
}

void Z80Machine::start(uint16_t entry)
{
    cpu_.r.pc = entry;
    // Stack below the IM2 interrupt vector table (0xFE00-0xFF00) that the SFX
    // engine fills during init, so early pushes/returns aren't clobbered. Well
    // above any engine's player+data (which sit from 0x8000), and engines that
    // care reset SP themselves anyway.
    cpu_.r.sp = 0xFDF0;
    // (Re)park "JR $" at the idle address in case loading touched it, then push
    // it as the routine's return target.
    ram_[idleAddr_] = 0x18;
    ram_[(idleAddr_ + 1) & 0xFFFF] = 0xFE;
    ram_[--cpu_.r.sp & 0xFFFF] = idleAddr_ >> 8;
    ram_[--cpu_.r.sp & 0xFFFF] = idleAddr_ & 0xFF;
    cpu_.set_time(0);
    finished_ = false;
    speakerLevel_ = 0;
    cycleRemainder_ = 0.0;
    frameCounter_ = 0;
    edgeBase_ = 0;
}

void Z80Machine::out(Z80_Cpu::time_t time, int port, int data)
{
    // The speaker is bit 4 of port 0xFE (only the low address byte is decoded).
    if ((port & 0xFF) == 0xFE) {
        uint8_t level = (data >> 4) & 1;
        if (level != speakerLevel_) {
            edges_.push_back({time + edgeBase_, level});
            speakerLevel_ = level;
        }
    }
}

void Z80Machine::injectInterrupt()
{
    if (!cpu_.r.iff1) {
        return; // interrupts disabled (the di/busy-loop engines)
    }
    // Resume address: if the CPU is sitting on a HALT, continue past it.
    uint16_t pc = cpu_.r.pc;
    uint16_t ret = (ram_[pc] == 0x76) ? static_cast<uint16_t>(pc + 1) : pc;
    uint16_t sp = cpu_.r.sp;
    ram_[static_cast<uint16_t>(--sp)] = ret >> 8;
    ram_[static_cast<uint16_t>(--sp)] = ret & 0xFF;
    cpu_.r.sp = sp;
    cpu_.r.iff1 = 0;
    cpu_.r.iff2 = 0;
    if (cpu_.r.im == 2) {
        // IM2: vector address = (I << 8) | bus byte; the Spectrum's floating
        // bus reads 0xFF during interrupt acknowledge.
        uint16_t vec = static_cast<uint16_t>((cpu_.r.i << 8) | 0xFF);
        cpu_.r.pc = ram_[vec] | (ram_[static_cast<uint16_t>(vec + 1)] << 8);
    } else {
        cpu_.r.pc = 0x0038; // IM0/IM1 -> RST 38h
    }
}

int Z80Machine::generate(int16_t* out, int frames, int sampleRate,
                         int16_t amplitude)
{
    if (frames <= 0) {
        return 0;
    }
    if (finished_) {
        std::memset(out, 0, frames * sizeof(int16_t));
        return 0;
    }

    const double cyclesPerSample = Z80_CLOCK / sampleRate;
    double exact = cyclesPerSample * frames + cycleRemainder_;
    Z80_Cpu::time_t cycles = static_cast<Z80_Cpu::time_t>(exact);
    cycleRemainder_ = exact - cycles;

    // The level the speaker holds entering this block.
    uint8_t levelAtStart = speakerLevel_;
    edges_.clear();

    // Run `cycles`, but break at 50Hz interrupt boundaries so interrupt-driven
    // engines (SFX) get their ISR fired. Each sub-slice's speaker edges are
    // offset by `edgeBase_` so they land at the right place in the block.
    constexpr long FRAME_CYCLES = static_cast<long>(Z80_CLOCK / 50.0); // 70000
    long done = 0;
    bool hitIdle = false;
    while (done < cycles) {
        long toInt = FRAME_CYCLES - frameCounter_;
        if (toInt <= 0) {
            toInt = FRAME_CYCLES;
        }
        long chunk = std::min(static_cast<long>(cycles) - done, toInt);
        edgeBase_ = static_cast<Z80_Cpu::time_t>(done);
        if (runSlice(static_cast<Z80_Cpu::time_t>(chunk))) {
            hitIdle = true;
        }
        cpu_.adjust_time(-static_cast<Z80_Cpu::time_t>(chunk));
        done += chunk;
        frameCounter_ += chunk;
        if (frameCounter_ >= FRAME_CYCLES) {
            frameCounter_ -= FRAME_CYCLES;
            injectInterrupt();
        }
    }

    // Clamp edge cycles into [0, cycles] and integrate the 1-bit level over each
    // output sample's cycle window (a box filter -- cheap anti-aliasing for the
    // square wave). fraction-high in [0,1] maps to [-amp, +amp].
    uint8_t lvl = levelAtStart;
    size_t ei = 0;
    for (int i = 0; i < frames; i++) {
        long c0 = static_cast<long>(static_cast<long long>(i) * cycles / frames);
        long c1 =
            static_cast<long>(static_cast<long long>(i + 1) * cycles / frames);
        long high = 0;
        long pos = c0;
        while (ei < edges_.size() && edges_[ei].cycle < c1) {
            long ec = std::max<long>(edges_[ei].cycle, c0);
            if (lvl) {
                high += ec - pos;
            }
            pos = ec;
            lvl = edges_[ei].level;
            ei++;
        }
        if (lvl) {
            high += c1 - pos;
        }
        long span = c1 - c0;
        out[i] = span > 0
                     ? static_cast<int16_t>(((2 * high - span) * amplitude) / span)
                     : 0;
    }
    speakerLevel_ = lvl;

    if (hitIdle) {
        finished_ = true;
    }
    return frames;
}

// --- Z80 core run loop -------------------------------------------------------
// Same integration pattern as the GME *_Cpu.cpp files: define the access macros,
// then #include the core's run body inside our method.

#define CPU cpu_
#define OUT_PORT(addr, data) out(TIME(), addr, data)
#define IN_PORT(addr) (0xFF) // no keys pressed / EAR low
#define IDLE_ADDR idleAddr_
#define RST_BASE 0

#define CPU_BEGIN                                          \
    bool Z80Machine::runSlice(Z80_Cpu::time_t end)         \
    {                                                      \
        bool idle = false;                                 \
        CPU.set_end_time(end);

#include "gme/Z80_Cpu_run.h"

    // After the run body falls through (out of time), report whether we ended
    // up spinning at the idle address.
    idle = (CPU.r.pc == idleAddr_);
    return idle;
}

} // namespace musix::bbsong
