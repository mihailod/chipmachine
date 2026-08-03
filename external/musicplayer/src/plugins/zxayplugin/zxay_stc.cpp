// Sound Tracker 1.x compiled modules (.stc, and the .zxs / .st13 files that
// are the same format under another name).
//
// This is one of the two formats in this plugin with no redistributable ZX
// replay routine to run -- Bulba published Sound Tracker's format description
// but not a player -- so the sequencer is written out. The layout comes from
// players/source/ST11FMT.txt (RAMSOFT, 1993), and the note table and the exact
// order of operations per tick follow Sergey Bulba's own AY_Emul
// implementation, used under his attribution grant (see players/PROVENANCE.md).
//
// The format is small enough to state completely: three channels, each walking
// a byte-coded pattern stream; a 32-step sample giving volume, noise period and
// a signed tone offset per step; and a 32-step ornament transposing the note.

#include "zxay_loop.h"
#include "zxay_native.h"
#include "zxay_ticksource.h"

#include <cstring>

namespace musix::zxay {

namespace {

// Tone periods for the 96 semitones, C-1 upwards. Sound Tracker's own table.
constexpr uint16_t kNoteTable[96] = {
    0xef8, 0xe10, 0xd60, 0xc80, 0xbd8, 0xb28, 0xa88, 0x9f0, 0x960, 0x8e0,
    0x858, 0x7e0, 0x77c, 0x708, 0x6b0, 0x640, 0x5ec, 0x594, 0x544, 0x4f8,
    0x4b0, 0x470, 0x42c, 0x3f0, 0x3be, 0x384, 0x358, 0x320, 0x2f6, 0x2ca,
    0x2a2, 0x27c, 0x258, 0x238, 0x216, 0x1f8, 0x1df, 0x1c2, 0x1ac, 0x190,
    0x17b, 0x165, 0x151, 0x13e, 0x12c, 0x11c, 0x10b, 0x0fc, 0x0ef, 0x0e1,
    0x0d6, 0x0c8, 0x0bd, 0x0b2, 0x0a8, 0x09f, 0x096, 0x08e, 0x085, 0x07e,
    0x077, 0x070, 0x06b, 0x064, 0x05e, 0x059, 0x054, 0x04f, 0x04b, 0x047,
    0x042, 0x03f, 0x03b, 0x038, 0x035, 0x032, 0x02f, 0x02c, 0x02a, 0x027,
    0x025, 0x023, 0x021, 0x01f, 0x01d, 0x01c, 0x01a, 0x019, 0x017, 0x016,
    0x015, 0x013, 0x012, 0x011, 0x010, 0x00f};

constexpr int kSampleSize = 0x63;   // compiled sample record
constexpr int kOrnamentSize = 0x21; // compiled ornament record
constexpr int kSamplesBase = 0x1B;

class StcSource : public TickSource
{
public:
    StcSource(std::vector<uint8_t> mod, int sampleRate)
        : machine_(sampleRate), mod_(std::move(mod))
    {
    }

    bool start()
    {
        if (mod_.size() < 32) {
            return false;
        }
        delay_ = mod_[0];
        positions_ = word(1);
        ornaments_ = word(3);
        patterns_ = word(5);
        if (delay_ == 0 || positions_ >= mod_.size() ||
            ornaments_ >= mod_.size() || patterns_ >= mod_.size()) {
            return false;
        }
        lastPosition_ = at(positions_);
        transposition_ = at(positions_ + 2);
        delayCounter_ = 1;
        for (auto& c : chan_) {
            c = Channel{};
            c.ornamentPointer = ornaments_ + 1;
            c.sampleTickCounter = -1;
        }
        return seekPattern(at(positions_ + 1));
    }

protected:
    bool advance() override
    {
        if (!step()) {
            return false;
        }
        if (loop_.addTick(machine_.registers()) || loop_.exhausted()) {
            return false;
        }
        emitTick(machine_);
        return true;
    }

private:
    struct Channel
    {
        uint16_t addressInPattern = 0;
        uint16_t samplePointer = 0;
        uint16_t ornamentPointer = 0;
        int noteSkipCounter = 0;
        int numberOfNotesToSkip = 0;
        int sampleTickCounter = -1;
        int positionInSample = 0;
        int note = 0;
        int amplitude = 0;
        int tone = 0;
        bool envelopeEnabled = false;
    };

    uint8_t at(size_t i) const { return i < mod_.size() ? mod_[i] : 0; }
    uint16_t word(size_t i) const { return at(i) | (at(i + 1) << 8); }

    // Patterns are keyed by number, not indexed, so finding one is a scan
    // through the 7-byte records until the number matches.
    bool seekPattern(uint8_t number)
    {
        for (size_t i = 0; patterns_ + 7 * i + 6 < mod_.size(); i++) {
            size_t rec = patterns_ + 7 * i;
            if (mod_[rec] == number) {
                chan_[0].addressInPattern = word(rec + 1);
                chan_[1].addressInPattern = word(rec + 3);
                chan_[2].addressInPattern = word(rec + 5);
                return true;
            }
            if (mod_[rec] == 0xFF) {
                break;
            }
        }
        return false;
    }

    // Samples and ornaments are keyed the same way.
    uint16_t findSample(uint8_t number) const
    {
        for (size_t k = 0; kSamplesBase + kSampleSize * k < mod_.size(); k++) {
            if (mod_[kSamplesBase + kSampleSize * k] == number) {
                return static_cast<uint16_t>(kSamplesBase + kSampleSize * k + 1);
            }
        }
        return kSamplesBase + 1;
    }

