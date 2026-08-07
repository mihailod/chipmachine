#include "ted_machine.h"

#include "ted_cpu.h"
#include "ted_sound.h"

#include <algorithm>
#include <cstring>

namespace musix::tedcr {

namespace {

// Addresses in the synthetic high page. Only the four the corpus actually lands
// on are placed where the real KERNAL puts them; the rest of $8000-$FFFF is
// filled with RTS so a stray JSR returns instead of running into garbage.
constexpr uint16_t IRQ_ENTRY = 0xFCB3;    // what the $FFFE vector points at
constexpr uint16_t IRQ_DISPATCH = 0xCE00; // IRQ-vs-BRK split
constexpr uint16_t IRQ_DEFAULT = 0xCE0E;  // where $0314 points until a tune claims it
constexpr uint16_t IRQ_EXIT = 0xFCC3;     // handlers end with JMP here
constexpr uint16_t RASTER_HOOK_DEFAULT = 0xCE42; // where $0312 points by default
constexpr uint16_t SOUND_TIMER = 0xCECD;         // BASIC SOUND's duration counters
// The two raster compare lines the KERNAL alternates between. Bit 6 of the
// compare is what selects between them, and having it set is what routes the
// interrupt to the user hook at $0312.
constexpr uint8_t RASTER_HOOK = 0xCC;
constexpr uint8_t RASTER_HOUSEKEEPING = 0xA1;
constexpr uint16_t NMI_STUB = 0xFCFA;
constexpr uint16_t SPIN = 0xFCF0;         // an RTS out of the tune lands here

// KERNAL entry points the corpus calls, both of them screen output. Handled as
// host traps rather than 6502: PRIMM has to walk the inline string and rewrite
// the return address, and doing that in 6502 would need zero-page scratch that
// is not ours to clobber.
constexpr uint16_t KERNAL_CHROUT = 0xFFD2;
constexpr uint16_t KERNAL_PRIMM = 0xFF4F;

// BASIC's RUN. A recurring HVTC rip harness sets BASIC's end-of-program pointer,
// relinks, and hands over with `JMP $8BDC` -- so the tune it actually wants to
// play is named by a SYS in the program that RUN would then execute. Trapped and
// resolved to that SYS rather than interpreting BASIC. Everything else the
// harness calls on the way ($8BBE relink, $8818, $F3B5) is bookkeeping we do not
// need, and lands on the RTS filler.
constexpr uint16_t BASIC_RUN = 0x8BDC;

// A tune whose RUN resolves back to the SYS we already entered would spin here
// forever, so the trap only fires a few times.
constexpr int MAX_BASIC_RUNS = 4;

// $FF09 / $FF0A bits.
constexpr uint8_t IRQ_RASTER = 0x02;
constexpr uint8_t IRQ_LIGHTPEN = 0x04;
constexpr uint8_t IRQ_TIMER1 = 0x08;
constexpr uint8_t IRQ_TIMER2 = 0x10;
constexpr uint8_t IRQ_TIMER3 = 0x40;
constexpr uint8_t IRQ_SOURCES = IRQ_RASTER | IRQ_LIGHTPEN | IRQ_TIMER1 | IRQ_TIMER2 | IRQ_TIMER3;

// Playback backstop. These tunes loop forever; the host's song length normally
// stops them long before this does.
constexpr int MAX_SECONDS = 3600;

// The one machine the global CPU hooks address. Playback is one song at a time,
// so an active pointer avoids threading a userdata pointer the core does not
// carry -- the same arrangement victrackerplugin uses.
TedMachine* g_active = nullptr;

} // namespace

} // namespace musix::tedcr

// Memory hooks for the 6502 core (see ted_cpu.h). Global, as the core requires.
extern "C" uint8_t ted_mem_read(uint16_t addr)
{
    return musix::tedcr::g_active->read(addr);
}
extern "C" void ted_mem_write(uint16_t addr, uint8_t val)
{
    musix::tedcr::g_active->write(addr, val);
}

