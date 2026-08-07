#pragma once
//
// Clean-room DefleMask sequencer -- SEGA Genesis and SEGA Master System.
//
// Drives a YM2612 (Aaron Giles' ymfm, BSD-3, external/ymfm) and an SN76489
// (sn76489.h, written here) from a parsed Module. Behaviour comes from
// DefleMask's own published manual -- the effect tables are archived verbatim
// in spec/EFFECTS_standard_legacy_manual.txt and spec/EFFECTS_genesis_legacy_manual.txt.
// Nothing here is derived from Furnace. See README.md.
//
// Scope: SYSTEM_GENESIS (0x02) and SYSTEM_SMS (0x03). The SMS is the same
// SN76489 with no FM side, so it costs only a channel-layout change.
//
// The EXT.CH3 Genesis variants (0x42, and 0x12 as the older specs label it) are
// NOT handled -- they split FM channel 3 into four independently-pitched
// operators, which is a different channel model rather than an extra effect.
// 57 of the 833 Genesis files use it; they are declined so they keep falling
// through to Furnace in the plus build rather than being played wrongly here.

#include "dmf_file.h"
#include "sn76489.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace dmfcr {

class Ym2612Chip; // opaque ymfm holder, defined in dmf_player.cpp

// True if this module targets a system this player can drive: plain SEGA
// Genesis (0x02) or SEGA Master System (0x03). The SMS is the same SN76489
// this player already had for the Genesis PSG, with no FM side at all.
bool playableSystem(const Module& m);

class Player
{
public:
    Player();
    ~Player();

    // `rate` is the output sample rate. Returns false (with `err` set) if the
    // module targets a system this player does not cover.
    bool init(const Module& m, int rate, std::string& err);

    // Render interleaved stereo float frames.
    void render(float* out, int frames);

    // Goes true once the order list has wrapped back to its start.
    bool looped() const { return looped_; }

    // Goes true when the song ran off the END of its order list without ever
    // taking a backward Bxx jump -- i.e. it is a one-shot, not a loop. Jingles
    // ("course clear", "1-up", "victory") are the common case. Furnace reports
    // exactly this through isPlaying(), and a player that instead restarts such
    // a module plays it forever.
    bool ended() const { return ended_; }

private:
    // --- sequencer -------------------------------------------------------
    void tick();          // one engine tick
    void processRow();    // tick 0 of a row: read the pattern cells
    void advanceRow();

    struct ChannelState;
    void rowEffects(ChannelState& cs, int ch, const Row& row);
    void tickEffects(ChannelState& cs, int ch);
    void runMacros(ChannelState& cs, int ch);

    // --- chip plumbing ---------------------------------------------------
    void fmWrite(int port, uint8_t reg, uint8_t val);
    void fmKeyOn(int ch, bool on);
    // Request a key-on that will be issued after the chip has been clocked at
    // least once -- see the note on pendingKeyOn_.
    void fmKeyOnDeferred(int ch);
    void flushPendingKeyOn();
    void loadFmInstrument(int ch, const Instrument& ins);
    void applyFmVolume(int ch);
    void applyFmPitch(int ch);
    double fnumDeltaToNote(int ch, int targetSemi) const;
    void applyPsg(int ch);
    void psgWrite(uint8_t v) { psg_.write(v); }

    static int noteToSemitone(const Row& r);

    const Module* mod_ = nullptr;
    int rate_ = 44100;

    // Channel layout for the current system. Genesis is 6 FM then 4 PSG; SMS is
    // 4 PSG and no FM at all, so the PSG channels start at 0 and the YM2612 is
    // never created.
    int fmChannels_ = 6;
    int psgBase_ = 6;

    std::unique_ptr<Ym2612Chip> ym_;
    SN76489 psg_;

