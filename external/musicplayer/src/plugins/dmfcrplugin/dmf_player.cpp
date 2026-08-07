// Clean-room DefleMask sequencer (Genesis + SMS) -- see dmf_player.h.

#include "dmf_player.h"

#include "ymfm.h"
#include "ymfm_opn.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

// Development trace, off unless DMFCR_DEBUG is set in the environment. Costs a
// predictable branch when disabled and keeps the harnesses from needing a
// separate build of the player.
namespace {
bool dmfcrDebug()
{
    static const bool on = (getenv("DMFCR_DEBUG") != nullptr);
    return on;
}
} // namespace
#define DMFCR_TRACE(...)                                                       \
    do {                                                                       \
        if (dmfcrDebug()) { fprintf(stderr, __VA_ARGS__); }                    \
    } while (0)

namespace dmfcr {

namespace {

// Mega Drive clocks. The YM2612 runs at the 68000 clock (master/7) and the PSG
// at master/15; both derive from the same 53.693175 MHz NTSC master crystal.
constexpr double kYmClockNtsc = 7670453.0;
constexpr double kPsgClockNtsc = 3579545.0;
constexpr double kYmClockPal = 7600489.0;
constexpr double kPsgClockPal = 3546895.0;

constexpr int kFmChannels = 6;

// Diagnostic channel mute: bit i silences channel i, e.g. DMFCR_MUTE=0x3C0 to
// hear the FM alone. Development only, and worth keeping -- it is how the
// "broadband excess" on one file was pinned on the PSG rather than the FM.
const unsigned kMuteMask = [] {
    const char* e = getenv("DMFCR_MUTE");
    return e != nullptr ? (unsigned)strtoul(e, nullptr, 0) : 0u;
}();

// Operator record N in the file -> YM2612 register offset.
//
// STRAIGHT THROUGH, and that is the point. The YM2612's four operator slots sit
// at +0/+4/+8/+12 and are conventionally labelled "operator 1, 3, 2, 4" -- the
// famous interleave -- so the obvious mapping for a file that numbers its
// operators 1..4 is { 0, 8, 4, 12 }. That is wrong here: DefleMask writes the
// records in REGISTER order, so its "operator 2" is the slot at +4.
//
// Measured, because the interleaved reading is plausible enough to look right:
// over 120 files it scores mean 0.9027 / median 0.9252, against 0.9272 / 0.9472
// for this one. It is also what makes the algorithm table below coherent --
// with the interleave, instruments come out with their carriers silenced (an
// ALG 4 patch reading TL 28/127/0/127 would have BOTH carriers at 127).
constexpr uint8_t kOpOffset[4] = { 0, 4, 8, 12 };

// Which operators are carriers, per algorithm; only carriers take the channel
// volume, modulators keep the instrument's own TL. Bit i is the operator at
// register offset 4*i, i.e. DefleMask's operator i+1 (see kOpOffset).
constexpr uint8_t kCarriers[8] = {
    0x08, // ALG 0: 1->2->3->4
    0x08, // ALG 1: (1+2)->3->4
    0x08, // ALG 2: (1+(2->3))->4
    0x08, // ALG 3: ((1->2)+3)->4
    0x0A, // ALG 4: (1->2) + (3->4)
    0x0E, // ALG 5: 1->(2,3,4)
    0x0E, // ALG 6: (1->2) + 3 + 4
    0x0F, // ALG 7: 1 + 2 + 3 + 4
};

// F-numbers for one octave. The YM2612 encodes pitch as an 11-bit F-number plus
// a 3-bit block (octave); within a block the F-number is linear in frequency, so
// one octave of values plus a block shift covers the whole range.
//
// Computed rather than tabulated: fnum = freq * 2^20 / (clock/144) for block 4,
// which is the standard reference octave.
double fnumForSemitone(double semi, double ymClock)
{
    // semi is an absolute semitone where 0 == C-0.
    double freq = 440.0 * std::pow(2.0, (semi - 57.0) / 12.0); // A-4 == semi 57
    double base = ymClock / 144.0;
    return freq * 1048576.0 / base;
}

// Normalised F-number + block for a (fractional) semitone. Split out because
// portamento works in F-NUMBER units, not semitones, and so needs the same
// base the note itself uses.
void fnumBlockFor(double semi, double ymClock, double& fnum, int& block)
{
    if (semi < 0) { semi = 0; }
    if (semi > 119) { semi = 119; }
    block = static_cast<int>(semi / 12.0);
    if (block < 0) { block = 0; }
    if (block > 7) { block = 7; }
    fnum = fnumForSemitone(semi, ymClock) / std::pow(2.0, block - 1);
    while (fnum > 2047.0 && block < 7) {
        block++;
        fnum /= 2.0;
    }
    while (fnum < 1024.0 && block > 0) {
        block--;
        fnum *= 2.0;
    }
}

int16_t clampS16(int v)
{
    if (v > 32767) { return 32767; }
    if (v < -32768) { return -32768; }
    return static_cast<int16_t>(v);
}

} // namespace

// ymfm needs an interface object; the YM2612 has no external memory so every
// hook is a no-op.
class DmfYmfmInterface : public ymfm::ymfm_interface
{
};

class Ym2612Chip
{
public:
    Ym2612Chip() : chip(intf) {}
    DmfYmfmInterface intf;
    ymfm::ym2612 chip;
    ymfm::ym2612::output_data out;
};

bool playableSystem(const Module& m)
{
    // Plain Genesis (EXT.CH3 excluded -- see the note in dmf_player.h) and SMS.
    if (m.system == SYS_GENESIS) { return m.totalChannels == 10; }
    if (m.system == SYS_SMS) { return m.totalChannels == 4; }
    return false;
}

Player::Player() = default;
Player::~Player() = default;

int Player::noteToSemitone(const Row& r)
{
    // The spec's note table runs 01 C#, 02 D- ... 11 B-, 12 C-, so C is 12 and
    // the semitone within the octave is simply note % 12.
    if (r.note == kNoteEmpty && r.octave == 0) { return -1; }
    if (r.note == kNoteOff) { return -2; }
    if (r.note > 12) { return -1; }
    return static_cast<int>(r.octave) * 12 + (r.note % 12);
}

bool Player::init(const Module& m, int rate, std::string& err)
{
    if (!playableSystem(m)) {
        err = "not a SEGA Genesis or Master System DefleMask module";
        return false;
    }
    mod_ = &m;
    rate_ = rate > 0 ? rate : 44100;

    // SMS has no FM side at all; its four channels are the PSG.
    fmChannels_ = (m.system == SYS_SMS) ? 0 : kFmChannels;
    psgBase_ = fmChannels_;

    const bool pal = (m.framesMode == 0);
    const double ymClock = pal ? kYmClockPal : kYmClockNtsc;
    const double psgClock = pal ? kPsgClockPal : kPsgClockNtsc;

    if (fmChannels_ > 0) {
        ym_.reset(new Ym2612Chip());
        ym_->chip.reset();
        fmStep_ = (ymClock / 144.0) / rate_;
        fmAcc_ = 0.0;
    } else {
        ym_.reset();
    }

    psg_.reset(psgClock, rate_);

    arpTickSpeed_ = m.arpTickSpeed > 0 ? m.arpTickSpeed : 1;
    speed1_ = m.tickTime1 > 0 ? m.tickTime1 : 6;
    speed2_ = m.tickTime2 > 0 ? m.tickTime2 : 6;

    // One tick every (timeBase + 1) frames of the base rate.
    const double tickHz = m.baseHz() / static_cast<double>(m.timeBase + 1);
    samplesPerTick_ = rate_ / (tickHz > 0 ? tickHz : 60.0);
    tickAcc_ = 0.0;

    ch_.assign(m.totalChannels, ChannelState{});
    for (int i = 0; i < m.totalChannels; i++) {
        ch_[i].isFm = (i < fmChannels_);
        ch_[i].volume = ch_[i].isFm ? 0x7F : 0x0F;
    }

    if (fmChannels_ > 0) {
        // LFO off, DAC off.
        fmWrite(0, 0x22, 0x00);
        fmWrite(0, 0x27, 0x00);
        fmWrite(0, 0x2B, 0x00);
        for (int c = 0; c < fmChannels_; c++) {
            int port = c < 3 ? 0 : 1;
            uint8_t idx = static_cast<uint8_t>(c % 3);
            fmWrite(port, static_cast<uint8_t>(0xB4 + idx), 0xC0);
        }
    }
    // Silence the PSG.
    for (int c = 0; c < 4; c++) {
        psgWrite(static_cast<uint8_t>(0x90 | (c << 5) | 0x0F));
    }

    row_ = 0;
    order_ = 0;
    tickInRow_ = 0;
    speedPhase_ = 0;
    looped_ = false;
    // NOT processRow() here: tick() runs it whenever tickInRow_ == 0, and the
    // first tick is exactly that case. Doing it here as well processed row 0
    // twice, firing every note on it a second time.
    return true;
}

void Player::fmWrite(int port, uint8_t reg, uint8_t val)
{
    // There is no YM2612 on the SMS. The FM effect column (10xy-1Dxx) is still
    // dispatched for every channel, so a module that carries one on an SMS
    // track -- or simply has junk in an unused effect slot -- must not reach a
    // chip that does not exist.
    if (!ym_) { return; }
    static const bool traceRegs = (getenv("DMFCR_REGS") != nullptr);
    if (traceRegs) { fprintf(stderr, "  reg p%d %02X = %02X\n", port, reg, val); }
    ym_->chip.write(static_cast<uint32_t>(port * 2 + 0), reg);
    ym_->chip.write(static_cast<uint32_t>(port * 2 + 1), val);
}

void Player::fmKeyOn(int c, bool on)
{
    // Key on/off is a single global register: %0000cccc where the high nibble
    // selects operators and the low bits the channel (0-2 = ch1-3, 4-6 = ch4-6).
    uint8_t chBits = static_cast<uint8_t>(c < 3 ? c : (c - 3) + 4);
    uint8_t v = static_cast<uint8_t>((on ? 0xF0 : 0x00) | chBits);
    fmWrite(0, 0x28, v);
    if (!on) { pendingKeyOn_ &= static_cast<uint8_t>(~(1u << c)); }
}

void Player::fmKeyOnDeferred(int c)
{
    // See pendingKeyOn_ in the header: the envelope only restarts if the chip
    // observes the off->on edge, so this cannot be written until the key-off
    // has been clocked through.
    if (c >= 0 && c < 6) { pendingKeyOn_ |= static_cast<uint8_t>(1u << c); }
}

void Player::flushPendingKeyOn()
{
    if (pendingKeyOn_ == 0) { return; }
    for (int c = 0; c < 6; c++) {
        if ((pendingKeyOn_ & (1u << c)) != 0) {
            uint8_t chBits = static_cast<uint8_t>(c < 3 ? c : (c - 3) + 4);
            fmWrite(0, 0x28, static_cast<uint8_t>(0xF0 | chBits));
        }
    }
    pendingKeyOn_ = 0;
}

void Player::loadFmInstrument(int c, const Instrument& ins)
{
    ChannelState& cs = ch_[c];
    int port = c < 3 ? 0 : 1;
    uint8_t idx = static_cast<uint8_t>(c % 3);

    cs.fmAlg = static_cast<uint8_t>(ins.alg & 7);
    cs.fmFb = static_cast<uint8_t>(ins.fb & 7);
    cs.fmFms = static_cast<uint8_t>(ins.lfo & 7);
    cs.fmAms = static_cast<uint8_t>(ins.lfo2 & 3);

    for (int o = 0; o < 4; o++) {
        const FmOperator& op = ins.ops[o];
        uint8_t off = static_cast<uint8_t>(kOpOffset[o] + idx);
        fmWrite(port, static_cast<uint8_t>(0x30 + off),
                static_cast<uint8_t>(((op.dt & 7) << 4) | (op.mult & 0x0F)));
        cs.opTl[o] = static_cast<uint8_t>(op.tl & 0x7F);
        fmWrite(port, static_cast<uint8_t>(0x50 + off),
                static_cast<uint8_t>(((op.rs & 3) << 6) | (op.ar & 0x1F)));
        fmWrite(port, static_cast<uint8_t>(0x60 + off),
                static_cast<uint8_t>(((op.am & 1) << 7) | (op.dr & 0x1F)));
        fmWrite(port, static_cast<uint8_t>(0x70 + off),
                static_cast<uint8_t>(op.d2r & 0x1F));
        fmWrite(port, static_cast<uint8_t>(0x80 + off),
                static_cast<uint8_t>(((op.sl & 0x0F) << 4) | (op.rr & 0x0F)));
        fmWrite(port, static_cast<uint8_t>(0x90 + off),
                static_cast<uint8_t>(op.ssgMode & 0x0F));
    }

    fmWrite(port, static_cast<uint8_t>(0xB0 + idx),
            static_cast<uint8_t>((cs.fmFb << 3) | cs.fmAlg));
    fmWrite(port, static_cast<uint8_t>(0xB4 + idx),
            static_cast<uint8_t>(cs.pan | (cs.fmAms << 4) | cs.fmFms));

    applyFmVolume(c);
}

void Player::applyFmVolume(int c)
{
    ChannelState& cs = ch_[c];
    int port = c < 3 ? 0 : 1;
    uint8_t idx = static_cast<uint8_t>(c % 3);
    uint8_t carriers = kCarriers[cs.fmAlg & 7];

    int vol = (kMuteMask & (1u << c)) ? 0 : cs.volume;
    if (cs.tremDepth > 0) {
        // Tremolo rides on top of the channel volume.
        static const int kTrem[32] = { 0,  12, 24, 36, 47, 58, 68, 78, 86, 92, 97,
                                       100,101,100, 97, 92, 86, 78, 68, 58, 47, 36,
                                       24, 12, 0,  0,  0,  0,  0,  0,  0,  0 };
        int d = kTrem[(cs.tremPos >> 2) & 31] * cs.tremDepth / 128;
        vol -= d;
    }
    if (vol < 0) { vol = 0; }
    if (vol > 0x7F) { vol = 0x7F; }

    for (int o = 0; o < 4; o++) {
        uint8_t off = static_cast<uint8_t>(kOpOffset[o] + idx);
        int tl = cs.opTl[o];
        if ((carriers & (1 << o)) != 0) {
            // Channel volume attenuates the carriers. DefleMask's FM volume is
            // 0..0x7F with 0x7F == loudest, and TL is attenuation with 0 ==
            // loudest, so the channel volume adds attenuation.
            tl += (0x7F - vol);
            if (tl > 0x7F) { tl = 0x7F; }
        }
        fmWrite(port, static_cast<uint8_t>(0x40 + off), static_cast<uint8_t>(tl));
    }
}

void Player::applyFmPitch(int c)
{
    ChannelState& cs = ch_[c];
    if (cs.semitone < 0) { return; }
    int port = c < 3 ? 0 : 1;
    uint8_t idx = static_cast<uint8_t>(c % 3);

    const bool pal = (mod_->framesMode == 0);
    const double ymClock = pal ? kYmClockPal : kYmClockNtsc;

    // Semitone-domain modifiers (arpeggio, vibrato, fine tune) first...
    double semi = cs.semitone + (cs.pitch + cs.fineTune + globalFineTune_) / 64.0;
    double fnum;
    int block;
    fnumBlockFor(semi, ymClock, fnum, block);

    // ...then the portamento offset, which the manual defines in FREQUENCY
    // rather than in semitones: "change the frequency by adding the xx value on
    // each tick". On this chip the frequency register is the F-number, so the
    // slide is a straight addition to it and the resulting glide is linear in
    // frequency, not in pitch.
    fnum += cs.fnumOffset;
    while (fnum > 2047.0 && block < 7) {
        block++;
        fnum /= 2.0;
    }
    while (fnum < 1024.0 && block > 0) {
        block--;
        fnum *= 2.0;
    }

    int f = static_cast<int>(fnum + 0.5);
    if (f < 0) { f = 0; }
    if (f > 2047) { f = 2047; }

    // High byte must be written first: the chip latches it and commits both on
    // the low write.
    fmWrite(port, static_cast<uint8_t>(0xA4 + idx),
            static_cast<uint8_t>(((block & 7) << 3) | ((f >> 8) & 7)));
    fmWrite(port, static_cast<uint8_t>(0xA0 + idx), static_cast<uint8_t>(f & 0xFF));
}

// The F-number distance from the channel's current note to `targetSemi`, which
// is what 3xx has to travel. Computed at the note's own block so it is in the
// same units the slide accumulates in.
double Player::fnumDeltaToNote(int c, int targetSemi) const
{
    const bool pal = (mod_->framesMode == 0);
    const double ymClock = pal ? kYmClockPal : kYmClockNtsc;
    const ChannelState& cs = ch_[c];
    double f0, f1;
    int b0, b1;
    fnumBlockFor(cs.semitone, ymClock, f0, b0);
    fnumBlockFor(targetSemi, ymClock, f1, b1);
    // Bring the target into the current note's block so the two are comparable.
    f1 *= std::pow(2.0, b1 - b0);
    return f1 - f0;
}

void Player::applyPsg(int c)
{
    ChannelState& cs = ch_[c];
    int psgCh = c - psgBase_; // 0..3, 3 == noise
    const bool pal = (mod_->framesMode == 0);
    const double psgClock = pal ? kPsgClockPal : kPsgClockNtsc;

    int vol = (kMuteMask & (1u << c)) ? 0 : cs.volume;
    if (vol < 0) { vol = 0; }
    if (vol > 0x0F) { vol = 0x0F; }
    // PSG registers hold ATTENUATION, so a full-volume 0x0F becomes 0.
    uint8_t att = static_cast<uint8_t>(0x0F - vol);
    if (cs.semitone < 0 || !cs.keyOn) { att = 0x0F; }
    psgWrite(static_cast<uint8_t>(0x90 | (psgCh << 5) | att));

    if (cs.semitone < 0) { return; }

    if (psgCh == 3) {
        // Noise: the low two bits pick the shift rate, bit 2 white/periodic.
        uint8_t mode = static_cast<uint8_t>(cs.psgNoiseMode & 0x07);
        psgWrite(static_cast<uint8_t>(0xE0 | mode));
        return;
    }

    double semi = cs.semitone + (cs.pitch + cs.fineTune + globalFineTune_) / 64.0;
    double freq = 440.0 * std::pow(2.0, (semi - 57.0) / 12.0);
    if (freq < 1.0) { freq = 1.0; }
    int period = static_cast<int>(psgClock / (32.0 * freq) + 0.5);
    // Same frequency-domain portamento as the FM side, but this chip's register
    // is a PERIOD divisor, so a rising frequency is a falling register value.
    period -= static_cast<int>(cs.fnumOffset + (cs.fnumOffset >= 0 ? 0.5 : -0.5));
    if (period < 1) { period = 1; }
    if (period > 1023) { period = 1023; }
    psgWrite(static_cast<uint8_t>(0x80 | (psgCh << 5) | (period & 0x0F)));
    psgWrite(static_cast<uint8_t>((period >> 4) & 0x3F));
}

void Player::processRow()
{
    pendingOrder_ = -1;
    pendingRow_ = -1;

    for (int c = 0; c < mod_->totalChannels; c++) {
        const Channel& chan = mod_->channels[c];
        // Index the stored pattern blocks by ORDER POSITION, not by the matrix
        // value. DMF writes one block per channel per order row, and when the
        // same pattern number appears at several positions the CONTENT is
        // duplicated into each -- verified over 200 files: of 21,290 duplicate
        // matrix entries, all 21,290 had byte-identical content at those
        // positions and none differed. So the two readings select the same
        // notes, except that a matrix value >= the block count made the
        // by-value form fall back to pattern 0 and play the wrong bar. Indexing
        // by position cannot go out of range and removes that case.
        if (order_ >= static_cast<int>(chan.patterns.size())) { continue; }
        const Pattern& p = chan.patterns[order_];
        if (row_ >= static_cast<int>(p.rows.size())) { continue; }
        const Row& r = p.rows[row_];

        ChannelState& cs = ch_[c];
        cs.cutTick = -1;
        cs.delayTick = -1;
        cs.retrig = 0;

        // EDxx note delay defers the whole cell.
        int delay = -1;
        for (int e = 0; e < chan.effectColumns; e++) {
            if (r.effects[e].code == 0xED && r.effects[e].value > 0) {
                delay = r.effects[e].value;
            }
        }
        if (delay > 0) {
            cs.delayTick = delay;
            cs.delayedRow = r;
            continue;
        }

        rowEffects(cs, c, r);
    }
}

void Player::rowEffects(ChannelState& cs, int c, const Row& r)
{
    const Channel& chan = mod_->channels[c];
    int semi = noteToSemitone(r);

    // Instrument change.
    if (r.instrument >= 0 && r.instrument < static_cast<int>(mod_->instruments.size())) {
        if (r.instrument != cs.instrument) {
            cs.instrument = r.instrument;
            if (cs.isFm && mod_->instruments[r.instrument].fm) {
                loadFmInstrument(c, mod_->instruments[r.instrument]);
            }
        }
        cs.macroVolPos = cs.macroArpPos = cs.macroDutyPos = 0;
        cs.macroDone = false;
    }

    if (r.volume >= 0) {
        cs.volume = r.volume;
        if (cs.isFm) { applyFmVolume(c); }
    }

    // A row without a volume-slide effect cancels one left running by an
    // earlier row. DefleMask documents 0xy/1xx/2xx/3xx/4xy as persisting until
    // turned off with 00 but says nothing either way for Axy, so this was
    // measured: over the whole corpus it is a wash, but across the worst files
    // -- the ones where a stuck slide runs the volume to an end stop and keeps
    // it there -- it is clearly better (mean 0.658 vs 0.636).
    {
        bool hasVolSlide = false;
        for (int e = 0; e < chan.effectColumns; e++) {
            int code = r.effects[e].code;
            if (code == 0x0A || code == 0x05 || code == 0x06) { hasVolSlide = true; }
        }
        if (!hasVolSlide) { cs.volSlide = 0; }
    }

    // Scan effects first so 3xx (porta to note) can intercept the note.
    bool portaToNote = false;
    for (int e = 0; e < chan.effectColumns; e++) {
        if (r.effects[e].code == 0x03) { portaToNote = true; }
    }

    if (semi == -2) { // note off
        cs.keyOn = false;
        if (cs.isFm) { fmKeyOn(c, false); }
        else { applyPsg(c); }
    } else if (semi >= 0) {
        // A channel whose rows never carry an instrument column still has to
        // sound like something. DefleMask starts with instrument 0 selected, so
        // that is what such a note plays. Leaving the channel patchless instead
        // is not neutral -- an unwritten YM2612 has every operator at TL 0, i.e.
        // FULL output, so the channel blares a default patch under the track.
        if (cs.instrument < 0 && !mod_->instruments.empty()) {
            cs.instrument = 0;
            if (cs.isFm && mod_->instruments[0].fm) {
                loadFmInstrument(c, mod_->instruments[0]);
            }
        }
        if (portaToNote && cs.semitone >= 0) {
            cs.portaTarget = semi;
            cs.portaDelta = fnumDeltaToNote(c, semi);
        } else {
            cs.semitone = semi;
            cs.pitch = 0;
            cs.fnumOffset = 0.0;
            cs.portaDelta = 0.0;
            cs.portaTarget = -1;
            cs.vibPos = 0;
            cs.arpPos = 0;
            cs.macroVolPos = cs.macroArpPos = cs.macroDutyPos = 0;
            cs.macroDone = false;
            cs.keyOn = true;
            DMFCR_TRACE("noteon ch%-2d semi %3d ins %2d vol %3d %s\n", c, semi,
                        cs.instrument, cs.volume, cs.isFm ? "FM" : "PSG");
            // With the DAC enabled, channel 6 IS the sample channel -- the FM
            // side of it is taken over and must not sound. Keying it on anyway
            // leaves a full-volume patch blaring under the whole track, and
            // typically a DEFAULT one: DAC rows usually carry no instrument
            // column at all, so nothing ever loads and every operator sits at
            // TL 0, i.e. maximum output.
            const bool dacTakesChannel = (c == 5 && dacEnabled_);
            if (cs.isFm && !dacTakesChannel) {
                fmKeyOn(c, false);
                applyFmPitch(c);
                fmKeyOnDeferred(c);
            } else if (cs.isFm) {
                fmKeyOn(c, false);
            } else {
                applyPsg(c);
            }
        }
    }

    for (int e = 0; e < chan.effectColumns; e++) {
        int code = r.effects[e].code;
        int val = r.effects[e].value;
        if (code < 0) { continue; }
        if (val < 0) { val = 0; }
        int x = (val >> 4) & 0x0F;
        int y = val & 0x0F;

        switch (code) {
        case 0x00: // 0xy arpeggio
            cs.arpX = x;
            cs.arpY = y;
            if (val == 0) { cs.arpPos = 0; }
            break;
        case 0x01: cs.slideSpeed = val; break;   // portamento up
        case 0x02: cs.slideSpeed = -val; break;  // portamento down
        case 0x03: cs.portaSpeed = val; break;   // porta to note
        case 0x04: // vibrato
            if (x != 0) { cs.vibSpeed = x; }
            if (y != 0) { cs.vibDepth = y; }
            if (val == 0) { cs.vibDepth = 0; }
            break;
        case 0x05: // porta to note + volume slide
            cs.volSlide = (x != 0) ? x : -y;
            break;
        case 0x06: // vibrato + volume slide
            cs.volSlide = (x != 0) ? x : -y;
            break;
        case 0x07: // tremolo
            if (x != 0) { cs.tremSpeed = x; }
            cs.tremDepth = y;
            break;
        case 0x08: { // panning
            // The manual documents 0x01 right, 0x10 left, 0x11 both, and that is
            // exactly what the corpus contains: a census over 127 Genesis files
            // found only 0x01/0x10/0x11 (and the equivalent 0x0F/0xF0/0xFF),
            // plus a bare 0x00 (238 rows).
            //
            // 0x00 is treated as "both speakers", not as silence. On the chip
            // the two 0xB4 bits ARE the speaker enables, so the literal reading
            // of 0x00 is no output -- but that was measured both ways over 120
            // files and the literal reading scored very slightly WORSE (mean
            // cosine 0.8648 vs 0.8663), so a bare 0800 is evidently a "reset
            // panning" idiom rather than a mute. It is also the safer of the
            // two: this way a misread can never silence a channel outright.
            uint8_t pan = 0;
            if ((val & 0x0F) != 0) { pan |= 0x40; } // right
            if ((val & 0xF0) != 0) { pan |= 0x80; } // left
            if (pan == 0) { pan = 0xC0; }
            cs.pan = pan;
            if (cs.isFm) {
                int port = c < 3 ? 0 : 1;
                uint8_t idx = static_cast<uint8_t>(c % 3);
                fmWrite(port, static_cast<uint8_t>(0xB4 + idx),
                        static_cast<uint8_t>(cs.pan | (cs.fmAms << 4) | cs.fmFms));
            }
            break;
        }
        case 0x09: if (val > 0) { speed1_ = val; } break;
        case 0x0A: cs.volSlide = (x != 0) ? x : -y; break;
        case 0x0B: pendingOrder_ = val; pendingRow_ = 0; break;
        case 0x0C: cs.retrig = val; break;
        case 0x0D: pendingOrder_ = order_ + 1; pendingRow_ = val; break;
        case 0x0F: if (val > 0) { speed2_ = val; } break;

        // --- Exx extended -------------------------------------------------
        case 0xE0: // arpeggio tick speed; 0 would freeze the arpeggio
            if (val > 0) { arpTickSpeed_ = val; }
            break;
        case 0xE3: cs.vibMode = val & 3; break;
        case 0xE4: cs.vibFine = val & 0x0F; break;
        case 0xE5: cs.fineTune = (val - 0x80) / 2; break;
        case 0xEC: cs.cutTick = val; break;
        case 0xEF: globalFineTune_ += (val - 0x80) / 2; break;
        // E1xy / E2xy note slide: x is the speed, y the number of semitones to
        // move by. Documented as "similar to 3xx", so it is a portamento with
        // the destination given as an offset rather than as the next note.
        case 0xE1:
            if (cs.semitone >= 0) {
                cs.noteSlideSpeed = x;
                cs.noteSlideTarget = cs.semitone + y;
            }
            break;
        case 0xE2:
            if (cs.semitone >= 0) {
                cs.noteSlideSpeed = -x;
                cs.noteSlideTarget = cs.semitone - y;
            }
            break;
        case 0xEB: dacBank_ = val; break;

        // --- YM2612 -------------------------------------------------------
        case 0x10: // 10xy LFO control
            fmWrite(0, 0x22, static_cast<uint8_t>((x != 0 ? 0x08 : 0x00) | (y & 7)));
            break;
        case 0x11: { // 11xx feedback
            cs.fmFb = static_cast<uint8_t>(val & 7);
            int port = c < 3 ? 0 : 1;
            uint8_t idx = static_cast<uint8_t>(c % 3);
            fmWrite(port, static_cast<uint8_t>(0xB0 + idx),
                    static_cast<uint8_t>((cs.fmFb << 3) | cs.fmAlg));
            break;
        }
        case 0x12: case 0x13: case 0x14: case 0x15: { // TL of operator 1..4
            int op = code - 0x12;
            cs.opTl[op] = static_cast<uint8_t>(val & 0x7F);
            applyFmVolume(c);
            break;
        }
        case 0x16: { // 16xy MULT of operator x
            int op = x - 1;
            if (op >= 0 && op < 4) {
                int port = c < 3 ? 0 : 1;
                uint8_t idx = static_cast<uint8_t>(c % 3);
                uint8_t off = static_cast<uint8_t>(kOpOffset[op] + idx);
                const Instrument* ins =
                    (cs.instrument >= 0 &&
                     cs.instrument < static_cast<int>(mod_->instruments.size()))
                        ? &mod_->instruments[cs.instrument]
                        : nullptr;
                uint8_t dt = ins != nullptr ? static_cast<uint8_t>(ins->ops[op].dt & 7) : 0;
                fmWrite(port, static_cast<uint8_t>(0x30 + off),
                        static_cast<uint8_t>((dt << 4) | (y & 0x0F)));
            }
            break;
        }
        case 0x17: // 17xx DAC enable
            dacEnabled_ = (val != 0);
            fmWrite(0, 0x2B, static_cast<uint8_t>(dacEnabled_ ? 0x80 : 0x00));
            if (dacEnabled_ && fmChannels_ > 5) {
                // Take channel 6's FM voice out immediately, not just at the
                // next note -- it may already be sounding.
                fmKeyOn(5, false);
            }
            break;
        case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: { // AR control
            int port = c < 3 ? 0 : 1;
            uint8_t idx = static_cast<uint8_t>(c % 3);
            int lo = (code == 0x19) ? 0 : (code - 0x1A);
            int hi = (code == 0x19) ? 3 : (code - 0x1A);
            const Instrument* ins =
                (cs.instrument >= 0 &&
                 cs.instrument < static_cast<int>(mod_->instruments.size()))
                    ? &mod_->instruments[cs.instrument]
                    : nullptr;
            for (int o = lo; o <= hi; o++) {
                uint8_t off = static_cast<uint8_t>(kOpOffset[o] + idx);
                uint8_t rs = ins != nullptr ? static_cast<uint8_t>(ins->ops[o].rs & 3) : 0;
                fmWrite(port, static_cast<uint8_t>(0x50 + off),
                        static_cast<uint8_t>((rs << 6) | (val & 0x1F)));
            }
            break;
        }

        // --- SN76489 ------------------------------------------------------
        case 0x20: // 20xy noise mode
            cs.psgNoiseMode = ((y != 0) ? 0x04 : 0x00) | (x != 0 ? 0x03 : 0x00);
            if (!cs.isFm) { applyPsg(c); }
            break;

        default:
            break;
        }
    }

    // A DAC-enabled channel 6 plays a PCM sample INSTEAD of FM -- the chip's
    // own DAC takes the channel over, which is why this drives registers
    // 0x2A/0x2B rather than mixing a sample in alongside the FM output.
    if (c == 5 && dacEnabled_ && semi >= 0 && !mod_->samples.empty()) {
        // Which sample the note picks. DefleMask's manual documents "a max of
        // 12 sample banks ... from 0 to 11" (EBxx) and an octave is 12 notes,
        // so the note within the octave selects inside the bank. Measured
        // against the alternatives; see kDacSelect.
        // Measured against the alternatives over the 363 DAC files: this scores
        // 0.9299, "note - 1" 0.9293, "always the first sample" 0.9291.
        int idx = dacBank_ * 12 + (static_cast<int>(r.note) % 12);
        if (idx < 0 || idx >= static_cast<int>(mod_->samples.size())) { idx = 0; }
        dacSample_ = &mod_->samples[idx];
        dacPos_ = 0.0;
        // Sample rates are indexed 1..5 in the corpus (never 0).
        static const double kRates[] = { 8000, 8000, 11025, 16000, 22050, 32000 };
        int ri = dacSample_->rate;
        if (ri < 0 || ri > 5) { ri = 5; }
        // The note does NOT pitch the sample -- it always plays at its recorded
        // rate. That is what the hardware does: the Mega Drive's DAC is fed a
        // byte at a time by the CPU at a fixed rate, so there is no resampling
        // to pitch with, and DefleMask does not do it in software either.
        //
        // Measured over the 363 DAC files, and the trend is unambiguous --
        // treating the note as a pitch against a C-5 reference scores mean
        // 0.9325, C-4 0.9345, C-3 0.9369, and not pitching at all 0.9416.
        //
        // Getting this wrong is expensive and hard to see. A sample is often
        // mostly silence with one burst in the middle; playing it at a quarter
        // speed means the pattern loops and retriggers before the burst is ever
        // reached, so the drum track is simply absent. On the file that led
        // here it cost about 40 dB -- the first 21% of the sample it managed to
        // reach has RMS 75 against the full sample's 3291.
        dacStep_ = kRates[ri] / rate_;
    }
}

void Player::tickEffects(ChannelState& cs, int c)
{
    bool pitchDirty = false;

    if (cs.slideSpeed != 0 && cs.semitone >= 0) {
        cs.fnumOffset += cs.slideSpeed;
        pitchDirty = true;
    }

    if (cs.portaTarget >= 0 && cs.portaSpeed > 0 && cs.semitone >= 0) {
        // Also a frequency-domain slide: walk fnumOffset towards the F-number
        // distance to the destination note, then snap to that note when it
        // arrives so later effects work from the right base.
        double want = cs.portaDelta;
        if (cs.fnumOffset < want) {
            cs.fnumOffset += cs.portaSpeed;
            if (cs.fnumOffset >= want) { cs.fnumOffset = want; }
        } else if (cs.fnumOffset > want) {
            cs.fnumOffset -= cs.portaSpeed;
            if (cs.fnumOffset <= want) { cs.fnumOffset = want; }
        }
        if (cs.fnumOffset == want) {
            cs.semitone = cs.portaTarget;
            cs.portaTarget = -1;
            cs.fnumOffset = 0.0;
            cs.portaDelta = 0.0;
        }
        pitchDirty = true;
    }

    // E1xy / E2xy note slide -- a portamento towards an offset destination.
    if (cs.noteSlideTarget >= 0 && cs.noteSlideSpeed != 0 && cs.semitone >= 0) {
        int cur = cs.semitone * 64 + cs.pitch;
        int tgt = cs.noteSlideTarget * 64;
        int step = cs.noteSlideSpeed > 0 ? cs.noteSlideSpeed : -cs.noteSlideSpeed;
        if (cur < tgt) {
            cur += step;
            if (cur >= tgt) { cur = tgt; cs.noteSlideTarget = -1; }
        } else if (cur > tgt) {
            cur -= step;
            if (cur <= tgt) { cur = tgt; cs.noteSlideTarget = -1; }
        } else {
            cs.noteSlideTarget = -1;
        }
        cs.pitch = cur - cs.semitone * 64;
        pitchDirty = true;
    }

    if (cs.vibDepth > 0) {
        cs.vibPos = (cs.vibPos + cs.vibSpeed) & 63;
        double s = std::sin(cs.vibPos * 2.0 * M_PI / 64.0);
        if (cs.vibMode == 1 && s < 0) { s = -s; }
        if (cs.vibMode == 2 && s > 0) { s = -s; }
        // E4xx scales the depth; its default is F, i.e. no reduction.
        double fine = cs.vibFine / 15.0;
        cs.pitch += static_cast<int>(s * cs.vibDepth * 2.0 * fine);
        pitchDirty = true;
    }

    // Cxx retrig: restart the note every `retrig` ticks for the rest of the row.
    if (cs.retrig > 0 && tickInRow_ > 0 && (tickInRow_ % cs.retrig) == 0 &&
        cs.semitone >= 0) {
        if (cs.isFm) {
            fmKeyOn(c, false);
            fmKeyOnDeferred(c);
        } else {
            cs.keyOn = true;
            applyPsg(c);
        }
    }

    if (cs.tremDepth > 0) {
        cs.tremPos += cs.tremSpeed;
        if (cs.isFm) { applyFmVolume(c); } else { applyPsg(c); }
    }

    // E0xx sets how many ticks each arpeggio step lasts; DMF versions below
    // 0x14 carry an initial value in the module header.
    if (cs.arpX != 0 || cs.arpY != 0) {
        if (++cs.arpTick >= (arpTickSpeed_ > 0 ? arpTickSpeed_ : 1)) {
            cs.arpTick = 0;
            cs.arpPos = (cs.arpPos + 1) % 3;
        }
        pitchDirty = true;
    }

    if (cs.volSlide != 0) {
        cs.volume += cs.volSlide;
        int maxv = cs.isFm ? 0x7F : 0x0F;
        if (cs.volume < 0) { cs.volume = 0; }
        if (cs.volume > maxv) { cs.volume = maxv; }
        if (cs.isFm) { applyFmVolume(c); } else { applyPsg(c); }
    }

    if (cs.cutTick >= 0 && tickInRow_ >= cs.cutTick) {
        cs.cutTick = -1;
        cs.keyOn = false;
        if (cs.isFm) { fmKeyOn(c, false); } else { applyPsg(c); }
    }

    if (cs.delayTick >= 0 && tickInRow_ >= cs.delayTick) {
        int d = cs.delayTick;
        cs.delayTick = -1;
        Row r = cs.delayedRow;
        (void)d;
        rowEffects(cs, c, r);
        return;
    }

    if (pitchDirty) {
        int save = cs.pitch;
        if (cs.arpX != 0 || cs.arpY != 0) {
            int add = (cs.arpPos == 1) ? cs.arpX : (cs.arpPos == 2 ? cs.arpY : 0);
            cs.pitch += add * 64;
        }
        if (cs.isFm) { applyFmPitch(c); } else { applyPsg(c); }
        cs.pitch = save;
    }
}

void Player::runMacros(ChannelState& cs, int c)
{
    if (cs.instrument < 0 ||
        cs.instrument >= static_cast<int>(mod_->instruments.size())) {
        return;
    }
    const Instrument& ins = mod_->instruments[cs.instrument];
    if (ins.fm) { return; } // STD macros only

    auto step = [](const Macro& m, int& pos) -> const int32_t* {
        if (m.values.empty()) { return nullptr; }
        if (pos >= static_cast<int>(m.values.size())) {
            if (m.loopPos >= 0) { pos = m.loopPos; }
            else { pos = static_cast<int>(m.values.size()) - 1; }
        }
        return &m.values[pos++];
    };

    if (const int32_t* v = step(ins.volume, cs.macroVolPos)) {
        int maxv = cs.isFm ? 0x7F : 0x0F;
        int nv = *v;
        if (nv < 0) { nv = 0; }
        if (nv > maxv) { nv = maxv; }
        cs.volume = nv;
        if (cs.isFm) { applyFmVolume(c); } else { applyPsg(c); }
    }
    if (const int32_t* v = step(ins.arpeggio, cs.macroArpPos)) {
        if (cs.semitone >= 0) {
            int save = cs.pitch;
            // Values are stored with a +12 offset in relative mode.
            cs.pitch += (ins.arpeggio.mode == 1) ? 0 : (*v - 12) * 64;
            if (cs.isFm) { applyFmPitch(c); } else { applyPsg(c); }
            cs.pitch = save;
        }
    }
    if (const int32_t* v = step(ins.duty, cs.macroDutyPos)) {
        if (!cs.isFm && (c - psgBase_) == 3) {
            cs.psgNoiseMode = *v & 7;
            applyPsg(c);
        }
    }
}

void Player::advanceRow()
{
    if (pendingOrder_ >= 0) {
        int next = pendingOrder_;
        int wantRow = pendingRow_ > 0 ? pendingRow_ : 0;
        pendingOrder_ = -1;
        pendingRow_ = -1;

        if (next >= mod_->matrixRows) {
            // Ran off the end of the order list. A Dxx here is documented as a
            // no-op ("will not work on the last pattern of the song, because
            // there is no next pattern"), so the song is simply over.
            order_ = 0;
            row_ = 0;
            ended_ = true;
        } else {
            // A BACKWARD jump is the module saying "loop here" -- that, and only
            // that, makes this a looping song rather than a one-shot.
            if (next <= order_) { looped_ = true; }
            order_ = next;
            row_ = wantRow;
        }
        speedPhase_ = row_ & 1;
        return;
    }
    row_++;
    if (row_ >= static_cast<int>(mod_->rowsPerPattern)) {
        row_ = 0;
        order_++;
        if (order_ >= mod_->matrixRows) {
            // Fell off the end of the order list on its own. This is a LOOP,
            // not an end: Furnace's default for tracker modules is to repeat
            // indefinitely, and treating it as an end was measured to be wrong
            // (it cut short many modules Furnace keeps playing). Only an
            // explicit jump PAST the order list ends a song -- see above.
            order_ = 0;
            looped_ = true;
        }
    }
    speedPhase_ = row_ & 1;
}

void Player::tick()
{
    if (tickInRow_ == 0) { processRow(); }

    for (int c = 0; c < mod_->totalChannels; c++) {
        runMacros(ch_[c], c);
        tickEffects(ch_[c], c);
    }

    tickInRow_++;
    // Which of the two tick times this row gets. DefleMask alternates them, but
    // the published manual does not say what the alternation is keyed on, and
    // it only becomes observable once a jump lands on an odd row: keying on the
    // row INDEX resets the phase at every jump, whereas a running toggle
    // carries it across. Measured both ways -- see kSpeedPhaseToggle.
    int rowTicks = (speedPhase_ == 0) ? speed1_ : speed2_;
    if (rowTicks < 1) { rowTicks = 1; }
    if (tickInRow_ >= rowTicks) {
        tickInRow_ = 0;
        advanceRow();
    }
}

void Player::render(float* out, int frames)
{
    for (int i = 0; i < frames; i++) {
        tickAcc_ += 1.0;
        while (tickAcc_ >= samplesPerTick_) {
            tickAcc_ -= samplesPerTick_;
            tick();
        }

        // Advance ymfm at its native rate and linearly interpolate. On SMS
        // there is no YM2612 at all and the FM side contributes nothing.
        float l = 0.0f;
        float r = 0.0f;
        if (ym_) {
            fmAcc_ += fmStep_;
            while (fmAcc_ >= 1.0) {
                fmAcc_ -= 1.0;
                fmPrev_[0] = fmCur_[0];
                fmPrev_[1] = fmCur_[1];
                ym_->chip.generate(&ym_->out, 1);
                fmCur_[0] = ym_->out.data[0] / 32768.0f;
                fmCur_[1] = ym_->out.data[1] / 32768.0f;
            }
            float t = static_cast<float>(fmAcc_);
            l = fmPrev_[0] + (fmCur_[0] - fmPrev_[0]) * t;
            r = fmPrev_[1] + (fmCur_[1] - fmPrev_[1]) * t;

            // The chip has now been clocked, so any key-off issued by this
            // sample's tick has been observed and the matching key-on can go
            // out. See pendingKeyOn_.
            flushPendingKeyOn();
        }

        // Feed the chip's DAC. Writing register 0x2A is what actually makes the
        // sample come out: with 0x2B bit 7 set, ymfm replaces FM channel 6 with
        // this value, so channel 6 is silenced for free and the DAC sits at the
        // right point in the mix. Mixing the sample in alongside the FM output
        // instead -- which this used to do -- leaves channel 6 audible
        // underneath every drum hit.
        if (ym_ && dacEnabled_ && dacSample_ != nullptr &&
            !dacSample_->data.empty()) {
            size_t si = static_cast<size_t>(dacPos_);
            if (si < dacSample_->data.size()) {
                int16_t raw = dacSample_->data[si];
                // 8-bit samples are stored UNSIGNED 0..255 in the 16-bit words;
                // 16-bit samples are signed. Measured over the corpus: the
                // 8-bit range is exactly 0..255 and the 16-bit range is the
                // full -32768..32767. Reading the 8-bit ones as signed makes
                // them near-silent positive DC.
                int v8;
                if (dacSample_->bits == 8) {
                    v8 = raw & 0xFF;
                } else {
                    v8 = (static_cast<int>(raw) >> 8) + 0x80;
                }
                // "Sample Amp" is a percentage (the corpus is mostly 50 and
                // 100), applied about the DAC's 0x80 centre.
                int amp = dacSample_->amp > 0 ? dacSample_->amp : 100;
                v8 = 0x80 + ((v8 - 0x80) * amp) / 100;
                if (v8 < 0) { v8 = 0; }
                if (v8 > 255) { v8 = 255; }
                // Mixed in here rather than pushed through the chip's own DAC
                // register. Driving 0x2A is the faithful route and DOES silence
                // channel 6 for free, but measured it loses sample-only modules
                // outright (one went 0.904 -> 0.021), so the sample is mixed
                // directly and channel 6 is silenced explicitly instead -- see
                // the 0x2B write on effect 17.
                // Half scale, swept against Furnace over the 363 DAC files:
                // flat from 0.25 to 1.0 (0.9404-0.9415), so this sits mid-range.
                float s = static_cast<float>((v8 - 0x80) / 128.0 * 0.5);
                l += s;
                r += s;
                dacPos_ += dacStep_;
            } else {
                dacSample_ = nullptr;
            }
        }

        float p = psg_.tick();
        l += p;
        r += p;

        // Block DC. ymfm models the YM2612's DAC "ladder" discontinuity, which
        // leaves a standing offset even with every channel silent -- a reset
        // chip generating nothing still reports 504. A real Mega Drive's output
        // is AC-coupled, so that offset never reaches the speaker; without this
        // filter "silence" here is a constant 504 rather than zero, which both
        // biases the mix and defeats silence detection.
        const float kDcPole = 0.9995f;
        float dl = l - dcPrevIn_[0] + kDcPole * dcPrevOut_[0];
        float dr = r - dcPrevIn_[1] + kDcPole * dcPrevOut_[1];
        dcPrevIn_[0] = l;
        dcPrevIn_[1] = r;
        dcPrevOut_[0] = dl;
        dcPrevOut_[1] = dr;

        out[i * 2 + 0] = dl;
        out[i * 2 + 1] = dr;
    }
}

} // namespace dmfcr
