// Sound Tracker Pro modules (.stp, and the .stp2 files that are the same
// format under another name).
//
// Sound Tracker Pro is the one format here where Bulba published a player but
// only as a SYMBOLIC listing -- players/source/stp.listing.txt, decompiled by
// VfNG/NEW in 1997 -- with no byte column to rebuild an image from and no
// assembler dialect this repo already speaks. Rather than teach the vendored
// Z80 assembler a fourth dialect for one format, the sequencer is written out
// the same way Sound Tracker's and ASC's are, following Bulba's own AY_Emul
// implementation under his attribution grant. The listing stays vendored as
// the cross-reference.
//
// Unlike the other module formats here, STP pointers are ABSOLUTE Z80
// addresses rather than offsets from the start of the file, so the module has
// to be placed in a 64K image at the address it was compiled for. That address
// is not stored anywhere -- it is recovered from the positions pointer, which
// always points just past the 10-byte header.

#include "zxay_loop.h"
#include "zxay_native.h"
#include "zxay_ticksource.h"

#include <cstring>

namespace musix::zxay {

namespace {

// Sound Tracker Pro shares Sound Tracker's note table.
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

constexpr int kHeaderSize = 10;

class StpSource : public TickSource
{
public:
    StpSource(const std::vector<uint8_t>& mod, int sampleRate)
        : machine_(sampleRate), ram_(0x10000, 0)
    {
        // Recover the address this module was compiled for. The header stores
        // absolute pointers but not the base, so the base has to be worked
        // back out of one of them: take the patterns pointer, read the first
        // entry of the patterns table (pattern 0's channel-A data address),
        // and subtract where that data sits in the FILE.
        //
        // Which is either right after the 10-byte header, or 53 bytes further
        // on when the module carries a "KSA SOFTWARE COMPILATION OF " tag at
        // +10 followed by a 25-character title -- the arrangement most of the
        // .stp2 corpus uses.
        if (mod.size() < 16) {
            return;
        }
        const uint16_t patternsPtr = mod[3] | (mod[4] << 8);
        static const char kKsaId[] = "KSA SOFTWARE COMPILATION OF ";
        constexpr size_t kKsaLen = sizeof(kKsaId) - 1;
        constexpr size_t kKsaBlock = 53; // the tag plus the 25-byte title
        const bool ksa = mod.size() >= 10 + kKsaLen &&
                         std::memcmp(mod.data() + 10, kKsaId, kKsaLen) == 0;
        const size_t dataStart = kHeaderSize + (ksa ? kKsaBlock : 0);
        if (patternsPtr >= mod.size()) {
            return;
        }
        const uint16_t firstPattern =
            mod[patternsPtr] | (mod[patternsPtr + 1] << 8);
        if (firstPattern < dataStart) {
            return;
        }
        base_ = static_cast<uint16_t>(firstPattern - dataStart);
        if (ksa) {
            titleAt_ = 38;
        }
        size_t n = std::min(mod.size(), static_cast<size_t>(0x10000 - base_));
        std::memcpy(ram_.data() + base_, mod.data(), n);
        loaded_ = n == mod.size();
    }

    // The KSA-tagged modules carry a 25-character title at +38.
    size_t titleOffset() const { return titleAt_; }

    bool start()
    {
        if (!loaded_) {
            return false;
        }
        delay_ = at(base_);
        positions_ = word(base_ + 1);
        patterns_ = word(base_ + 3);
        ornaments_ = word(base_ + 5);
        samples_ = word(base_ + 7);
        if (delay_ == 0) {
            return false;
        }
        delayCounter_ = 1;
        transposition_ = at(positions_ + 3);
        // Reset the channels BEFORE handing them their pattern addresses --
        // the other way round, `c = Channel{}` wipes the addresses straight
        // back to zero and all three channels start reading address 0.
        for (auto& c : chan_) {
            c = Channel{};
            // Both pointers start on record 0, whose first two bytes are the
            // loop point and the length.
            c.samplePointer = word(samples_);
            c.loopSamplePosition = at(c.samplePointer++);
            c.sampleLength = at(c.samplePointer++);
            c.ornamentPointer = word(ornaments_);
            c.loopOrnamentPosition = at(c.ornamentPointer++);
            c.ornamentLength = at(c.ornamentPointer++);
        }
        setPatternAddresses(at(positions_ + 2));
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
        int16_t currentTonSliding = 0;
        uint8_t note = 0, volume = 0, amplitude = 0;
        uint8_t positionInSample = 0, sampleLength = 1, loopSamplePosition = 0;
        uint8_t positionInOrnament = 0, ornamentLength = 1,
                loopOrnamentPosition = 0;
        int8_t noteSkipCounter = 0, glissade = 0;
        uint8_t numberOfNotesToSkip = 0;
        bool enabled = false, envelopeEnabled = false;
    };

    uint8_t at(size_t i) const { return i < ram_.size() ? ram_[i] : 0; }
    uint16_t word(size_t i) const { return at(i) | (at(i + 1) << 8); }

    void setPatternAddresses(uint8_t patternOffset)
    {
        for (int i = 0; i < 3; i++) {
            chan_[i].addressInPattern = word(patterns_ + patternOffset + i * 2);
        }
    }