    // Channels whose key-on is waiting for the chip to be clocked.
    //
    // Retriggering a note means key-off then key-on, and the YM2612's envelope
    // generator only restarts when it OBSERVES that off->on edge. Issuing both
    // writes back to back, with no clock in between, leaves the register at
    // "on" and the envelope never restarts -- the note that was already sounding
    // just carries on decaying. The first note on a channel works (it is a
    // genuine off->on), every retrigger after it silently does nothing, and the
    // track fades to nothing while the sequencer runs on. Measured on ymfm: a
    // retrigger with no clock between the two writes leaves the amplitude at
    // 1029, with one clock between it returns to the full 5440.
    //
    // So the key-off goes out immediately and the key-on is deferred to after
    // the next chip clock, which render() guarantees happens every output
    // sample (the chip runs at ~53 kHz against a 44.1 kHz output). The delay is
    // one output sample; real drivers space these writes for the same reason.
    uint8_t pendingKeyOn_ = 0;

    // ymfm runs at its own native rate (clock/144); we resample to `rate_`.
    double fmStep_ = 0.0;
    double fmAcc_ = 0.0;
    float fmPrev_[2] = { 0, 0 };
    float fmCur_[2] = { 0, 0 };

    // One-pole DC blocker, standing in for the console's output coupling.
    float dcPrevIn_[2] = { 0, 0 };
    float dcPrevOut_[2] = { 0, 0 };

    // Tick scheduling.
    double samplesPerTick_ = 0.0;
    double tickAcc_ = 0.0;

    int speed1_ = 6;
    int speed2_ = 6;
    int speedPhase_ = 0; // 0 = this row uses speed1, 1 = speed2
    int tickInRow_ = 0;
    int row_ = 0;
    int order_ = 0;
    bool looped_ = false;
    bool ended_ = false;
    int globalFineTune_ = 0;
    int arpTickSpeed_ = 1; // E0xx / the pre-0x14 header field

    // Pending order/row changes requested by Bxx / Dxx during a row.
    int pendingOrder_ = -1;
    int pendingRow_ = -1;

    struct ChannelState
    {
        bool isFm = false;
        int instrument = -1;
        int volume = 0x7F;      // channel volume in that chip's own range
        int semitone = -1;      // absolute semitone, -1 == no note
        bool keyOn = false;

        // pitch
        int pitch = 0;          // current fractional pitch offset, 1/64 semitone
        int portaTarget = -1;
        int portaSpeed = 0;
        int slideSpeed = 0;     // 1xx / 2xx, signed
        // Portamento offset, in F-NUMBER units rather than semitones: the
        // manual defines 1xx/2xx/3xx as changing the FREQUENCY by xx per tick,
        // and on both of these chips the frequency register is what that means.
        // Kept separate from `pitch` (semitones) which carries arpeggio,
        // vibrato and fine tune.
        double fnumOffset = 0.0;
        double portaDelta = 0.0; // 3xx: F-number distance still to travel
        int vibPos = 0, vibSpeed = 0, vibDepth = 0, vibMode = 0;
        int arpX = 0, arpY = 0, arpPos = 0, arpTick = 0;
        int vibFine = 15; // E4xx fine vibrato depth; F == full
        int fineTune = 0;
        int noteSlideSpeed = 0, noteSlideTarget = -1;

        // volume
        int volSlide = 0;
        int tremPos = 0, tremSpeed = 0, tremDepth = 0;

        int cutTick = -1, delayTick = -1;
        int retrig = 0;
        Row delayedRow;

        // macros (STD instruments, PSG side)
        int macroVolPos = 0, macroArpPos = 0, macroDutyPos = 0;
        bool macroDone = false;

        // PSG
        int psgNoiseMode = 0;

        // FM
        uint8_t fmAlg = 0, fmFb = 0, fmAms = 0, fmFms = 0;
        uint8_t opTl[4] = { 0, 0, 0, 0 };
        uint8_t pan = 0xC0; // both speakers
    };

    std::vector<ChannelState> ch_;

    // DAC (channel 6 sample output, effect 17xx)
    bool dacEnabled_ = false;
    const Sample* dacSample_ = nullptr;
    double dacPos_ = 0.0;
    double dacStep_ = 0.0;
    int dacBank_ = 0;
};

} // namespace dmfcr
