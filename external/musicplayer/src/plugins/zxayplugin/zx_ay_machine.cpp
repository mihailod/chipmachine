#include "zx_ay_machine.h"

#include <algorithm>
#include <cmath>
#include <cstring>

// The GME Z80 core's run body (Z80_Cpu_run.h, #included at the bottom) expects
// the same preamble its sibling *_Cpu.cpp files get.
#include "gme/blargg_endian.h"
#include "gme/blargg_source.h"

namespace musix::zxay {

namespace {

// Ayumi is configured for the AY-3-8910 family rather than the YM2149: the ZX
// Spectrum 128 and the +2 both carry an AY-3-8912, and its 16-step volume
// curve is what this music was written against.
constexpr int kIsYm = 0;

// Channel panning per stereo mode, {A, B, C} with 0 = hard left, 1 = hard
// right. Not fully hard-panned: real ABC-stereo machines bleed a long way, and
// 15%/85% is the usual compromise (fully hard-panned AY is fatiguing on
// headphones and collapses badly in mono).
constexpr double kPanAbc[3] = {0.15, 0.5, 0.85};
constexpr double kPanAcb[3] = {0.15, 0.85, 0.5};
constexpr double kPanMono[3] = {0.5, 0.5, 0.5};

} // namespace

ZxAyMachine::ZxAyMachine(int sampleRate, Stereo stereo, double ayClock)
    : ram_(0x10000 + Z80_Cpu::cpu_padding, 0), sampleRate_(sampleRate)
{
    samplesPerTick_ = sampleRate / PLAY_HZ;

    ayumi_configure(&ay_, kIsYm, ayClock, sampleRate);
    const double* pan = stereo == Stereo::acb    ? kPanAcb
                        : stereo == Stereo::mono ? kPanMono
                                                 : kPanAbc;
    for (int i = 0; i < TONE_CHANNELS; i++) {
        // is_eqp=1 = equal-power panning, which keeps the summed loudness
        // constant as a channel moves off centre.
        ayumi_set_pan(&ay_, i, pan[i], 1);
    }
    // Silence: all three channels off in the mixer, all volumes zero.
    writeRegister(7, 0x3F);

    // Flat 64K, everything readable and writable -- these players are heavily
    // self-modifying (Bulba's own note: "player is not reallocable").
    cpu_.reset(ram_.data(), ram_.data());
    cpu_.map_mem(0, 0x10000, ram_.data());
}

void ZxAyMachine::poke(uint16_t addr, const uint8_t* data, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        ram_[(addr + i) & 0xFFFF] = data[i];
    }
}

// --- AY register file --------------------------------------------------------
// Ayumi exposes the chip decomposed into setters rather than as 14 registers,
// so this is the register file the Z80 sees.

void ZxAyMachine::writeRegister(int reg, uint8_t value)
{
    if (reg < 0 || reg > 15) {
        return;
    }
    regs_[reg] = value;
    switch (reg) {
    case 0:
    case 1:
        ayumi_set_tone(&ay_, 0, (regs_[1] & 0x0F) << 8 | regs_[0]);
        break;
    case 2:
    case 3:
        ayumi_set_tone(&ay_, 1, (regs_[3] & 0x0F) << 8 | regs_[2]);
        break;
    case 4:
    case 5:
        ayumi_set_tone(&ay_, 2, (regs_[5] & 0x0F) << 8 | regs_[4]);
        break;
    case 6:
        ayumi_set_noise(&ay_, value & 0x1F);
        break;
    case 7:
        for (int i = 0; i < TONE_CHANNELS; i++) {
            ayumi_set_mixer(&ay_, i, (value >> i) & 1, (value >> (i + 3)) & 1,
                            (regs_[8 + i] >> 4) & 1);
        }
        break;
    case 8:
    case 9:
    case 10: {
        int ch = reg - 8;
        ayumi_set_mixer(&ay_, ch, (regs_[7] >> ch) & 1,
                        (regs_[7] >> (ch + 3)) & 1, (value >> 4) & 1);
        ayumi_set_volume(&ay_, ch, value & 0x0F);
        break;
    }
    case 11:
    case 12:
        ayumi_set_envelope(&ay_, regs_[12] << 8 | regs_[11]);
        break;
    case 13:
        // Writing R13 always restarts the envelope, even with an unchanged
        // shape -- tracker vibrato/"envelope retrigger" effects depend on it,
        // so this must not be short-circuited.
        ayumi_set_envelope_shape(&ay_, value & 0x0F);
        break;
    default:
        break; // R14/R15 are the I/O ports; nothing here drives them
    }
}

