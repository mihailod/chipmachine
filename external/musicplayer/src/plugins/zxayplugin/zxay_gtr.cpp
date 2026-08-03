// Global Tracker modules (.gtr).
//
// Reclaimed from ZXTune (GPL-3), which is gated out of the App Store build.
// Bulba published Global Tracker's player only as a symbolic listing with no
// byte column (players/source/gtr.listing.txt, by Doctor Max / Global
// Corporation), so as with Sound Tracker Pro this is a sequencer written out
// following his AY_Emul implementation, under his attribution grant. See
// players/PROVENANCE.md.
//
// Two quirks worth knowing. The format has two versions, distinguished by the
// byte after the "GTR" tag, and they differ in whether selecting an ornament
// cancels the envelope and whether a "stop" code ends the row. And the player
// starts every channel pointing at a NULL sample and ornament -- the last four
// bytes of the address space, which are zero -- rather than at record 0, so a
// channel that sounds before its first sample-select command is silent rather
// than playing whatever record 0 happens to hold.
//
// Bulba's own note on the format, kept because it explains audible behaviour:
// "Global Tracker player has error with envelope (if it once is on, no any way
// to off it except in sample only). You'll can't to off it by calling init
// even. I am not fix it bug in AY Emulator for compatibility." Neither do we.

#include "zxay_loop.h"
#include "zxay_native.h"
#include "zxay_ticksource.h"

#include <cstring>

namespace musix::zxay {

namespace {

// Global Tracker uses Pro Tracker 3's Sound Tracker note table.
constexpr uint16_t kNoteTable[96] = {
    0x0EF8, 0x0E10, 0x0D60, 0x0C80, 0x0BD8, 0x0B28, 0x0A88, 0x09F0, 0x0960,
    0x08E0, 0x0858, 0x07E0, 0x077C, 0x0708, 0x06B0, 0x0640, 0x05EC, 0x0594,
    0x0544, 0x04F8, 0x04B0, 0x0470, 0x042C, 0x03FD, 0x03BE, 0x0384, 0x0358,
    0x0320, 0x02F6, 0x02CA, 0x02A2, 0x027C, 0x0258, 0x0238, 0x0216, 0x01F8,
    0x01DF, 0x01C2, 0x01AC, 0x0190, 0x017B, 0x0165, 0x0151, 0x013E, 0x012C,
    0x011C, 0x010A, 0x00FC, 0x00EF, 0x00E1, 0x00D6, 0x00C8, 0x00BD, 0x00B2,
    0x00A8, 0x009F, 0x0096, 0x008E, 0x0085, 0x007E, 0x0077, 0x0070, 0x006B,
    0x0064, 0x005E, 0x0059, 0x0054, 0x004F, 0x004B, 0x0047, 0x0042, 0x003F,
    0x003B, 0x0038, 0x0035, 0x0032, 0x002F, 0x002C, 0x002A, 0x0027, 0x0025,
    0x0023, 0x0021, 0x001F, 0x001D, 0x001C, 0x001A, 0x0019, 0x0017, 0x0016,
    0x0015, 0x0013, 0x0012, 0x0011, 0x0010, 0x000F};

constexpr uint16_t kSamplesTable = 0x27;
constexpr uint16_t kOrnamentsTable = 0x45;
constexpr uint16_t kPatternsTable = 0x65;
constexpr uint16_t kNumberOfPositions = 0x125;
constexpr uint16_t kLoopPosition = 0x126;
constexpr uint16_t kPositions = 0x127;
// The player's "no sample / no ornament" sentinel: the top of the address
// space, which is always zero.
constexpr uint16_t kNullRecord = 0x10000 - 4;

class GtrSource : public TickSource
{
public:
    GtrSource(const std::vector<uint8_t>& mod, int sampleRate)
        : machine_(sampleRate), ram_(0x10000, 0)
    {
        // Pointers are offsets from the module start, so it simply loads at 0.
        std::memcpy(ram_.data(), mod.data(),
                    std::min(mod.size(), static_cast<size_t>(0x10000)));
        loaded_ = !mod.empty();
    }

    bool start()
    {
        if (!loaded_) {
            return false;
        }
        delay_ = at(0);
        version_ = at(4);
        numberOfPositions_ = at(kNumberOfPositions);
        loopPosition_ = at(kLoopPosition);
        if (delay_ == 0 || numberOfPositions_ == 0) {
            return false;
        }
        for (auto& c : chan_) {
            c = Channel{};
            c.samplePointer = kNullRecord;
            c.sampleLength = 4;
            c.ornamentPointer = kNullRecord;
            c.ornamentLength = 1;
            c.enabled = true;
        }
        setPatternAddresses(at(kPositions));
        delayCounter_ = 1;
        return true;
    }

protected:
    bool advance() override
    {
        step();
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
        uint16_t samplePointer = 0, ornamentPointer = 0;
        uint16_t tone = 0;
        uint8_t note = 0, volume = 0, amplitude = 0;
        uint8_t positionInSample = 0, sampleLength = 4, loopSamplePosition = 0;
        uint8_t positionInOrnament = 0, ornamentLength = 1,
                loopOrnamentPosition = 0;
        int8_t noteSkipCounter = 0;
        bool enabled = false, envelopeEnabled = false;
    };

