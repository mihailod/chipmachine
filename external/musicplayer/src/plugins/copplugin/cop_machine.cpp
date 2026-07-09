#include "cop_machine.h"

#include "etracker_bin.h"
#include "SAASound.h"

#include "gme/blargg_endian.h"
#include "gme/blargg_source.h"

#include <algorithm>
#include <cstring>

namespace musix::cop {

namespace {
// The 32K SAM image is mirrored across the whole 64K space (the player's code
// runs at 0x8000+, aliasing the 0x0000+ load), reproducing SCPlayer's `& 0x7fff`
// masking.
constexpr uint16_t IMAGE_MASK = 0x7FFF;
constexpr size_t IMAGE_SIZE = 0x8000;

// Free byte just above the largest possible song load (SCPlayer parks its CALL
// stub here too). We plant a "JR $" self-loop and use it as the return target of
// a CALL: when the replayer RETs it lands here and spins until the cycle slice
// ends, and we detect completion via PC == IDLE.
constexpr uint16_t IDLE = 0x7000;

// E-Tracker "get_loop" routine. SCPlayer plants a trap here so it can detect the
// song looping back; we plant the same "JR $" and treat PC reaching it as the
// end of one play-through. (Only meaningful for the E-Tracker player, which is
// shared by both data files and "compiled" songs.)
constexpr uint16_t LOOP_TRAP = 0x04A7;

// E-Tracker data files load their song body at this offset, after the 1203-byte
// replayer that we drop in at 0x0000.
constexpr uint16_t ETRACKER_SONG_LOAD = 0x04B3;

constexpr long MIN_TICKS = PLAY_HZ * 2;
constexpr long MAX_TICKS = PLAY_HZ * 600;

constexpr long CYCLES_PER_TICK = 6000000 / PLAY_HZ; // SCPlayer's per-tick budget
} // namespace

CopMachine::CopMachine(int sampleRate)
    : ram_(IMAGE_SIZE + Z80_Cpu::cpu_padding, 0), sampleRate_(sampleRate),
      samplesPerTick_(sampleRate / PLAY_HZ)
{
    cpu_.reset(ram_.data(), ram_.data());
    // Mirror the single 32K buffer across both halves of the address space so
    // opcode fetches at 0x8000+ alias the 0x0000+ load.
    cpu_.map_mem(0x0000, IMAGE_SIZE, ram_.data());
    cpu_.map_mem(IMAGE_SIZE, IMAGE_SIZE, ram_.data());

    saa_ = CreateCSAASound();
    saa_->SetSoundParameters(SAAP_NOFILTER | SAAP_44100 | SAAP_16BIT | SAAP_STEREO);
    if (sampleRate != 44100) {
        saa_->SetSampleRate(sampleRate);
    }
}

CopMachine::~CopMachine()
{
    if (saa_ != nullptr) {
        DestroyCSAASound(saa_);
    }
}

bool CopMachine::init(const uint8_t* song, size_t songLen)
{
    if (songLen < 16 || songLen > IMAGE_SIZE) {
        return false;
    }

    // Detect an E-Tracker data file by its signature at offset 0x0A; if found,
    // drop the shared E-Tracker replayer at 0x0000 and load the file (with its
    // 10-byte header) at 0x04B3. Otherwise the file is a self-contained image.
    eTracker_ = false;
    uint16_t loadAddr = 0x0000;
    if (songLen >= 0x12 && std::memcmp(song + 0x0A, "ETracker", 8) == 0) {
        std::memcpy(ram_.data(), etracker_bin, sizeof(etracker_bin));
        eTracker_ = true;
        loadAddr = ETRACKER_SONG_LOAD;
    }

    size_t avail = IMAGE_SIZE - loadAddr;
    size_t n = std::min(songLen, avail);
    std::memcpy(ram_.data() + loadAddr, song, n);

    // SCPlayer's format patches: pin the subsong selector for the two raw
    // replayer revisions it recognises, and flag the third (a "compiled"
    // E-Tracker song) so it uses the E-Tracker calling convention.
    if (std::memcmp(ram_.data() + 0x13, "\x01\xff\x01\x3e\x1c\xed", 6) == 0) {
        ram_[0x01] = 1;
        ram_[0x02] = 0;
    } else if (std::memcmp(ram_.data(), "\x43\x72\x3d\xc2\x23\x81", 6) == 0) {
        ram_[0x01] = 1;
    } else if (std::memcmp(ram_.data(), "\x21\xb3\x84\xc3\xef\x83", 6) == 0) {
        eTracker_ = true;
    }

    ram_[IDLE] = 0x18; // JR $
    ram_[IDLE + 1] = 0xFE;

    if (eTracker_) {
        // Run the init routine, then switch to the play entry and arm the loop
        // trap (mirrors SCPlayer: CALL 0x8000 init, then CALL 0x8006 per tick).
        if (!call(0x8000)) {
            return false;
        }
        playEntry_ = 0x8006;
        ram_[LOOP_TRAP] = 0x18; // JR $
        ram_[LOOP_TRAP + 1] = 0xFE;
    } else {
        playEntry_ = 0x8000;
    }
    return true;
}

bool CopMachine::call(uint16_t entry)
{
    cpu_.r.pc = entry;
    cpu_.r.sp = 0xCF00;
    ram_[--cpu_.r.sp & IMAGE_MASK] = IDLE >> 8;
    ram_[--cpu_.r.sp & IMAGE_MASK] = IDLE & 0xFF;
    cpu_.set_time(0);
    long done = 0;
    while (done < CYCLES_PER_TICK) {
        constexpr long chunk = 50000;
        runSlice(chunk);
        cpu_.adjust_time(-chunk);
        done += chunk;
        uint16_t pc = cpu_.r.pc;
        if (pc == IDLE) {
            return true;
        }
        // E-Tracker loop point reached -> one play-through done.
        if (eTracker_ && (pc & IMAGE_MASK) == LOOP_TRAP && ticksDone_ > MIN_TICKS) {
            finished_ = true;
            return true;
        }
    }
    return false; // ran away
}

int CopMachine::generate(int16_t* out, int frames)
{
    if (finished_) {
        std::memset(out, 0, frames * 2 * sizeof(int16_t));
        return 0;
    }
    int produced = 0;
    while (produced < frames) {
        if (tickPhase_ == 0) {
            call(playEntry_);
            ticksDone_++;
            if (ticksDone_ > MAX_TICKS) {
                finished_ = true;
            }
        }
        int n = std::min(frames - produced, samplesPerTick_ - tickPhase_);
        saa_->GenerateMany(reinterpret_cast<unsigned char*>(out + produced * 2),
                           static_cast<unsigned long>(n));
        produced += n;
        tickPhase_ += n;
        if (tickPhase_ >= samplesPerTick_) {
            tickPhase_ = 0;
        }
        if (finished_) {
            return produced;
        }
    }
    return frames;
}

// --- GME Z80 core run body ---------------------------------------------------
// Same integration pattern as sccmusixx's scc_machine.cpp / bbsong's
// z80_machine.cpp: define the access macros, then #include the core's run body.
// Memory is fully mapped (mirrored 32K), so loads/stores go straight to ram_;
// only OUT/IN need handling, routing the SAM's SAA1099 ports into SAASound.
#define CPU cpu_
#define OUT_PORT(addr, data)                                                   \
    do {                                                                       \
        if (((addr) & 0xFFFF) == 0x01FF) {                                     \
            saaLatch_ = (uint8_t)(data);                                       \
            saa_->WriteAddress((uint8_t)(data));                               \
        } else {                                                               \
            saa_->WriteData((uint8_t)(data));                                  \
        }                                                                      \
        port_[(addr) & 0xFF] = (uint8_t)(data);                               \
    } while (0)
#define IN_PORT(addr) (0xFF)
#define READ_MEM(addr) (ram_[(addr) & IMAGE_MASK])
#define WRITE_MEM(addr, data) (ram_[(addr) & IMAGE_MASK] = (uint8_t)(data))
#define IDLE_ADDR IDLE
#define RST_BASE 0

#undef BYTE // SAASound.h defines BYTE as `unsigned char`; the GME run body below
            // redefines it as a cast macro. SAASound is done being used here.

#define CPU_BEGIN                                                              \
    bool CopMachine::runSlice(Z80_Cpu::time_t end)                             \
    {                                                                          \
        bool idle = false;                                                     \
        CPU.set_end_time(end);

#include "gme/Z80_Cpu_run.h"

    idle = (CPU.r.pc == IDLE);
    return idle;
}

} // namespace musix::cop