namespace musix::tedcr {

static uint16_t sysAddress(const std::vector<uint8_t>& ram, uint16_t loadAddr);

TedMachine::TedMachine(int sampleRate)
    : ram_(0x10000, 0), rom_(0x8000, 0x60), sampleRate_(sampleRate)
{
    buildRom();
}

TedMachine::~TedMachine()
{
    if (g_active == this) {
        g_active = nullptr;
    }
}

// The whole replacement for roms.h: three short routines and the vectors.
// Everything here is the only way these sequences can be written -- save the
// registers, work out whether it was an IRQ or a BRK, dispatch through the RAM
// vector, restore and return.
void TedMachine::buildRom()
{
    auto put = [this](uint16_t addr, std::initializer_list<uint8_t> bytes) {
        uint16_t a = addr;
        for (uint8_t b : bytes) {
            rom_[a - 0x8000] = b;
            a++;
        }
    };

    // $FCB3  PHA / TXA / PHA / TYA / PHA / STA $FDD0 / JMP $CE00
    put(IRQ_ENTRY, {0x48, 0x8A, 0x48, 0x98, 0x48, 0x8D, 0xD0, 0xFD, 0x4C, 0x00, 0xCE});

    // $CE00  TSX / LDA $0104,X / AND #$10 / BNE +3 / JMP ($0314) / JMP ($0316)
    //
    // At this point the stack holds, from the interrupt: PCH, PCL, P, then the
    // A/X/Y pushed above -- so with X = SP the pushed status byte is at $0104,X.
    // Bit 4 of it is the B flag, which is what separates a BRK from an IRQ.
    put(IRQ_DISPATCH, {0xBA, 0xBD, 0x04, 0x01, 0x29, 0x10, 0xD0, 0x03,
                       0x6C, 0x14, 0x03, 0x6C, 0x16, 0x03});

    // Where $0314 points until a tune installs its own handler. The real KERNAL
    // scans the keyboard and blinks the cursor here, none of which we need, but
    // two things about it do matter. It acknowledges the interrupt -- without
    // that the source re-fires forever -- and, on a raster interrupt whose
    // compare line has bit 6 set, it dispatches through a SECOND vector at
    // $0312. That is the 264's user raster hook, and part of the corpus installs
    // its player there and nowhere else: `STX $0312 / STY $0313 / CLI` and spin.
    //
    // Note the acknowledge is not done on the way in: the real $CE00 does not
    // touch $FF09 either, and tunes that claim $0314 do their own.
    put(IRQ_DEFAULT, {0xAD, 0x09, 0xFF,   // LDA $FF09
                      0x8D, 0x09, 0xFF,   // STA $FF09   (write a 1 to clear)
                      0x29, 0x02,         // AND #$02    raster?
                      0xF0, 0x0D,         // BEQ  $CE25  -> exit
                      0xA9, RASTER_HOOK,  // LDA #$CC
                      0x2C, 0x0B, 0xFF,   // BIT $FF0B   bit 6 -> V
                      0x50, 0x03,         // BVC  $CE22
                      0x6C, 0x12, 0x03,   // JMP ($0312) -- the user hook
                      0x8D, 0x0B, 0xFF,   // $CE22: STA $FF0B  (next compare)
                      0x4C, 0xC3, 0xFC}); // $CE25: JMP $FCC3

    // $CE42  JSR $CFBF / JSR $CECD / LDA #$A1 / STA $FF0B / JMP $FCC3
    //
    // Where $0312 points until a tune claims it. Between this and the handler
    // above the two halves alternate the compare line: bit 6 clear selects $CC,
    // whose bit 6 is set, which selects the hook, which selects $A1 again. Two
    // raster interrupts per frame, one of which reaches the user vector.
    //
    // $CFBF is cassette and serial housekeeping and lands on the RTS filler; the
    // call is kept only so $CE45 stays the address of the JSR below, which is
    // where tunes that chain into the middle of this expect to land.
    put(RASTER_HOOK_DEFAULT, {0x20, 0xBF, 0xCF,   // JSR $CFBF
                              0x20, 0xCD, 0xCE,   // JSR $CECD
                              0xA9, RASTER_HOUSEKEEPING,
                              0x8D, 0x0B, 0xFF,   // STA $FF0B
                              0x4C, 0xC3, 0xFC}); // JMP $FCC3

    // $CE58  LDA #$A1 / STA $FF0B / JMP $FCC3 -- the tail of the above, at the
    // address the real KERNAL keeps it, for tunes that chain straight here.
    put(0xCE58, {0xA9, RASTER_HOUSEKEEPING, 0x8D, 0x0B, 0xFF, 0x4C, 0xC3, 0xFC});

    // $CECD  the SOUND duration timers.
    //
    // This is what backs the duration argument of BASIC 3.5's SOUND: a 16-bit
    // counter per voice at $04FC/$04FE (voice 1) and $04FD/$04FF (voice 2),
    // counted UP once per interrupt while non-zero, and on overflow the voice is
    // switched off by masking its enable bits out of $FF11.
    //
    // Music that never touches BASIC still depends on it, because a counter the
    // KERNAL ticks for free is a ready-made frame timer: a tune seeds $04FC and
    // waits for it to come back to zero to time its next note.
    put(SOUND_TIMER, {0xA2, 0x01,        // LDX #$01
                      0xBD, 0xFC, 0x04,  // $CECF: LDA $04FC,X
                      0x1D, 0xFE, 0x04,  // ORA $04FE,X
                      0xF0, 0x13,        // BEQ  $CEEA   (idle -> next voice)
                      0xFE, 0xFC, 0x04,  // INC $04FC,X
                      0xD0, 0x0E,        // BNE  $CEEA
                      0xFE, 0xFE, 0x04,  // INC $04FE,X
                      0xD0, 0x09,        // BNE  $CEEA
                      0xBD, 0xEE, 0xCE,  // LDA $CEEE,X  (the mask for this voice)
                      0x2D, 0x11, 0xFF,  // AND $FF11
                      0x8D, 0x11, 0xFF,  // STA $FF11    (voice off)
                      0xCA,              // $CEEA: DEX
                      0x10, 0xE2,        // BPL  $CECF
                      0x60,              // RTS
                      0xEF, 0x9F});      // $CEEE: masks -- clear bit 4, clear bits 5+6

    // $FCC3  PLA / TAY / PLA / TAX / PLA / RTI
    put(IRQ_EXIT, {0x68, 0xA8, 0x68, 0xAA, 0x68, 0x40});

    put(SPIN, {0x4C, 0xF0, 0xFC}); // JMP *
    put(NMI_STUB, {0x40});         // RTI

    auto vector = [&put](uint16_t at, uint16_t target) {
        put(at, {static_cast<uint8_t>(target & 0xFF), static_cast<uint8_t>(target >> 8)});
    };
    vector(0xFFFA, NMI_STUB);
    vector(0xFFFC, IRQ_ENTRY); // never used; we set the PC directly
    vector(0xFFFE, IRQ_ENTRY);
}

void TedMachine::reset()
{
    std::fill(ram_.begin(), ram_.end(), uint8_t{0});
    std::memset(tedRegs_, 0, sizeof(tedRegs_));
    romEnabled_ = true;
    portDdr_ = 0;
    portData_ = 0xFF;
    raster_ = 0;
    rasterCompare_ = RASTER_HOOK; // the state the KERNAL's alternation leaves it in
    masterCarry_ = 0;
    timerCarry_ = 0;
    soundCarry_ = 0;
    soundBudget_ = 0;
    linePos_ = 0;
    irqStatus_ = 0;
    // A booted machine is already taking a raster interrupt every frame -- that
    // is how the KERNAL scans the keyboard -- so the enable bit is set before a
    // tune ever runs. Plenty of the corpus depends on it: they set only the
    // compare line in $FF0B, CLI, and spin, never touching $FF0A at all. Since
    // this machine jumps straight to the SYS address it has to start in that
    // state deliberately.
    irqMask_ = IRQ_RASTER;
    ff06_ = 0x1B; // display on, 25 rows -- the state a booted machine is in
    ff07_ = 0x08;
    keyLatch_ = 0xFF;
    keyPressed_ = 0;
    for (int i = 0; i < 3; i++) {
        timer_[i] = 0xFFFF;
        timerLatch_[i] = 0xFFFF;
        timerRun_[i] = false;
    }
    lines_ = 0;
    finished_ = false;
    basicRuns_ = 0;
    stats_ = Stats{};

    // The IRQ and BRK vectors point at our default handler until a tune claims
    // them, which is what the KERNAL's own cold start does.
    ram_[0x0314] = IRQ_DEFAULT & 0xFF;
    ram_[0x0315] = IRQ_DEFAULT >> 8;
    ram_[0x0316] = IRQ_DEFAULT & 0xFF;
    ram_[0x0317] = IRQ_DEFAULT >> 8;
    ram_[0x0312] = RASTER_HOOK_DEFAULT & 0xFF;
    ram_[0x0313] = RASTER_HOOK_DEFAULT >> 8;
    installRamHelpers();

    tedsnd_init(TED_MASTER_PAL / 8, sampleRate_);
}

// The KERNAL builds a small set of routines in RAM at boot, and tunes call into
// them: helpers that read a byte from the RAM *underneath* the ROM. Each is the
// same six instructions -- bank RAM in, read through a zero-page pointer, bank
// ROM back -- differing only in which pointer they use, plus one at $0494 whose
// pointer is patched in by the caller.
//
// Nothing here is a copy of anything: the sequence is the only way to do the
// job on this machine. The addresses and the set of zero-page pointers are the
// KERNAL's layout, reproduced because that is what tunes JSR to.
void TedMachine::installRamHelpers()
{
    auto put = [this](uint16_t addr, std::initializer_list<uint8_t> bytes) {
        uint16_t a = addr;
        for (uint8_t b : bytes) {
            ram_[a++] = b;
        }
    };

    // $0494  STA $049C / SEI / STA $FF3F / LDA ($00),Y / STA $FF3E / CLI / RTS
    // The LDA operand at $049C is what the leading STA patches, so the caller
    // picks the zero-page pointer in A.
    put(0x0494, {0x8D, 0x9C, 0x04, 0x78, 0x8D, 0x3F, 0xFF, 0xB1, 0x00,
                 0x8D, 0x3E, 0xFF, 0x58, 0x60});

    // The fixed-pointer variants, eleven bytes apart.
    static const struct { uint16_t at; uint8_t zp; } fixed[] = {
        {0x04A5, 0x3B}, {0x04B0, 0x22}, {0x04BB, 0x24},
        {0x04C6, 0x6F}, {0x04D1, 0x5F}, {0x04DC, 0x64},
    };
    for (const auto& f : fixed) {
        put(f.at, {0x78,                    // SEI
                   0x8D, 0x3F, 0xFF,        // STA $FF3F   bank RAM in
                   0xB1, f.zp,              // LDA (zp),Y
                   0x8D, 0x3E, 0xFF,        // STA $FF3E   bank ROM back
                   0x58,                    // CLI
                   0x60});                  // RTS
    }
}

// Pulls the target out of the one-line BASIC stub every 264-series music file
// carries: link, line number, the SYS token ($9E), then the address in ASCII.
// Jumping straight there is what lets this machine do without a BASIC ROM --
// tedplay instead injects a call to BASIC's RUN routine, which is the only
// reason it needed one.
//
// Templated on the reader because it runs over two different memories: the file
// as loaded from disk, and live RAM when BASIC's RUN is trapped, by which point
// the program may have been rewritten.
template <typename Read> static uint16_t scanForSys(Read read, uint16_t base)
{
    for (int i = 4; i < 40; i++) {
        uint16_t p = static_cast<uint16_t>(base + i);
        if (read(p) != 0x9E) {
            continue;
        }
        p++;
        while (read(p) == ' ') {
            p++;
        }
        int value = 0;
        bool any = false;
        while (read(p) >= '0' && read(p) <= '9') {
            value = value * 10 + (read(p) - '0');
            p++;
            any = true;
        }
        return (any && value > 0 && value < 0x10000) ? static_cast<uint16_t>(value) : 0;
    }
    return 0;
}

static uint16_t sysAddress(const std::vector<uint8_t>& ram, uint16_t loadAddr)
{
    return scanForSys([&ram](uint16_t a) { return ram[a]; }, loadAddr);
}

uint16_t basicSysTarget(const uint8_t* prg, size_t len)
{
    if (prg == nullptr || len < 8) {
        return 0;
    }
    uint16_t load = static_cast<uint16_t>(prg[0] | (prg[1] << 8));
    return scanForSys(
        [prg, len, load](uint16_t a) -> uint8_t {
            size_t off = static_cast<size_t>(2 + (a - load));
            return off < len ? prg[off] : uint8_t{0};
        },
        load);
}

bool TedMachine::load(const uint8_t* data, size_t len)
{
    if (data == nullptr || len < 8) {
        return false;
    }
    uint16_t loadAddr = static_cast<uint16_t>(data[0] | (data[1] << 8));
    // 264-series programs load at the BASIC start. $0801 would be a C64 file.
    if (loadAddr < 0x0400 || loadAddr >= 0xFC00) {
        return false;
    }

    g_active = this;
    reset();

    size_t n = len - 2;
    if (loadAddr + n > 0x10000) {
        n = 0x10000 - loadAddr;
    }
    std::memcpy(ram_.data() + loadAddr, data + 2, n);

    // No SYS means there is no machine code to enter: the file is a BASIC
    // program, and its music -- if any -- is BASIC 3.5 SOUND statements that
    // only an interpreter can play. Decline rather than run the program text as
    // if it were 6502, which produces noise rather than silence and would be
    // worse than not claiming the file at all.
    uint16_t entry = sysAddress(ram_, loadAddr);
    if (entry == 0) {
        return false;
    }
    lastEntry_ = entry;

    tedcpu_reset();
    // A tune reached by BASIC's SYS finds the stack as deep as BASIC left it,
    // and some of the corpus reads that depth with TSX and uses it -- typically
    // to size a copy that must stop short of the live stack. Measured against a
    // real BASIC hand-off, the tune sees $F6, which is this minus the return
    // address pushed below.
    tedcpu_set_sp(0xF8);
    tedcpu_push16(SPIN - 1); // an RTS out of the tune parks in the spin loop
    tedcpu_set_pc(entry);

    long long linesPerSecond = TED_MASTER_PAL / LINE_MASTER;
    maxLines_ = linesPerSecond * MAX_SECONDS;
    return true;
}

void TedMachine::pressKey(int key)
{
    keyPressed_ = key;
}

void TedMachine::setIrq(uint8_t bit)
{
    irqStatus_ |= bit;
}

bool TedMachine::irqPending() const
{
    return (irqStatus_ & irqMask_ & IRQ_SOURCES) != 0;
}

void TedMachine::tickTimers(int ticks)
{
    static const uint8_t bits[3] = {IRQ_TIMER1, IRQ_TIMER2, IRQ_TIMER3};
    for (int i = 0; i < 3; i++) {
        if (!timerRun_[i]) {
            continue;
        }
        int c = timer_[i] - ticks;
        while (c < 0) {
            setIrq(bits[i]);
            stats_.timerIrqs++;
            // Timer 1 reloads from its latch; timers 2 and 3 free-run, wrapping
            // through $FFFF.
            c += (i == 0 && timerLatch_[0] > 0) ? timerLatch_[0] : 0x10000;
        }
        timer_[i] = c;
    }
}

void TedMachine::tick(int master)
{
    linePos_ += master;
    while (linePos_ >= LINE_MASTER) {
        linePos_ -= LINE_MASTER;
        raster_++;
        if (raster_ >= linesPerFrame_) {
            raster_ = 0;
        }
        if (raster_ == rasterCompare_) {
            setIrq(IRQ_RASTER);
            stats_.rasterIrqs++;
        }
        if (maxLines_ > 0 && ++lines_ > maxLines_) {
            finished_ = true;
        }
    }

    timerCarry_ += master;
    int t = timerCarry_ >> 1;
    timerCarry_ &= 1;
    if (t > 0) {
        tickTimers(t);
    }

    soundCarry_ += master;
    soundBudget_ += soundCarry_ >> 3;
    soundCarry_ &= 7;
}

bool TedMachine::handleTrap()
{
    if (!romEnabled_) {
        return false;
    }
    uint16_t pc = tedcpu_get_pc();
    if (pc == BASIC_RUN) {
        uint16_t entry = basicRuns_ < MAX_BASIC_RUNS ? sysAddress(ram_, 0x1001) : 0;
        basicRuns_++;
        if (entry != 0) {
            lastEntry_ = entry;
            tedcpu_set_pc(entry);
            return true;
        }
        // Nothing to hand over to: fall through to the RTS filler.
        return false;
    }
    if (pc == KERNAL_CHROUT) {
        // Swallow the character and return.
        tedcpu_set_pc(static_cast<uint16_t>(tedcpu_pull16() + 1));
        return true;
    }
    if (pc == KERNAL_PRIMM) {
        // The string follows the JSR inline; execution resumes past its
        // terminator, so skip it rather than returning to the caller's address.
        uint16_t ret = tedcpu_pull16();
        uint16_t p = static_cast<uint16_t>(ret + 1);
        for (int i = 0; i < 256 && read(p) != 0; i++) {
            p++;
        }
        tedcpu_set_pc(static_cast<uint16_t>(p + 1));
        return true;
    }
    return false;
}

// One raster line. The display steals five cycles for DRAM refresh and halves
// the CPU over the display window, which together are what make a frame
// 22,890 CPU cycles with the screen on and 34,008 with it off -- both measured.
void TedMachine::runLine()
{
    tick(LINE_REFRESH); // refresh: the clock advances, the CPU does not

    int masterPerCpu = (displayOn() && raster_ < DISPLAY_LINES) ? 2 : 1;
    masterCarry_ += LINE_MASTER - LINE_REFRESH;

    while (masterCarry_ >= masterPerCpu && !finished_) {
        int cycles;
        if (irqPending() && (cycles = tedcpu_irq()) > 0) {
            stats_.irqs++;
        } else if (handleTrap()) {
            cycles = 6;
        } else {
            cycles = tedcpu_step();
            stats_.instructions++;
        }
        if (cycles <= 0) {
            cycles = 2; // a core that reports nothing must still advance time
        }
        int master = cycles * masterPerCpu;
        masterCarry_ -= master;
        tick(master);
    }
}

int TedMachine::generate(int16_t* out, int frames)
{
    if (g_active != this) {
        g_active = this;
    }
    int produced = 0;
    int idle = 0;
    while (produced < frames) {
        int dt = soundBudget_;
        int n = tedsnd_render(out + produced * 2, frames - produced, 2, &dt);
        soundBudget_ = dt;
        produced += n;
        if (produced >= frames || finished_) {
            break;
        }
        if (n > 0) {
            idle = 0;
        } else if (++idle > 100000) {
            finished_ = true;
            break;
        }
        runLine();
    }
    return produced;
}

uint8_t TedMachine::read(uint16_t addr)
{
    if (addr < 2) {
        // 7501 on-chip port. Bits driven as inputs read back high.
        return addr == 0 ? portDdr_
                         : static_cast<uint8_t>((portData_ & portDdr_) | (0xC0 & ~portDdr_));
    }
    if (addr < 0x8000) {
        return ram_[addr];
    }
    // The I/O window stays visible whichever way $8000-$FFFF is banked.
    if (addr >= 0xFF00 && addr <= 0xFF3F) {
        return readTed(addr);
    }
    if (addr >= 0xFD00 && addr <= 0xFEFF) {
        return 0xFF; // 6529B, ACIA, TCBM -- nothing we emulate reads back
    }
    return romEnabled_ ? rom_[addr - 0x8000] : ram_[addr];
}

void TedMachine::write(uint16_t addr, uint8_t value)
{
    if (addr < 2) {
        (addr == 0 ? portDdr_ : portData_) = value;
        return;
    }
    if (addr >= 0xFF00 && addr <= 0xFF3F) {
        writeTed(addr, value);
        return;
    }
    if (addr >= 0xFD00 && addr <= 0xFEFF) {
        if (addr == 0xFD30) {
            keyLatch_ = value; // keyboard row select
        }
        return; // SID card at $FD40-$FD5F is deliberately absent -- see README
    }
    // ROM is read-only, so writes above $8000 land in the RAM underneath it.
    // That is exactly how a tune installs its own $FFFE vector while still
    // running with ROM banked in.
    ram_[addr] = value;
}

uint8_t TedMachine::readTed(uint16_t addr)
{
    switch (addr) {
    case 0xFF00: return static_cast<uint8_t>(timer_[0] & 0xFF);
    case 0xFF01: return static_cast<uint8_t>((timer_[0] >> 8) & 0xFF);
    case 0xFF02: return static_cast<uint8_t>(timer_[1] & 0xFF);
    case 0xFF03: return static_cast<uint8_t>((timer_[1] >> 8) & 0xFF);
    case 0xFF04: return static_cast<uint8_t>(timer_[2] & 0xFF);
    case 0xFF05: return static_cast<uint8_t>((timer_[2] >> 8) & 0xFF);
    case 0xFF06: return ff06_;
    case 0xFF07: return ff07_;
    case 0xFF08: {
        // Keyboard columns for the rows selected via $FD30. Nothing is held
        // down unless the host asked for a key; see pressKey().
        if (keyPressed_ == 0) {
            return 0xFF;
        }
        return static_cast<uint8_t>(~(1u << (keyPressed_ & 7)) & 0xFF);
    }
    case 0xFF09:
        return static_cast<uint8_t>(irqStatus_ | (irqPending() ? 0x80 : 0x00) | 0x25);
    case 0xFF0A:
        return static_cast<uint8_t>(irqMask_ | ((rasterCompare_ >> 8) & 1) | 0xA0);
    case 0xFF0B: return static_cast<uint8_t>(rasterCompare_ & 0xFF);
    case 0xFF1C: return static_cast<uint8_t>(((raster_ >> 8) & 0x01) | 0xFE);
    case 0xFF1D: return static_cast<uint8_t>(raster_ & 0xFF);
    case 0xFF1E:
        // Horizontal position, counted in the same units the chip reports.
        return static_cast<uint8_t>((linePos_ * 4) & 0xFC);
    case 0xFF1F:
        return static_cast<uint8_t>(0x08 | (raster_ & 0x07));
    case 0xFF3E:
    case 0xFF3F:
        return 0xFF;
    default:
        break;
    }
    if (addr >= 0xFF15 && addr <= 0xFF19) {
        return static_cast<uint8_t>(tedRegs_[addr & 0x3F] | 0x80); // top bit unused
    }
    return tedRegs_[addr & 0x3F];
}

void TedMachine::writeTed(uint16_t addr, uint8_t value)
{
    tedRegs_[addr & 0x3F] = value;
    switch (addr) {
    // Writing a timer's low byte stops it and sets the latch; writing the high
    // byte sets the rest of the latch and starts it running again.
    case 0xFF00:
    case 0xFF02:
    case 0xFF04: {
        int i = (addr - 0xFF00) / 2;
        timerLatch_[i] = (timerLatch_[i] & 0xFF00) | value;
        timer_[i] = (timer_[i] & 0xFF00) | value;
        timerRun_[i] = false;
        break;
    }
    case 0xFF01:
    case 0xFF03:
    case 0xFF05: {
        int i = (addr - 0xFF01) / 2;
        timerLatch_[i] = (timerLatch_[i] & 0x00FF) | (value << 8);
        timer_[i] = (timer_[i] & 0x00FF) | (value << 8);
        timerRun_[i] = true;
        break;
    }
    case 0xFF06: ff06_ = value; break;
    case 0xFF07: ff07_ = value; break;
    case 0xFF09:
        irqStatus_ &= static_cast<uint8_t>(~(value & 0x7E)); // write a 1 to clear
        break;
    case 0xFF0A:
        irqMask_ = static_cast<uint8_t>(value & IRQ_SOURCES);
        rasterCompare_ = (rasterCompare_ & 0x0FF) | ((value & 1) << 8);
        break;
    case 0xFF0B:
        rasterCompare_ = (rasterCompare_ & 0x100) | value;
        break;
    case 0xFF0E:
    case 0xFF0F:
    case 0xFF10:
    case 0xFF11:
    case 0xFF12:
        tedsnd_store(addr & 0x1F, value);
        stats_.soundWrites++;
        break;
    case 0xFF3E: romEnabled_ = true; break;
    case 0xFF3F: romEnabled_ = false; break;
    default: break;
    }
}

} // namespace musix::tedcr
