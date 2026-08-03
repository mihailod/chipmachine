// Pro Sound Maker modules (.psm).
//
// Reclaimed from ZXTune (GPL-3), which is gated out of the App Store build.
// Bulba published no ZX replay routine for Pro Sound Maker, so this is a
// sequencer written out following his AY_Emul implementation, under his
// attribution grant -- the same arrangement as .stc/.asc/.stp/.sqt. See
// players/PROVENANCE.md.
//
// Pro Sound Maker is the odd one out among the formats here in three ways.
// Its pattern streams carry a CALL/RETURN (0xF9 jumps to a subroutine and
// comes back after a repeat count), its notes are stored as DESCENDING
// offsets from a running transposition rather than as absolute pitches, and
// both its sample and its ornament run on packed 5-bit counters with their own
// loop and volume-ramp state. Its position list is walked by channel C, which
// also owns the per-pattern tempo.
//
// The four header words are offsets from the start of the module, so unlike
// Sound Tracker Pro and SQ-Tracker there is no compiled base address to
// recover.

#include "zxay_loop.h"
#include "zxay_native.h"
#include "zxay_ticksource.h"

#include <cstring>

namespace musix::zxay {

namespace {

constexpr uint16_t kNoteTable[96] = {
    0xD3D, 0xC7F, 0xBCB, 0xB22, 0xA82, 0x9EB, 0x95D, 0x8D6, 0x857, 0x7DF,
    0x76E, 0x703, 0x69F, 0x63F, 0x5E6, 0x591, 0x541, 0x4F6, 0x4AE, 0x46B,
    0x42C, 0x3F0, 0x3B7, 0x382, 0x34F, 0x320, 0x2F3, 0x2C8, 0x2A1, 0x27B,
    0x257, 0x236, 0x216, 0x1F8, 0x1DC, 0x1C1, 0x1A8, 0x190, 0x179, 0x164,
    0x150, 0x13D, 0x12C, 0x11B, 0x10B, 0x0FC, 0x0EE, 0x0E0, 0x0D4, 0x0C8,
    0x0BD, 0x0B2, 0x0A8, 0x09F, 0x096, 0x08D, 0x085, 0x07E, 0x077, 0x070,
    0x06A, 0x064, 0x05E, 0x059, 0x054, 0x04F, 0x04B, 0x047, 0x043, 0x03F,
    0x03B, 0x038, 0x035, 0x032, 0x02F, 0x02D, 0x02A, 0x028, 0x025, 0x023,
    0x021, 0x01F, 0x01E, 0x01C, 0x01A, 0x019, 0x018, 0x016, 0x015, 0x014,
    0x013, 0x012, 0x011, 0x010, 0x00F, 0x00E};

class PsmSource : public TickSource
{
public:
    PsmSource(std::vector<uint8_t> mod, int sampleRate)
        : machine_(sampleRate), mod_(std::move(mod))
    {
    }

    bool start()
    {
        if (mod_.size() < 16) {
            return false;
        }
        positions_ = word(0);
        samples_ = word(2);
        ornaments_ = word(4);
        patterns_ = word(6);
        if (positions_ < 8 || positions_ >= mod_.size() ||
            samples_ >= mod_.size() || ornaments_ >= mod_.size() ||
            patterns_ >= mod_.size()) {
            return false;
        }
        const uint8_t pat = at(positions_);
        transposition_ = static_cast<uint8_t>(at(positions_ + 1) + 48);
        delay_ = at(patterns_ + pat * 7);
        for (int i = 0; i < 3; i++) {
            chan_[i] = Channel{};
            chan_[i].addressInPattern = word(patterns_ + pat * 7 + 1 + i * 2);
            chan_[i].noteSkipCounter = 1;
            chan_[i].note = -128;
        }
        delayCounter_ = 1;
        return delay_ != 0;
    }

protected:
    bool advance() override
    {
        if (!step() || loop_.addTick(machine_.registers()) ||
            loop_.exhausted()) {
            return false;
        }
        emitTick(machine_);
        return true;
    }

private:
    struct Channel
    {
        uint16_t addressInPattern = 0, retAddress = 0;
        uint16_t divShift = 0, tone = 0;
        uint8_t numberOfNotesToSkip = 0, noteSkipCounter = 0;
        uint8_t amplitude = 0, retCnt = 0, vol = 0, volCnt = 0, loopCnt = 0;
        uint8_t orn = 0, envType = 0, envDiv = 0, samp = 0;
        int8_t ornTick = 0, smpTick = 0, note = 0;
    };

    uint8_t at(size_t i) const { return i < mod_.size() ? mod_[i] : 0; }
    uint16_t word(size_t i) const { return at(i) | (at(i + 1) << 8); }

    void setEnvelopePeriod(uint16_t v)
    {
        machine_.writeRegister(11, v & 0xFF);
        machine_.writeRegister(12, v >> 8);
    }