    uint16_t findOrnament(uint8_t number) const
    {
        for (size_t k = 0; ornaments_ + kOrnamentSize * k < mod_.size(); k++) {
            if (mod_[ornaments_ + kOrnamentSize * k] == number) {
                return static_cast<uint16_t>(ornaments_ + kOrnamentSize * k + 1);
            }
        }
        return ornaments_ + 1;
    }

    void interpret(Channel& c)
    {
        for (;;) {
            uint8_t code = at(c.addressInPattern);
            if (code < 0x60) {
                c.note = code;
                c.sampleTickCounter = 32;
                c.positionInSample = 0;
                c.addressInPattern++;
                break;
            }
            if (code < 0x70) {
                c.samplePointer = findSample(code - 0x60);
            } else if (code < 0x80) {
                c.ornamentPointer = findOrnament(code - 0x70);
                c.envelopeEnabled = false;
            } else if (code == 0x80) {
                c.sampleTickCounter = -1; // rest: shuts the channel
                c.addressInPattern++;
                break;
            } else if (code == 0x81) {
                c.addressInPattern++; // empty location
                break;
            } else if (code == 0x82) {
                c.ornamentPointer = findOrnament(0);
                c.envelopeEnabled = false;
            } else if (code <= 0x8E) {
                // Envelope shape in the code, period in the byte after it.
                machine_.writeRegister(13, code - 0x80);
                c.addressInPattern++;
                machine_.writeRegister(11, at(c.addressInPattern));
                machine_.writeRegister(12, 0);
                c.envelopeEnabled = true;
                c.ornamentPointer = findOrnament(0);
            } else {
                // 0xA1..0xE0: this many empty rows follow the next code.
                c.numberOfNotesToSkip = code - 0xA1;
            }
            c.addressInPattern++;
        }
        c.noteSkipCounter = c.numberOfNotesToSkip;
    }

    // Produces one tick of a channel's sample/ornament and folds its mixer
    // bits into `mixer`, which is shifted right after each channel so A ends
    // up in bit 0/3, B in 1/4 and C in 2/5 -- the AY's R7 layout.
    void channelRegisters(Channel& c, uint8_t& mixer)
    {
        if (c.sampleTickCounter >= 0) {
            c.sampleTickCounter--;
            c.positionInSample = (c.positionInSample + 1) & 0x1F;
            if (c.sampleTickCounter == 0) {
                uint8_t repeat = at(c.samplePointer + 0x60);
                if (repeat != 0) {
                    c.positionInSample = repeat & 0x1F;
                    c.sampleTickCounter = at(c.samplePointer + 0x61) + 1;
                } else {
                    c.sampleTickCounter = -1;
                }
            }
        }
        if (c.sampleTickCounter >= 0) {
            size_t i = ((c.positionInSample - 1) & 0x1F) * 3 + c.samplePointer;
            uint8_t b0 = at(i), b1 = at(i + 1), b2 = at(i + 2);
            if (b1 & 0x80) {
                mixer |= 64; // noise off for this channel
            } else {
                machine_.writeRegister(6, b1 & 0x1F);
            }
            if (b1 & 0x40) {
                mixer |= 8; // tone off for this channel
            }
            c.amplitude = b0 & 0x0F;
            int j = c.note + static_cast<int8_t>(
                                 at(c.ornamentPointer +
                                    ((c.positionInSample - 1) & 0x1F))) +
                    transposition_;
            if (j > 95) {
                j = 95;
            }
            if (j < 0) {
                j = 0;
            }
            int offset = b2 + ((b0 & 0xF0) << 4);
            c.tone = ((b1 & 0x20) ? kNoteTable[j] + offset
                                  : kNoteTable[j] - offset) &
                     0xFFF;
            if (c.envelopeEnabled) {
                c.amplitude |= 16;
            }
        } else {
            c.amplitude = 0;
        }
        mixer >>= 1;
    }

    bool step()
    {
        if (--delayCounter_ == 0) {
            delayCounter_ = delay_;
            // Channel A owns the position list: when its pattern data runs
            // out, the whole song advances.
            if (--chan_[0].noteSkipCounter < 0) {
                if (at(chan_[0].addressInPattern) == 0xFF) {
                    if (currentPosition_ == lastPosition_) {
                        currentPosition_ = 0;
                    } else {
                        currentPosition_++;
                    }
                    transposition_ =
                        at(positions_ + 2 + currentPosition_ * 2);
                    if (!seekPattern(at(positions_ + 1 + currentPosition_ * 2))) {
                        return false;
                    }
                }
                interpret(chan_[0]);
            }
            for (int i = 1; i < 3; i++) {
                if (--chan_[i].noteSkipCounter < 0) {
                    interpret(chan_[i]);
                }
            }
        }

        uint8_t mixer = 0;
        for (auto& c : chan_) {
            channelRegisters(c, mixer);
        }
        machine_.writeRegister(7, mixer);
        for (int i = 0; i < 3; i++) {
            machine_.writeRegister(i * 2, chan_[i].tone & 0xFF);
            machine_.writeRegister(i * 2 + 1, (chan_[i].tone >> 8) & 0x0F);
            machine_.writeRegister(8 + i, chan_[i].amplitude);
        }
        return true;
    }

    ZxAyMachine machine_;
    std::vector<uint8_t> mod_;
    LoopDetector loop_;
    Channel chan_[3];
    uint16_t positions_ = 0, ornaments_ = 0, patterns_ = 0;
    uint8_t delay_ = 1, lastPosition_ = 0;
    int delayCounter_ = 1;
    int currentPosition_ = 0;
    int8_t transposition_ = 0;
};

} // namespace

std::unique_ptr<Source> createStcSource(const std::vector<uint8_t>& data,
                                        int sampleRate)
{
    auto src = std::make_unique<StcSource>(data, sampleRate);
    if (!src->start()) {
        return nullptr;
    }
    return src;
}

} // namespace musix::zxay
