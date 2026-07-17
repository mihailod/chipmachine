#include "vt_machine.h"

#include "fake6502.h"
#include "vic_sound.h"
#include "vtplayer_bin.h"

#include <algorithm>
#include <cstring>

namespace musix::victracker {

namespace {

// PAL MOS6561 dot clock (Hz). The Swedish VIC-TRACKER corpus is all PAL; pitch
// and tempo derive from this rate regardless of the tune's NTSC/PAL play mode.
constexpr int VIC_CLOCK_PAL = 1108404;
// Screen (interrupt) periods in VIC cycles: a full PAL frame, and the shorter
// period a PAL machine uses to fake NTSC refresh. (SCREENTIME_PAL / _PAL_NTSC in
// the vendored vic20.i.)
constexpr int SCREENTIME_PAL = 22150;
constexpr int SCREENTIME_PAL_NTSC = 18383;

// A free landing pad between the player blob (~$2000-$2520) and the tune
// ($3300): CALLs return here and we detect completion by PC reaching it.
constexpr uint16_t TRAP = 0x2F00;

// The full T1 struct is 10432 bytes; .vt files store only the used prefix and
// the player expects the rest zero-filled in RAM (see playerdata.asm).
constexpr size_t T1_STRUCT_SIZE = 10432;

// Play the tune for at most this many interrupt ticks (a backstop; these tunes
// loop forever, so the host's default song length / fade normally stops first).
constexpr int MAX_SECONDS = 600;

// The one machine whose memory the global fake6502 hooks currently address.
// Playback is single-threaded (one song at a time), so a simple active pointer
// is sufficient and avoids threading a context userdata pointer fake6502 lacks.
VTMachine* g_active = nullptr;

} // namespace

} // namespace musix::victracker

// fake6502 memory hooks (global, as the core requires). Route to the active
// machine's 64K image.
extern "C" uint8_t fake6502_mem_read(fake6502_context* /*c*/, uint16_t addr)
{
    return musix::victracker::g_active->mem()[addr];
}
extern "C" void fake6502_mem_write(fake6502_context* /*c*/, uint16_t addr,
                                   uint8_t val)
{
    musix::victracker::g_active->mem()[addr] = val;
}

namespace musix::victracker {

VTMachine::VTMachine(int sampleRate)
    : mem_(0x10000, 0), sampleRate_(sampleRate), mono_(4096)
{
}

void VTMachine::callVector(uint16_t entry)
{
    fake6502_context cpu;
    std::memset(&cpu, 0, sizeof(cpu));
    fake6502_reset(&cpu);
    cpu.cpu.s = 0xFD;
    fake6502_push_16(&cpu, TRAP - 1); // RTS -> TRAP
    cpu.cpu.pc = entry;
    // Generous instruction budget; a single pl_Play/pl_Init is a few thousand.
    for (int i = 0; i < 2000000; i++) {
        if (cpu.cpu.pc == TRAP) {
            return;
        }
        fake6502_step(&cpu);
    }
}

bool VTMachine::init(const uint8_t* data, size_t len, int subsong)
{
    if (len < 6 || data[0] != 0x00 || data[1] != 0x33) {
        return false; // not a $3300 PRG
    }
    const uint8_t* tune = data + 2; // strip load address
    size_t tuneLen = len - 2;
    bool t1 = (tune[0] == 'T' && tune[1] == '1');
    bool t0 = (tune[0] == 'T' && tune[1] == '0');
    if (!t1 && !t0) {
        return false;
    }

    std::fill(mem_.begin(), mem_.end(), uint8_t{0});
    std::memcpy(mem_.data() + VT_BLOB_ORG, vtplayer_bin, vtplayer_bin_len);
    size_t copy = std::min(tuneLen, T1_STRUCT_SIZE);
    std::memcpy(mem_.data() + VT_TUNE_ADDR, tune, copy);

    subsongs_ = std::max<int>(1, mem_[VT_TUNE_ADDR + 7]); // pl_SongNum
    subsong = std::clamp(subsong, 0, subsongs_ - 1);

    // Interrupt rate from pl_PlayMode (T0 has no play-mode field -> PAL 50Hz).
    int mode = t1 ? mem_[VT_TUNE_ADDR + 4] : 1;
    static const int mult[11] = {1, 1, 2, 3, 4, 1, 2, 3, 4, 1, 1};
    bool ntscTiming = (mode >= 5 && mode <= 8);
    int frame = ntscTiming ? SCREENTIME_PAL_NTSC : SCREENTIME_PAL;
    interval_ = frame / mult[mode % 11];
    int playHz = VIC_CLOCK_PAL / interval_;
    maxTicks_ = static_cast<long>(playHz) * MAX_SECONDS;

    g_active = this;
    mem_[VT_PL_THISSONG] = static_cast<uint8_t>(subsong);
    callVector(VT_ENTRY_INIT);
    mem_[VT_PL_PLAYFLAG] = 1; // arm playback (pl_Init leaves it cleared)

    vicsnd_init(VIC_CLOCK_PAL, sampleRate_);
    return true;
}

void VTMachine::refill()
{
    g_active = this;
    callVector(VT_ENTRY_PLAY);
    // The player has written the frame's VIC register values; latch them.
    for (int r = 0xA; r <= 0xE; r++) {
        vicsnd_store(r, mem_[0x9000 + r]);
    }
    deltaCycles_ += interval_;
    pending_ = vicsnd_render(mono_.data(), static_cast<int>(mono_.size()), 1,
                             &deltaCycles_);
    monoPos_ = 0;
    if (++ticks_ > maxTicks_) {
        finished_ = true;
    }
}

int VTMachine::generate(int16_t* out, int frames)
{
    int produced = 0;
    while (produced < frames) {
        if (monoPos_ >= pending_) {
            if (finished_) {
                return produced; // 0 at a tick boundary => song end
            }
            refill();
        }
        while (produced < frames && monoPos_ < pending_) {
            int16_t s = mono_[monoPos_++];
            out[produced * 2] = s;     // duplicate mono to stereo
            out[produced * 2 + 1] = s;
            produced++;
        }
    }
    return produced;
}

} // namespace musix::victracker