    // The envelope shape/period a note carries when the "ornament" slot is
    // really an envelope selector (orn >= 33).
    void applyEnvelope(Channel& c)
    {
        if (c.envType >= 0xB1) {
            machine_.writeRegister(13, c.envType - 0xB1 + 8);
            setEnvelopePeriod(c.envDiv >= 0xF1
                                  ? static_cast<uint16_t>((c.envDiv & 15) << 8)
                                  : c.envDiv);
            c.ornTick = static_cast<int8_t>(c.ornTick | 0x40);
        } else {
            uint8_t b = static_cast<uint8_t>(c.envType - 0xA1);
            machine_.writeRegister(13, ((b & 3) << 1) | 8);
            b = static_cast<uint8_t>((b & 12) * 3 + c.note);
            if (b >= 48) {
                b -= 48;
                if (b >= 48) {
                    b -= 48;
                }
            }
            setEnvelopePeriod(kNoteTable[b + 48]);
        }
    }

    void interpret(Channel& c)
    {
        uint16_t at_ = c.addressInPattern;
        if (c.retCnt != 0 && --c.retCnt == 0) {
            at_ = c.retAddress;
        }
        for (;;) {
            const uint8_t code = at(at_);
            if (code < 0x60) {
                // Notes descend from the running transposition.
                if (c.note < 0) {
                    c.note = static_cast<int8_t>(transposition_ - code);
                } else {
                    c.note = static_cast<int8_t>(c.note - code);
                }
                if (c.note < 0) {
                    c.note = static_cast<int8_t>(c.note + 96);
                }
                c.volCnt = c.vol;
                c.smpTick = 0;
                c.divShift = 0;
                c.loopCnt = 1;
                c.ornTick = static_cast<int8_t>(
                    c.ornTick < 0 ? (c.ornTick & 0xE0) : (c.ornTick & 0xC0));
                if ((c.ornTick & 0x40) != 0 && c.orn >= 33) {
                    applyEnvelope(c);
                }
                at_++;
                break;
            }
            if (code == 0x60) {
                c.smpTick = static_cast<int8_t>(c.smpTick | 128);
                at_++;
                break;
            }
            if (code <= 0x6F) {
                c.samp = code - 0x61;
            } else if (code <= 0x8F) {
                c.orn = code - 0x70;
                c.ornTick = 0;
            } else if (code == 0x90) {
                at_++;
                break;
            } else if (code <= 0x9F) {
                c.vol = code - 0x90;
            } else if (code == 0xA0) {
                c.ornTick = static_cast<int8_t>(code);
            } else if (code <= 0xB0) {
                c.orn = 33;
                c.envType = code;
                c.ornTick = static_cast<int8_t>(c.ornTick | 0x40);
            } else if (code <= 0xB7) {
                c.envType = code;
                at_++;
                c.envDiv = at(at_);
                machine_.writeRegister(13, c.envType - 0xB1 + 8);
                setEnvelopePeriod(c.envDiv >= 0xF1
                                      ? static_cast<uint16_t>((c.envDiv & 15) << 8)
                                      : c.envDiv);
                c.ornTick = static_cast<int8_t>(c.ornTick | 0x40);
            } else if (code <= 0xF8) {
                c.numberOfNotesToSkip = code - 0xB7;
            } else if (code == 0xF9) {
                // Call: jump away, come back after `retCnt` rows.
                c.retAddress = static_cast<uint16_t>(at_ + 4);
                c.retCnt = at(static_cast<uint16_t>(at_ + 3));
                at_ = static_cast<uint16_t>(word(at_ + 1) - 1);
            } else if (code <= 0xFB) {
                c.orn = static_cast<uint8_t>(code - 0xFA + 32);
            } else {
                at_++;
                break;
            }
            at_++;
        }
        c.addressInPattern = at_;
        c.noteSkipCounter = c.numberOfNotesToSkip;
    }