void ZxAyMachine::out(int port, int data)
{
    // ZX Spectrum 128 partial decode: A1 low selects the AY, A15/A14 pick
    // register-select (0xFFFD) vs data-write (0xBFFD). Players write the
    // canonical addresses, but a few use OUTD/OUTI with a stale high byte, so
    // decode properly rather than comparing to 0xFFFD/0xBFFD outright.
    if ((port & 0x0002) != 0) {
        return;
    }
    if ((port & 0xC000) == 0xC000) {
        selectedReg_ = data & 0x0F;
    } else if ((port & 0xC000) == 0x8000) {
        writeRegister(selectedReg_, static_cast<uint8_t>(data));
    }
}

// --- calling the player ------------------------------------------------------

bool ZxAyMachine::call(uint16_t entry, uint16_t hl, uint8_t a)
{
    // "JR $" at the idle address: a routine that RETs lands here and spins, so
    // PC == idleAddr_ after a slice means it returned.
    ram_[idleAddr_] = 0x18;
    ram_[static_cast<uint16_t>(idleAddr_ + 1)] = 0xFE;

    cpu_.r.pc = entry;
    // Stack just under the idle stub. Every player in this plugin loads at
    // 0x8000 or 0xC000 with its module below it, so this is clear of both --
    // and the ones that care set SP themselves during INIT anyway.
    cpu_.r.sp = 0xFFF0;
    ram_[static_cast<uint16_t>(--cpu_.r.sp)] = idleAddr_ >> 8;
    ram_[static_cast<uint16_t>(--cpu_.r.sp)] = idleAddr_ & 0xFF;
    cpu_.r.w.hl = hl;
    cpu_.r.b.a = a;
    cpu_.r.iff1 = 0;
    cpu_.r.iff2 = 0;
    cpu_.set_time(0);

    long done = 0;
    constexpr long kChunk = 20000;
    while (done < kCallCycleCap) {
        long chunk = std::min(kChunk, kCallCycleCap - done);
        bool idle = runSlice(static_cast<Z80_Cpu::time_t>(chunk));
        cpu_.adjust_time(-static_cast<Z80_Cpu::time_t>(chunk));
        done += chunk;
        if (idle) {
            return true;
        }
    }
    return false;
}

// --- rendering ---------------------------------------------------------------

void ZxAyMachine::renderSamples(int16_t* out, int frames)
{
    for (int i = 0; i < frames; i++) {
        ayumi_process(&ay_);
        ayumi_remove_dc(&ay_);
        // Ayumi's output is nominally +/-1.0 but sums three channels, so it
        // overshoots on loud material; scale for headroom and clamp.
        double l = ay_.left * 0.7;
        double r = ay_.right * 0.7;
        out[i * 2] = static_cast<int16_t>(
            std::clamp(l * 32767.0, -32768.0, 32767.0));
        out[i * 2 + 1] = static_cast<int16_t>(
            std::clamp(r * 32767.0, -32768.0, 32767.0));
    }
}

int ZxAyMachine::renderTick(int16_t* out, int maxFrames)
{
    int n = std::min(maxFrames, samplesPerTick_);
    renderSamples(out, n);
    return n;
}

// --- Z80 core run loop -------------------------------------------------------
// Same integration pattern as the GME *_Cpu.cpp files and bbsongplugin: define
// the access macros, then #include the core's run body inside our method.

#define CPU cpu_
#define OUT_PORT(addr, data) out(addr, data)
#define IN_PORT(addr) (0xFF) // floating bus / no keys pressed
#define IDLE_ADDR idleAddr_
#define RST_BASE 0

#define CPU_BEGIN                                  \
    bool ZxAyMachine::runSlice(Z80_Cpu::time_t end) \
    {                                              \
        CPU.set_end_time(end);

#include "gme/Z80_Cpu_run.h"

    return CPU.r.pc == idleAddr_;
}

} // namespace musix::zxay
