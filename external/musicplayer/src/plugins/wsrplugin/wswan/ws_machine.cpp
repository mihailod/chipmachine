#include "ws_machine.hpp"

#include <algorithm>
#include <cstring>

namespace wswan {

namespace {

constexpr uint32_t MasterClock = 3072000;   // V30MZ / sound clock
constexpr uint32_t CyclesPerLine = 256;     // "counts down every horizontal blank (256 CPU cycles)"
constexpr uint16_t LinesPerFrame = 159;     // 144 visible + blanking; 3072000/(256*159) = 75.5 Hz
constexpr uint16_t VBlankLine = 144;        // "Display Current Line == 144"

// Interrupt bit indices, from the WSdev "Interrupts" table. Only the four this
// machine can generate are listed.
constexpr uint8_t IntLineMatch = 4;
constexpr uint8_t IntVBlankTimer = 5;
constexpr uint8_t IntVBlank = 6;
constexpr uint8_t IntHBlankTimer = 7;

// The stack pointer the format does NOT specify. A WSR entry point is a JMP into
// the rip's own init code, which is expected to set up its own environment, but
// several rips assume a usable stack from the first PUSH. 0x2000 is well inside
// free RAM (0x0400-0x2000 is free on every model) and is what in_wsr used, so
// rips that were only ever tested against it still work.
constexpr uint16_t DefaultSP = 0x2000;

// The APU's own scale was calibrated against libvgm, and libvgm then applies its
// per-chip `_CHIP_VOLUME` -- which for DEVID_WSWAN is 0x200, i.e. exactly 2.0 in
// its 8.8 fixed point. Nothing applies that here, so this path has to. Without
// it every .wsr renders at a measured 0.50 of what in_wsr produced.
constexpr int32_t OutputGain = 2;

bool hasFooter(const uint8_t* data, size_t size)
{
    if(size <= Machine::FooterSize) return false;
    const uint8_t* f = data + (size - Machine::FooterSize);
    return f[0] == 'W' && f[1] == 'S' && f[2] == 'R' && f[3] == 'F';
}

// Sound DMA ($4A-$52), Hyper Voice ($64-$6B) and the sound registers proper
// ($80-$9E) -- and nothing above them.
bool isApuPort(uint8_t p)
{
    return (p >= 0x4A && p <= 0x52) || (p >= 0x64 && p <= 0x6B)
        || (p >= 0x80 && p <= 0x9F);
}

int16_t clamp16(int32_t value)
{
    if(value > 32767) return 32767;
    if(value < -32768) return -32768;
    return (int16_t)value;
}

}

Machine::Machine()
{
    wsapu_init(&apu_, MasterClock, 44100);
}

bool Machine::load(const uint8_t* data, size_t size, uint32_t rate)
{
    if(!hasFooter(data, size)) return false;

    rom_.assign(data, data + size);

    firstSong_ = data[size - Machine::FooterSize + Machine::FooterFirstSong];

    if(rate == 0) rate = 44100;
    cyclesPerSmpl_ = (uint32_t)(((uint64_t)MasterClock << 16) / rate);
    wsapu_init(&apu_, MasterClock, rate);
    wsapu_set_ram(&apu_, iram_, IRamSize - 1);
    wsapu_set_dma_reader(&apu_, &Machine::dmaRead, this);
    return true;
}

void Machine::reset(unsigned song)
{
    std::memset(iram_, 0x00, sizeof(iram_));
    std::memset(sram_, 0x00, sizeof(sram_));
    std::memset(io_, 0x00, sizeof(io_));

    // Every bank register except the RAM one powers up all-ones, which is what
    // puts the top of the cartridge under the reset vector.
    bankLinear_ = 0xFF;
    bankRam_ = 0x00;
    bankRom0_ = 0xFF;
    bankRom1_ = 0xFF;   // "the mono WonderSwan expects $C3 to power up holding $FF"

    hReload_ = vReload_ = hCount_ = vCount_ = 0;
    timerCtrl_ = 0;
    line_ = 0;
    intBase_ = intEnable_ = intStatus_ = 0;

    clock_ = 0;
    hblankAcc_ = 0;
    sampleAcc_ = 0;

    wsapu_reset(&apu_);

    power();            // ares' V30MZ reset: CS=FFFF, IP=0000
    SP = DefaultSP;
    AW = (uint16_t)song;
}

// --- bus --------------------------------------------------------------------

// Where 64 KB bank `bank` starts inside the image.
//
// THE THING THAT MATTERS: a cartridge is aligned to the TOP of the bank space,
// so its LAST bank is number $FF, and bank numbers count DOWNWARDS from there.
// That is what puts the WSRF footer -- the last 32 bytes of the file -- under
// the reset vector at FFFF:0000, and it is why every bank register except $C1
// powers up holding $FF.
//
// Masking the address with `size - 1` instead only works when the image is a
// power of two. A 192 KB rip (Final Fantasy is one) then boots into the middle
// of its own data and executes garbage: the CPU runs, the PC wanders, and the
// file renders pure silence.
uint32_t Machine::bankStart(uint8_t bank) const
{
    int64_t start = (int64_t)((int32_t)bank - 0x100) * 0x10000 + (int64_t)rom_.size();
    int64_t size = (int64_t)rom_.size();
    start %= size;
    if(start < 0) start += size;
    return (uint32_t)start;
}

uint32_t Machine::romOffset(uint32_t address) const
{
    // Standard mapper layout: $20000-$2FFFF is ROM0's bank, $30000-$3FFFF is
    // ROM1's, and $40000-$FFFFF reads through the 1 MB linear window, whose
    // register selects the 1 MB (= sixteen 64 KB banks) on show.
    uint8_t bank;
    if(address < 0x30000)      bank = bankRom0_;
    else if(address < 0x40000) bank = bankRom1_;
    else                       bank = (uint8_t)((bankLinear_ << 4) | ((address >> 16) & 0x0F));

    return (bankStart(bank) + (address & 0xFFFF)) % (uint32_t)rom_.size();
}

auto Machine::read(n20 address) -> n8
{
    uint32_t a = address;
    if(a < 0x10000) return iram_[a];
    if(a < 0x20000) return sram_[((uint32_t)bankRam_ << 16 | (a & 0xFFFF)) % SRamSize];
    return rom_.empty() ? 0xFF : rom_[romOffset(a)];
}

auto Machine::write(n20 address, n8 data) -> void
{
    uint32_t a = address;
    if(a < 0x10000) { iram_[a] = data; return; }
    if(a < 0x20000) { sram_[((uint32_t)bankRam_ << 16 | (a & 0xFFFF)) % SRamSize] = data; return; }
    // Cartridge ROM is read-only.
}

uint8_t Machine::dmaRead(void* ctx, uint32_t address)
{
    return ((Machine*)ctx)->read(address & 0xFFFFF);
}

auto Machine::width(n20 address) -> u32
{
    // Internal RAM is a 16-bit bus, the cartridge an 8-bit one (16-bit is
    // configurable on the Color; nothing audible depends on it).
    return address < 0x10000 ? 2 : 1;
}

auto Machine::speed(n20 address) -> n32
{
    return 1;
}

auto Machine::ioWidth(n16 port) -> u32
{
    return 1;
}

auto Machine::ioSpeed(n16 port) -> n32
{
    return 1;
}

// --- ports ------------------------------------------------------------------

auto Machine::in(n16 port) -> n8
{
    uint8_t p = (uint8_t)port;

    // Sound, sound DMA and Hyper Voice live in the APU's own register file.
    // NOTE the upper bound: the sound registers END at $9E. Letting this run to
    // $FF swallows the timers ($A2-$AB), the interrupt controller ($B0-$B7) and
    // the bank registers ($C0-$C3) -- every rip then boots, writes its setup and
    // plays nothing, because the interrupt it enabled never arrives.
    if(isApuPort(p)) return wsapu_read_port(&apu_, p);

    switch(p) {
    case 0x02: return (uint8_t)line_;
    case 0xA4: return (uint8_t)(hReload_ & 0xFF);
    case 0xA5: return (uint8_t)(hReload_ >> 8);
    case 0xA6: return (uint8_t)(vReload_ & 0xFF);
    case 0xA7: return (uint8_t)(vReload_ >> 8);
    case 0xA8: return (uint8_t)(hCount_ & 0xFF);
    case 0xA9: return (uint8_t)(hCount_ >> 8);
    case 0xAA: return (uint8_t)(vCount_ & 0xFF);
    case 0xAB: return (uint8_t)(vCount_ >> 8);
    case 0xB0: {
        // "Bits 7..3 equal the vector offset; bits 2..0 equal the highest set
        // bit index of Interrupt Status."
        uint8_t pending = (uint8_t)(intStatus_ & intEnable_);
        uint8_t index = 0;
        for(int b = 7; b >= 0; b--) if(pending & (1 << b)) { index = (uint8_t)b; break; }
        return (uint8_t)((intBase_ & 0xF8) | index);
    }
    case 0xB2: return intEnable_;
    case 0xB4: return intStatus_;
    case 0xC0: return bankLinear_;
    case 0xC1: return bankRam_;
    case 0xC2: return bankRom0_;
    case 0xC3: return bankRom1_;
    default:   return io_[p];
    }
}

auto Machine::out(n16 port, n8 data) -> void
{
    uint8_t p = (uint8_t)port;
    uint8_t v = (uint8_t)data;

    io_[p] = v;

    if(isApuPort(p)) {
        wsapu_write_port(&apu_, p, v);
        return;
    }

    switch(p) {
    case 0x48:  // GDMA control
        if(v & 0x80) generalDma();
        break;

    case 0xA2: timerCtrl_ = v; break;
    // "The Timer Counter port is immediately initialized with this value upon
    // writing" -- which is why each half-write reloads the counter too.
    case 0xA4: hReload_ = (uint16_t)((hReload_ & 0xFF00) | v); hCount_ = hReload_; break;
    case 0xA5: hReload_ = (uint16_t)((hReload_ & 0x00FF) | (v << 8)); hCount_ = hReload_; break;
    case 0xA6: vReload_ = (uint16_t)((vReload_ & 0xFF00) | v); vCount_ = vReload_; break;
    case 0xA7: vReload_ = (uint16_t)((vReload_ & 0x00FF) | (v << 8)); vCount_ = vReload_; break;

    case 0xB0: intBase_ = v; break;
    case 0xB2: intEnable_ = v; break;
    case 0xB6: intStatus_ = (uint8_t)(intStatus_ & ~v); break;  // write 1 to clear

    case 0xC0: bankLinear_ = v; break;
    case 0xC1: bankRam_ = v; break;
    case 0xC2: bankRom0_ = v; break;
    case 0xC3: bankRom1_ = v; break;
    default: break;
    }
}

// "General DMA takes (5 + 2 * words) cycles"; the CPU is stalled for the whole
// transfer and the enable bit clears when it completes. Sound rips use it to
// push wavetables and sample blocks into RAM.
void Machine::generalDma()
{
    uint32_t source = (uint32_t)(io_[0x40] | (io_[0x41] << 8) | (io_[0x42] << 16)) & 0xFFFFE;
    uint16_t dest = (uint16_t)((io_[0x44] | (io_[0x45] << 8)) & 0xFFFE);
    uint32_t length = (uint32_t)((io_[0x46] | (io_[0x47] << 8)) & 0xFFFE);
    bool decrement = (io_[0x48] & 0x40) != 0;

    if(length == 0) { io_[0x48] &= (uint8_t)~0x80; return; }

    step(5);
    while(length) {
        iram_[dest & (IRamSize - 1)] = read(source);
        iram_[(dest + 1) & (IRamSize - 1)] = read(source + 1);
        source = decrement ? (source - 2) & 0xFFFFF : (source + 2) & 0xFFFFF;
        dest = (uint16_t)(decrement ? dest - 2 : dest + 2);
        length -= 2;
        step(2);
    }
    io_[0x48] &= (uint8_t)~0x80;
    io_[0x46] = io_[0x47] = 0;
}

// --- timing -----------------------------------------------------------------

void Machine::raise(uint8_t bit)
{
    // "Contrary to other systems, [$B2] also controls whether interrupts set
    // the relevant bit in Interrupt Status."
    if(intEnable_ & (1 << bit)) intStatus_ |= (uint8_t)(1 << bit);
}

auto Machine::step(u32 clocks) -> void
{
    clock_ += clocks;
    hblankAcc_ += clocks;
    while(hblankAcc_ >= CyclesPerLine) {
        hblankAcc_ -= CyclesPerLine;
        hblank();
    }
}

// One countdown step, shared by both timers. Returns true when the interrupt
// should fire.
//
// `armed` is a COMPATIBILITY relaxation, not hardware: some rips enable the
// timer's interrupt in $B2 and write a reload to $A4/$A6 but never write the
// countdown-enable bit in $A2 at all, and then sit in a spin loop waiting for a
// tick that a strict reading would never deliver ("With You - Mitsumete Itai"
// renders pure silence). in_wsr ignores $A2 entirely, which is why such rips
// exist and were shipped, so the countdown also runs when the interrupt is
// enabled and a non-zero reload has been programmed. A rip that does set $A2 is
// unaffected, and a rip that sets neither still stays quiet.
//
// The documented quirk is here too: "the timer interrupt will be triggered if
// the reload value is set to one even if the timer countdown is disabled, so
// long as the interrupt is enabled".
static bool tick(uint16_t& count, uint16_t reload, bool enabled, bool repeat, bool armed)
{
    if(!enabled) {
        if(reload == 1) return true;
        if(!armed) return false;
        if(count == 0) count = reload;
    }
    if(count == 0) return false;
    if(--count != 0) return false;
    if(repeat || !enabled) count = reload;
    return true;
}

void Machine::hblank()
{
    // The horizontal timer counts down every line. "The interrupt is triggered
    // when the counter would be about to count down to zero, that is when the
    // counter's value is 1"; auto-reload then reloads it, one-shot leaves the
    // counter at zero and it simply stops.
    //
    // The ENABLE BIT MUST NOT BE CLEARED when a one-shot expires. Nothing in the
    // hardware documentation says it is, and clearing it breaks the most common
    // idiom there is: a driver that arms a one-shot HBlank timer and re-arms it
    // from its own interrupt handler by rewriting the reload port. Do that and
    // the timer fires exactly once, the driver's tick never comes again, and the
    // rip renders silence while still looking alive (Glocal Hexcite).
    if(tick(hCount_, hReload_, (timerCtrl_ & 0x01) != 0, (timerCtrl_ & 0x02) != 0,
            (intEnable_ & (1 << IntHBlankTimer)) != 0 && hReload_ != 0))
        raise(IntHBlankTimer);

    line_++;
    if(line_ >= LinesPerFrame) line_ = 0;

    if(line_ == VBlankLine) {
        raise(IntVBlank);
        if(tick(vCount_, vReload_, (timerCtrl_ & 0x04) != 0, (timerCtrl_ & 0x08) != 0,
                (intEnable_ & (1 << IntVBlankTimer)) != 0 && vReload_ != 0))
            raise(IntVBlankTimer);
    }
    if(line_ == io_[0x03]) raise(IntLineMatch);
}

void Machine::pollInterrupts()
{
    uint8_t pending = (uint8_t)(intStatus_ & intEnable_);
    if(!pending) return;

    // Highest set bit wins, matching what $B0 reports back.
    uint8_t index = 0;
    for(int b = 7; b >= 0; b--) if(pending & (1 << b)) { index = (uint8_t)b; break; }

    // interrupt() returns false while the CPU has interrupts masked, and the
    // status bit simply stays pending -- which is the hardware behaviour and
    // also what lets a driver with a long critical section catch up later.
    if(interrupt((uint8_t)((intBase_ & 0xF8) | index))) {
        // Edge interrupts are requested once. A driver is expected to
        // acknowledge through $B6, but not every rip does, and a level-style
        // re-request of a timer that has already been serviced would trap the
        // CPU in its handler forever, so the accepted bit is cleared here.
        intStatus_ &= (uint8_t)~(1 << index);
    }
}

void Machine::render(int16_t* out, unsigned frames)
{
    for(unsigned i = 0; i < frames; i++) {
        sampleAcc_ += cyclesPerSmpl_;
        uint64_t target = clock_ + (sampleAcc_ >> 16);
        sampleAcc_ &= 0xFFFF;

        while(clock_ < target) {
            if(state.halt) {
                // HLT parks the CPU until an interrupt arrives; nothing here
                // consumes cycles, so time has to be moved on by hand or the
                // loop never ends. Advance to the next line boundary, which is
                // the only thing that can produce an interrupt.
                uint32_t toLine = CyclesPerLine - hblankAcc_;
                uint64_t remaining = target - clock_;
                step((u32)std::min<uint64_t>(toLine, remaining));
            } else {
                instruction();
            }
            pollInterrupts();
        }

        int32_t l = 0, r = 0;
        wsapu_render(&apu_, &l, &r, 1);
        out[i * 2 + 0] = clamp16(l * OutputGain);
        out[i * 2 + 1] = clamp16(r * OutputGain);
    }
}

}