    void channelRegisters(Channel& c, uint8_t& mixer)
    {
        uint8_t b = static_cast<uint8_t>(c.note & 127);
        const uint16_t wo = word(ornaments_ + c.orn * 2);
        if ((c.ornTick & 0x60) == 0) {
            b = static_cast<uint8_t>(b + at(wo + 2 + (c.ornTick & 0xFF)));
        }
        if (static_cast<int8_t>(b) < 0) {
            b = 0;
        } else if (b > 95) {
            b = 95;
        }
        int tone = kNoteTable[b];

        const uint16_t ws = word(samples_ + c.samp * 2);
        const size_t s = ws + 2 + static_cast<size_t>(c.smpTick & 0xFF) * 3;
        const uint8_t s0 = at(s), s1 = at(s + 1), s2 = at(s + 2);

        // An 11-bit signed tone slide accumulated per tick.
        uint16_t w = static_cast<uint16_t>(((s1 & 7) << 8) + s2);
        if ((s1 & 4) != 0) {
            w |= 0xF800;
        }
        c.divShift = static_cast<uint16_t>(c.divShift + w);
        tone = static_cast<int16_t>(tone + c.divShift);
        if (tone < 0) {
            tone = 0;
        } else if (tone >= 4096) {
            tone = 4095;
        }
        c.tone = static_cast<uint16_t>(tone);

        uint8_t amp = s0 & 15;
        if ((c.ornTick & 0x40) != 0) {
            amp |= 16;
        }
        amp = static_cast<uint8_t>(amp + c.volCnt - 15);
        if (static_cast<int8_t>(amp) < 0 || c.smpTick < 0) {
            amp = 0;
        }
        c.amplitude = amp;

        mixer |= static_cast<uint8_t>(((s0 >> 1) & 0x48));
        if (c.smpTick < 0) {
            mixer |= 0x40;
        }
        if (static_cast<int8_t>(s0) >= 0 && c.amplitude != 0) {
            machine_.writeRegister(6, s1 >> 3);
        }

        // Sample counter: 5 bits of position, with a loop point, a repeat
        // count and a volume ramp packed into the two header bytes.
        b = static_cast<uint8_t>((c.smpTick & 31) + 1);
        const uint8_t h0 = at(ws), h1 = at(ws + 1);
        if (b > (h0 & 31)) {
            if ((h1 & 0xE0) == 0) {
                c.smpTick = static_cast<int8_t>(c.smpTick | 128);
            } else {
                b = h1 & 31;
                if (--c.loopCnt == 0) {
                    c.loopCnt = h1 >> 5;
                    if ((h0 & 0x20) == 0) {
                        c.volCnt = static_cast<uint8_t>(c.volCnt + (h0 >> 6));
                    } else {
                        c.volCnt =
                            static_cast<uint8_t>(c.volCnt - ((h0 >> 6) + 1));
                    }
                    if (static_cast<int8_t>(c.volCnt) < 0) {
                        c.volCnt = 0;
                    } else if (c.volCnt > 15) {
                        c.volCnt = 15;
                    }
                }
            }
        }
        c.smpTick = static_cast<int8_t>(
            ((b ^ static_cast<uint8_t>(c.smpTick)) & 31) ^
            static_cast<uint8_t>(c.smpTick));

        b = static_cast<uint8_t>((c.ornTick & 31) + 1);
        const uint8_t o0 = at(wo), o1 = at(wo + 1);
        if (b > o0) {
            if (static_cast<int8_t>(o1) < 0) {
                b = o1;
            } else {
                c.ornTick = static_cast<int8_t>(c.ornTick | 0x20);
            }
        }
        c.ornTick = static_cast<int8_t>(
            ((b ^ static_cast<uint8_t>(c.ornTick)) & 31) ^
            static_cast<uint8_t>(c.ornTick));

        mixer >>= 1;
    }

    // Returns false at end of song (Pro Sound Maker's position list has an
    // explicit terminator, so unlike the others this format really does end).
    bool step()
    {
        for (int i = 0; i < 3; i++) {
            machine_.writeRegister(8 + i, 0);
        }
        if (--delayCounter_ == 0) {
            // Channel C drives the position list and the tempo.
            if (--chan_[2].noteSkipCounter == 0) {
                if (at(chan_[2].addressInPattern) == 255) {
                    currentPosition_++;
                    uint8_t b = at(positions_ + currentPosition_ * 2);
                    if (b == 255) {
                        b = at(positions_ + currentPosition_ * 2 + 1);
                        if (b == 255) {
                            return false; // end of song
                        }
                        currentPosition_ = b;
                        b = at(positions_ + b * 2);
                    }
                    transposition_ = static_cast<uint8_t>(
                        at(positions_ + currentPosition_ * 2 + 1) + 48);
                    delay_ = at(patterns_ + b * 7);
                    for (int i = 0; i < 3; i++) {
                        chan_[i].addressInPattern =
                            word(patterns_ + b * 7 + 1 + i * 2);
                        chan_[i].retCnt = 0;
                        chan_[i].note =
                            static_cast<int8_t>(chan_[i].note | -128);
                    }
                    chan_[0].noteSkipCounter = 1;
                    chan_[1].noteSkipCounter = 1;
                }
                interpret(chan_[2]);
            }
            for (int i = 1; i >= 0; i--) {
                if (--chan_[i].noteSkipCounter == 0) {
                    interpret(chan_[i]);
                }
            }
            delayCounter_ = delay_;
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
    uint16_t positions_ = 0, samples_ = 0, ornaments_ = 0, patterns_ = 0;
    uint8_t delay_ = 1, transposition_ = 0;
    int delayCounter_ = 1;
    int currentPosition_ = 0;
};

} // namespace

std::unique_ptr<Source> createPsmSource(const std::vector<uint8_t>& data,
                                        int sampleRate)
{
    auto src = std::make_unique<PsmSource>(data, sampleRate);
    if (!src->start()) {
        return nullptr;
    }
    return src;
}

} // namespace musix::zxay
