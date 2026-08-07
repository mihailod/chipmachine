#pragma once

// Commodore 264-series machine, cut down to what TED music needs: 64K of RAM,
// the TED register page, the three interval timers, the raster counter and its
// interrupt, ROM/RAM banking, and a synthetic high page carrying the handful of
// KERNAL entry points the corpus actually calls. No Commodore ROM images.
//
// See README.md for how the timing constants were measured and for the survey
// of what the HVTC corpus needs from the KERNAL.

#include <cstddef>
#include <cstdint>
#include <vector>

namespace musix::tedcr {

// PAL 264-series timing. The master clock is the TED's own; the CPU runs at that
// same rate, one cycle per master cycle, and what varies is how many of a line's
// cycles the TED leaves it. The sound counters run at an eighth regardless.
// Every one of these was measured against the engine being replaced -- see
// README.md, "Timing".
constexpr int TED_MASTER_PAL = 1773447;
constexpr int TED_MASTER_NTSC = 1789773;
constexpr int LINE_MASTER = 114;      // master cycles in one raster line
// CPU cycles the TED leaves for the processor on each kind of line. Measured
// per line off the engine being replaced; see the note above TedMachine::runLine
// for why the per-line split matters more than the frame total.
constexpr int LINE_CPU_BLANK = 109;   // outside the display window
constexpr int LINE_CPU_DISPLAY = 65;  // inside it, ordinary line
constexpr int LINE_CPU_BADLINE = 22;  // inside it, character/attribute fetch
constexpr int LINES_PAL = 312;
constexpr int LINES_NTSC = 262;
constexpr int DISPLAY_LINES = 204;    // lines over which the video fetch runs
// Below this a real machine's RAM has been written by the KERNAL and BASIC
// before any tune runs; above it, it is still raw DRAM. See TedMachine::reset.
constexpr int SYSTEM_AREA_END = 0x1000;

// Parses the target of the SYS in a 264-series .prg's one-line BASIC stub.
// Returns 0 when there is no SYS at all, which means the file is a BASIC
// program rather than a machine-code tune -- the plugin's content gate and
// TedMachine::load() both turn that into a decline.
uint16_t basicSysTarget(const uint8_t* prg, size_t len);

class TedMachine
{
public:
    explicit TedMachine(int sampleRate);
    ~TedMachine();

    // Loads a bare .prg. Returns false if it does not look like a 264-series
    // program. Jumps straight to the address in the BASIC SYS stub, which is why
    // no BASIC ROM is needed.
    bool load(const uint8_t* data, size_t len);

    // Renders interleaved stereo. Returns frames produced; a short return means
    // the backstop fired.
    int generate(int16_t* out, int frames);

    // Feeds a key to the keyboard latch, which is how the 264 corpus selects
    // between subtunes. 0 means "nothing pressed".
    void pressKey(int key);

    bool finished() const { return finished_; }

    // Counters, kept because a tune that never writes a sound register is a
    // tune this machine failed to start, which is worth being able to see from
    // the plugin as well as from the differential harness.
    struct Stats
    {
        long long instructions = 0;
        long long irqs = 0;
        long long soundWrites = 0;
        long long rasterIrqs = 0;
        long long timerIrqs = 0;
    };
    const Stats& stats() const { return stats_; }
    int rasterCompare() const { return rasterCompare_; }
    uint8_t irqMask() const { return irqMask_; }

    // Memory access, public because the CPU hooks route through them.
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t value);

private:
    void reset();
    void buildRom();
    void installRamHelpers();
    void runLine();
    void tick(int master);
    void tickTimers(int ticks);
    void setIrq(uint8_t bit);
    bool irqPending() const;
    bool handleTrap();
    uint8_t readTed(uint16_t addr);
    void writeTed(uint16_t addr, uint8_t value);
    bool displayOn() const { return (ff06_ & 0x10) != 0; }

    std::vector<uint8_t> ram_;   // 64K
    std::vector<uint8_t> rom_;   // $8000-$FFFF, synthetic
    bool romEnabled_ = true;

    // 7501 on-chip port.
    uint8_t portDdr_ = 0;
    uint8_t portData_ = 0xFF;

    // TED state.
    int raster_ = 0;
    int rasterCompare_ = 0;
    int linePos_ = 0;      // master cycles into the current raster line
    int cpuCarry_ = 0;
    int timerCarry_ = 0;
    int soundCarry_ = 0;
    int soundBudget_ = 0;
    uint8_t irqStatus_ = 0;
    uint8_t irqMask_ = 0;
    uint8_t ff06_ = 0x1B;
    uint8_t ff07_ = 0x08;
    uint8_t tedRegs_[0x40] = {};
    uint8_t keyLatch_ = 0xFF;
    int keyPressed_ = 0;

    int timer_[3] = {0xFFFF, 0xFFFF, 0xFFFF};
    int timerLatch_[3] = {0xFFFF, 0xFFFF, 0xFFFF};
    bool timerRun_[3] = {false, false, false};

    int sampleRate_;
    int linesPerFrame_ = LINES_PAL;
    long long lines_ = 0;
    uint16_t lastEntry_ = 0;
    int basicRuns_ = 0;
    long long maxLines_ = 0;
    bool finished_ = false;
    Stats stats_;
};

} // namespace musix::tedcr
