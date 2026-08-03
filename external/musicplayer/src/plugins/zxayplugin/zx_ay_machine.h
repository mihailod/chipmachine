#pragma once

// A ZX Spectrum 128 machine just big enough to run an AY tracker's ORIGINAL
// Z80 replay routine.
//
// This is the same idea as copplugin's CopMachine (SAM Coupe COP songs on the
// GME Z80 core into SAASound) and bbsongplugin's Z80Machine (Beepola beeper
// engines, speaker sampled off port 0xFE). Here the routine is one of Sergey
// Bulba's published ZX replayers, and what we trap is the AY-3-8912: OUT to
// 0xFFFD selects a register and OUT to 0xBFFD writes it, and those writes go
// straight into Ayumi.
//
// Running the real Z80 code is what makes this worth doing. Every one of these
// formats has exactly one authoritative definition -- the tracker author's own
// player -- and hand-porting a sequencer is where the version-gated fixups,
// portamento variants and table quirks get lost. There is nothing to get wrong
// here beyond the calling convention, which the authors documented.
//
// Deliberately NOT interrupt-driven. The players are documented as "call INIT
// once, then call PLAY every 1/50 s from your own loop", so that is what we do;
// no IM2 vector table, no HALT, no ROM. Nothing here needs the Spectrum ROM
// either -- these routines touch the AY and their own workspace and nothing
// else.

#include "gme/Z80_Cpu.h"

#include <cstdint>
#include <vector>

extern "C" {
#include "ayumi/ayumi.h"
}

namespace musix::zxay {

// Pentagon 128 timings: a 3.5 MHz Z80 with the AY clocked at half of it, and a
// 50 Hz interrupt -- the rate every player here expects PLAY to be called at.
//
// Pentagon rather than a Sinclair 128K (3.546900 MHz / 1.773450 MHz) on
// purpose. Nearly all of this corpus is ex-USSR, where the Pentagon and its
// relatives were the machines these trackers ran on, and it is what Ayfly,
// ZXTune and AY_Emul all clock ZX AY music at. The difference is only 1.3%,
// but 1.3% is a fifth of a semitone and it is audible against a recording --
// and it decorrelates the two engines' spectra badly enough to hide real
// faults, which is what first exposed the mismatch here (see the "[.zxab]"
// A/B test in test.cpp).
inline constexpr double PENTAGON_CPU_CLOCK = 3500000.0;
inline constexpr double ZX128_AY_CLOCK = PENTAGON_CPU_CLOCK / 2.0; // 1750000
inline constexpr int PLAY_HZ = 50;

// Stereo layout. ZX AY music is written to be heard in stereo, and which one
// depends on the machine: ABC on most ex-USSR clones, ACB in Eastern Europe.
// ABC is the common default and what the players emit "as is".
enum class Stereo
{
    abc,
    acb,
    mono,
};

class ZxAyMachine
{
public:
    ZxAyMachine(int sampleRate, Stereo stereo = Stereo::abc,
                double ayClock = ZX128_AY_CLOCK);

    ZxAyMachine(const ZxAyMachine&) = delete;
    ZxAyMachine& operator=(const ZxAyMachine&) = delete;

    // --- memory ------------------------------------------------------------
    void poke(uint16_t addr, const uint8_t* data, size_t n);
    uint8_t peek(uint16_t addr) const { return ram_[addr]; }
    void pokeByte(uint16_t addr, uint8_t v) { ram_[addr] = v; }

    // --- running the routine ------------------------------------------------
    // Calls `entry` as a subroutine and runs until it returns, with `hl` and
    // `a` set up as the player's documented INIT convention wants them.
    // Returns false if the routine ran away (see kCallCycleCap) rather than
    // returning, which is how a mis-detected file usually presents.
    bool call(uint16_t entry, uint16_t hl = 0, uint8_t a = 0);

    // --- rendering ----------------------------------------------------------
    // Direct AY access, for the formats that have no Z80 player at all: the
    // register dumps (.vtx/.psg) and the native sequencers (.stc/.asc) drive
    // the chip through here and never start the CPU.
    void writeRegister(int reg, uint8_t value);
    // Renders one 1/50 s tick's worth of samples with no CPU involvement.
    int renderTick(int16_t* out, int maxFrames);
    int samplesPerTick() const { return samplesPerTick_; }

    // The 14 audible registers, whoever last wrote them -- this is what
    // LoopDetector watches.
    const uint8_t* registers() const { return regs_; }

private:
    // A play routine gets three interrupt periods' worth of CPU before we call
    // it hung. Bulba documents PTxPlay's worst case as 10500 T-states, i.e.
    // 15% of one frame, so this is not a tight budget -- it only catches a
    // routine that has jumped into the weeds, which is what a wrongly-detected
    // module does.
    static constexpr long kCallCycleCap = 3 * 70938L;

    // Defined in the .cpp via Z80_Cpu_run.h, like GME's own *_Cpu.cpp files.
    bool runSlice(Z80_Cpu::time_t end);
    void out(int port, int data);
    void renderSamples(int16_t* out, int frames);

    Z80_Cpu cpu_;
    std::vector<uint8_t> ram_;
    struct ayumi ay_;
    int sampleRate_;
    int samplesPerTick_;
    int selectedReg_ = 0;
    uint8_t regs_[16] = {};
    uint16_t idleAddr_ = 0xFFFE;
};

} // namespace musix::zxay