    uint8_t at(size_t i) const { return i < ram_.size() ? ram_[i] : 0; }
    uint16_t word(size_t i) const { return at(i) | (at(i + 1) << 8); }

    // Positions store the pattern's byte offset into the 6-byte pattern table.
    void setPatternAddresses(uint8_t position)
    {
        const size_t rec = kPatternsTable + (position / 6) * 6;
        for (int i = 0; i < 3; i++) {
            chan_[i].addressInPattern = word(rec + i * 2);
        }
    }

    void interpret(Channel& c)
    {
        c.noteSkipCounter = 0;
        for (;;) {
            const uint8_t code = at(c.addressInPattern);
            if (code < 0x60) {
                c.note = code;
                c.positionInSample = 0;
                c.positionInOrnament = 0;
                c.enabled = true;
                c.addressInPattern++;
                return;
            }
            if (code < 0x70) {
                c.samplePointer = word(kSamplesTable + (code - 0x60) * 2);
                c.loopSamplePosition = at(c.samplePointer++);
                c.sampleLength = at(c.samplePointer++);
            } else if (code < 0x80) {
                c.ornamentPointer = word(kOrnamentsTable + (code - 0x70) * 2);
                c.loopOrnamentPosition = at(c.ornamentPointer++);
                c.ornamentLength = at(c.ornamentPointer++);
                c.positionInOrnament = 0;
                if (version_ != 0x10) {
                    c.envelopeEnabled = false;
                }
            } else if (code < 0xC0) {
                c.noteSkipCounter = static_cast<int8_t>(code - 0x80);
            } else if (code < 0xD0) {
                machine_.writeRegister(13, code - 0xC0);
                c.addressInPattern++;
                machine_.writeRegister(11, at(c.addressInPattern));
                machine_.writeRegister(12, 0);
                c.envelopeEnabled = true;
            } else if (code < 0xE0) {
                c.addressInPattern++;
                return;
            } else if (code == 0xE0) {
                c.enabled = false;
                if (version_ != 0x10) {
                    c.addressInPattern++;
                    return;
                }
            } else if (code <= 0xEF) {
                c.volume = static_cast<uint8_t>(15 - (code - 0xE0));
            }
            c.addressInPattern++;
        }
    }

    void channelRegisters(Channel& c, uint8_t& mixer, uint8_t& noise)
    {
        if (!c.enabled) {
            c.amplitude = 0;
            mixer |= 8 | 64;
            mixer >>= 1;
            return;
        }
        int j = c.note + at(c.ornamentPointer + c.positionInOrnament);
        if (j > 0x5F) {
            j = 0x5F;
        }
        if (++c.positionInOrnament == c.ornamentLength) {
            c.positionInOrnament = c.loopOrnamentPosition;
        }
        const size_t s = c.samplePointer + c.positionInSample;
        c.tone = static_cast<uint16_t>((kNoteTable[j] + word(s + 2)) & 0xFFF);
        const uint8_t b = at(s + 1);
        noise = static_cast<uint8_t>((noise | b) & 0x1F);
        int amp = at(s) - c.volume;
        if (amp < 0) {
            amp = 0;
        }
        c.amplitude = static_cast<uint8_t>(amp & 0x0F);
        if (static_cast<int8_t>(b) < 0 && c.envelopeEnabled) {
            c.amplitude |= 16;
        }
        if (b & 64) {
            mixer |= 64;
        }
        if (b & 32) {
            mixer |= 8;
        }
        c.positionInSample += 4;
        if (c.positionInSample == c.sampleLength) {
            c.positionInSample = c.loopSamplePosition;
        }
        mixer >>= 1;
    }

    void step()
    {
        if (--delayCounter_ == 0) {
            delayCounter_ = delay_;
            if (--chan_[0].noteSkipCounter < 0) {
                // A `while` rather than an `if`: an empty pattern would
                // otherwise leave the position list stuck on it for a tick.
                int guard = 0;
                while (at(chan_[0].addressInPattern) == 255 && guard++ < 256) {
                    if (++currentPosition_ == numberOfPositions_) {
                        currentPosition_ = loopPosition_;
                    }
                    setPatternAddresses(at(kPositions + currentPosition_));
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
        uint8_t noise = 0;
        for (auto& c : chan_) {
            channelRegisters(c, mixer, noise);
        }
        machine_.writeRegister(6, noise);
        machine_.writeRegister(7, mixer);
        for (int i = 0; i < 3; i++) {
            machine_.writeRegister(i * 2, chan_[i].tone & 0xFF);
            machine_.writeRegister(i * 2 + 1, (chan_[i].tone >> 8) & 0x0F);
            machine_.writeRegister(8 + i, chan_[i].amplitude);
        }
    }

    ZxAyMachine machine_;
    std::vector<uint8_t> ram_;
    LoopDetector loop_;
    Channel chan_[3];
    uint8_t delay_ = 1, version_ = 0x10;
    uint8_t numberOfPositions_ = 0, loopPosition_ = 0;
    int delayCounter_ = 1;
    int currentPosition_ = 0;
    bool loaded_ = false;
};

} // namespace

std::unique_ptr<Source> createGtrSource(const std::vector<uint8_t>& data,
                                        int sampleRate)
{
    auto src = std::make_unique<GtrSource>(data, sampleRate);
    if (!src->start()) {
        return nullptr;
    }
    return src;
}

} // namespace musix::zxay