    void interpret(Channel& c)
    {
        bool quit = false;
        do {
            uint8_t code = at(c.addressInPattern);
            if (code >= 1 && code <= 0x60) {
                c.note = code - 1;
                c.positionInSample = 0;
                c.positionInOrnament = 0;
                c.currentTonSliding = 0;
                c.enabled = true;
                quit = true;
            } else if (code <= 0x6F) {
                c.samplePointer = word(samples_ + (code - 0x61) * 2);
                c.loopSamplePosition = at(c.samplePointer++);
                c.sampleLength = at(c.samplePointer++);
            } else if (code <= 0x7F) {
                c.ornamentPointer = word(ornaments_ + (code - 0x70) * 2);
                c.loopOrnamentPosition = at(c.ornamentPointer++);
                c.ornamentLength = at(c.ornamentPointer++);
                c.envelopeEnabled = false;
                c.glissade = 0;
            } else if (code <= 0xBF) {
                c.numberOfNotesToSkip = code - 0x80;
            } else if (code <= 0xCF) {
                if (code != 0xC0) {
                    machine_.writeRegister(13, code - 0xC0);
                    c.addressInPattern++;
                    machine_.writeRegister(11, at(c.addressInPattern));
                    machine_.writeRegister(12, 0);
                }
                c.envelopeEnabled = true;
                c.loopOrnamentPosition = 0;
                c.glissade = 0;
                c.ornamentLength = 1;
            } else if (code <= 0xDF) {
                c.enabled = false;
                quit = true;
            } else if (code <= 0xEF) {
                quit = true;
            } else if (code == 0xF0) {
                c.addressInPattern++;
                c.glissade = static_cast<int8_t>(at(c.addressInPattern));
            } else {
                c.volume = code - 0xF1;
            }
            c.addressInPattern++;
        } while (!quit);
        c.noteSkipCounter = static_cast<int8_t>(c.numberOfNotesToSkip);
    }

    void channelRegisters(Channel& c, uint8_t& mixer)
    {
        if (!c.enabled) {
            mixer |= 0x48; // tone and noise both off for this channel
            c.amplitude = 0;
            mixer >>= 1;
            return;
        }
        c.currentTonSliding =
            static_cast<int16_t>(c.currentTonSliding + c.glissade);
        int j = c.note + transposition_;
        if (!c.envelopeEnabled) {
            j += at(c.ornamentPointer + c.positionInOrnament);
        }
        if (j > 95) {
            j = 95;
        }
        if (j < 0) {
            j = 0;
        }
        const size_t s = c.samplePointer + c.positionInSample * 4;
        const uint8_t b0 = at(s), b1 = at(s + 1);
        c.tone = static_cast<uint16_t>(
            (kNoteTable[j] + c.currentTonSliding + word(s + 2)) & 0xFFF);
        int amp = (b0 & 15) - c.volume;
        if (amp < 0) {
            amp = 0;
        }
        c.amplitude = static_cast<uint8_t>(amp);
        if ((b1 & 1) != 0 && c.envelopeEnabled) {
            c.amplitude |= 16;
        }
        mixer |= static_cast<uint8_t>((b0 >> 1) & 0x48);
        if (static_cast<int8_t>(b0) >= 0) {
            machine_.writeRegister(6, (b1 >> 1) & 31);
        }
        if (++c.positionInOrnament >= c.ornamentLength) {
            c.positionInOrnament = c.loopOrnamentPosition;
        }
        if (++c.positionInSample >= c.sampleLength) {
            c.positionInSample = c.loopSamplePosition;
            if (static_cast<int8_t>(c.loopSamplePosition) < 0) {
                c.enabled = false;
            }
        }
        mixer >>= 1;
    }

    void step()
    {
        if (--delayCounter_ == 0) {
            delayCounter_ = delay_;
            if (--chan_[0].noteSkipCounter < 0) {
                if (at(chan_[0].addressInPattern) == 0) {
                    if (++currentPosition_ == at(positions_)) {
                        currentPosition_ = at(positions_ + 1);
                    }
                    setPatternAddresses(
                        at(positions_ + 2 + currentPosition_ * 2));
                    transposition_ = static_cast<int8_t>(
                        at(positions_ + 3 + currentPosition_ * 2));
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
    }

    ZxAyMachine machine_;
    std::vector<uint8_t> ram_;
    LoopDetector loop_;
    Channel chan_[3];
    uint16_t base_ = 0, positions_ = 0, patterns_ = 0, ornaments_ = 0,
             samples_ = 0;
    uint8_t delay_ = 1;
    int delayCounter_ = 1;
    int currentPosition_ = 0;
    int8_t transposition_ = 0;
    size_t titleAt_ = 0;
    bool loaded_ = false;
};

} // namespace

std::unique_ptr<Source> createStpSource(const std::vector<uint8_t>& data,
                                        int sampleRate)
{
    auto src = std::make_unique<StpSource>(data, sampleRate);
    if (!src->start()) {
        return nullptr;
    }
    return src;
}

} // namespace musix::zxay
