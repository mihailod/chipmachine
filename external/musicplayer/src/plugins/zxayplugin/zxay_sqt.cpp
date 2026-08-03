// SQ-Tracker modules (.sqt).
//
// Bulba published SQ-Tracker's original replay routine as a MONS4D
// disassembly, and it rebuilds cleanly (players/sqt_bin.h) -- but it cannot be
// used. The routine is not relocatable and assembles at #C000, while SQT
// modules are themselves absolute and compiled to sit at roughly #C400-#C500,
// straight through the player. There is nowhere to put both.
//
// So this is the fourth native sequencer, following Bulba's AY_Emul under his
// attribution grant. Like Sound Tracker Pro it needs the module placed at its
// compiled address in a 64K image, which is recoverable because the samples
// pointer always points 10 bytes past the module's start.
//
// SQ-Tracker is unusual in two ways worth knowing when reading this. Its
// pattern streams are shared rather than per-channel -- a "line" opcode can
// carry a nested command block that sets volume or tempo for ALL three
// channels at once -- and its position list is walked per channel, C first,
// each entry carrying its own volume and transposition.

#include "zxay_loop.h"
#include "zxay_native.h"
#include "zxay_ticksource.h"

#include <cstring>

namespace musix::zxay {

namespace {

constexpr uint16_t kNoteTable[0x60] = {
    0xd5d, 0xc9c, 0xbe7, 0xb3c, 0xa9b, 0xa02, 0x973, 0x8eb, 0x86b, 0x7f2,
    0x780, 0x714, 0x6ae, 0x64e, 0x5f4, 0x59e, 0x54f, 0x501, 0x4b9, 0x475,
    0x435, 0x3f9, 0x3c0, 0x38a, 0x357, 0x327, 0x2fa, 0x2cf, 0x2a7, 0x281,
    0x25d, 0x23b, 0x21b, 0x1fc, 0x1e0, 0x1c5, 0x1ac, 0x194, 0x17d, 0x168,
    0x153, 0x140, 0x12e, 0x11d, 0x10d, 0x0fe, 0x0f0, 0x0e2, 0x0d6, 0x0ca,
    0x0be, 0x0b4, 0x0aa, 0x0a0, 0x097, 0x08f, 0x087, 0x07f, 0x078, 0x071,
    0x06b, 0x065, 0x05f, 0x05a, 0x055, 0x050, 0x04c, 0x047, 0x043, 0x040,
    0x03c, 0x039, 0x035, 0x032, 0x030, 0x02d, 0x02a, 0x028, 0x026, 0x024,
    0x022, 0x020, 0x01e, 0x01c, 0x01b, 0x019, 0x018, 0x016, 0x015, 0x014,
    0x013, 0x012, 0x011, 0x010, 0x00f, 0x00e};

class SqtSource : public TickSource
{
public:
    SqtSource(const std::vector<uint8_t>& mod, int sampleRate)
        : machine_(sampleRate), ram_(0x10000, 0)
    {
        if (mod.size() < 16) {
            return;
        }
        const uint16_t samples = mod[2] | (mod[3] << 8);
        if (samples < 10) {
            return;
        }
        base_ = static_cast<uint16_t>(samples - 10);
        size_t n = std::min(mod.size(), static_cast<size_t>(0x10000 - base_));
        std::memcpy(ram_.data() + base_, mod.data(), n);
        loaded_ = n == mod.size();
    }

    bool start()
    {
        if (!loaded_) {
            return false;
        }
        samples_ = word(base_ + 2);
        ornaments_ = word(base_ + 4);
        patterns_ = word(base_ + 6);
        positionsPointer_ = word(base_ + 8);
        loopPointer_ = word(base_ + 10);
        if (positionsPointer_ == 0 || patterns_ == 0) {
            return false;
        }
        delay_ = 1;
        delayCounter_ = 1;
        linesCounter_ = 1;
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
        uint16_t samplePointer = 0, pointInSample = 0;
        uint16_t ornamentPointer = 0, pointInOrnament = 0;
        uint16_t tone = 0;
        uint16_t lineStart = 0; // the "ix27" re-entry point for a held line
        int16_t toneSlideStep = 0, currentTonSliding = 0;
        uint8_t volume = 0, amplitude = 0, note = 0;
        uint8_t holdCounter = 0; // "ix21": lines still to hold
        int8_t sampleTickCounter = 0, ornamentTickCounter = 0;
        int8_t transposition = 0;
        bool enabled = false, envelopeEnabled = false, ornamentEnabled = false;
        bool gliss = false, mixNoise = false, mixTon = false;
        // Command blocks apply to this channel only when the position entry
        // said so ("b4ix0"); a held line re-runs its commands ("b7ix0"); and
        // the pattern pointer only advances while the line is being read for
        // the first time ("b6ix0").
        bool commandsApply = false, replayLine = false, advancePointer = false;
    };

    uint8_t at(size_t i) const { return i < ram_.size() ? ram_[i] : 0; }
    uint16_t word(size_t i) const { return at(i) | (at(i + 1) << 8); }

    // --- the nested command block ------------------------------------------
    void command(Channel& c, uint16_t& ptr, uint8_t a)
    {
        ptr++;
        if (c.advancePointer) {
            c.addressInPattern = ptr + 1;
            c.advancePointer = false;
        }
        const uint8_t v = at(ptr);
        switch (a - 1) {
        case 0:
            if (c.commandsApply) {
                c.volume = v & 15;
            }
            break;
        case 1:
            if (c.commandsApply) {
                c.volume = (c.volume + v) & 15;
            }
            break;
        case 2:
            if (c.commandsApply) {
                for (auto& ch : chan_) {
                    ch.volume = v;
                }
            }
            break;
        case 3:
            if (c.commandsApply) {
                for (auto& ch : chan_) {
                    ch.volume = (ch.volume + v) & 15;
                }
            }
            break;
        case 4:
            if (c.commandsApply) {
                delayCounter_ = v & 31;
                if (delayCounter_ == 0) {
                    delayCounter_ = 32;
                }
                delay_ = delayCounter_;
            }
            break;
        case 5:
            if (c.commandsApply) {
                delayCounter_ = (delayCounter_ + v) & 31;
                if (delayCounter_ == 0) {
                    delayCounter_ = 32;
                }
                delay_ = delayCounter_;
            }
            break;
        case 6:
            c.currentTonSliding = 0;
            c.gliss = true;
            c.toneSlideStep = static_cast<int16_t>(-v);
            break;
        case 7:
            c.currentTonSliding = 0;
            c.gliss = true;
            c.toneSlideStep = static_cast<int16_t>(v);
            break;
        default:
            c.envelopeEnabled = true;
            machine_.writeRegister(13, (a - 1) & 15);
            machine_.writeRegister(11, v);
            machine_.writeRegister(12, 0);
            break;
        }
    }

    void startSample(Channel& c, uint8_t index)
    {
        c.envelopeEnabled = false;
        c.ornamentEnabled = false;
        c.gliss = false;
        c.enabled = true;
        c.samplePointer = word(index * 2 + samples_);
        c.pointInSample = c.samplePointer + 2;
        c.sampleTickCounter = 32;
        c.mixNoise = true;
        c.mixTon = true;
    }

    void startOrnament(Channel& c, uint8_t index)
    {
        c.ornamentPointer = word(index * 2 + ornaments_);
        c.pointInOrnament = c.ornamentPointer + 2;
        c.ornamentTickCounter = 32;
        c.ornamentEnabled = true;
    }

    // The byte (or pair of bytes) that follows a note: sample select, ornament
    // select, and an optional command.
    void noteTail(Channel& c, uint16_t& ptr)
    {
        const uint8_t v = at(ptr);
        if (v < 0x80) {
            command(c, ptr, v);
        } else {
            if (((v >> 1) & 31) != 0) {
                startSample(c, (v >> 1) & 31);
            }
            if (v & 64) {
                int orn = at(ptr + 1) >> 4;
                if (v & 1) {
                    orn |= 16;
                }
                if (orn != 0) {
                    startOrnament(c, static_cast<uint8_t>(orn));
                }
                ptr++;
                if ((at(ptr) & 15) != 0) {
                    command(c, ptr, at(ptr) & 15);
                }
            }
        }
        ptr++;
    }

    // Re-runs the current line's tail without advancing the pattern pointer --
    // what a "hold this line for N rows" opcode does each row.
    void replayLine(Channel& c)
    {
        uint16_t ptr = c.lineStart;
        c.advancePointer = false;
        const uint8_t v = at(ptr);
        if (v < 0x80) {
            ptr++;
            noteTail(c, ptr);
        } else {
            startSample(c, v & 31);
        }
    }

    void interpret(Channel& c)
    {
        if (c.holdCounter != 0) {
            c.holdCounter--;
            if (c.replayLine) {
                replayLine(c);
            }
            return;
        }
        uint16_t ptr = c.addressInPattern;
        c.advancePointer = true;
        c.replayLine = false;
        for (;;) {
            const uint8_t v = at(ptr);
            if (v < 0x60) {
                c.note = v;
                c.lineStart = ptr;
                ptr++;
                noteTail(c, ptr);
                if (c.advancePointer) {
                    c.addressInPattern = ptr;
                }
                return;
            }
            if (v <= 0x6E) {
                command(c, ptr, v - 0x60);
                return;
            }
            if (v <= 0x7F) {
                c.mixNoise = false;
                c.mixTon = false;
                c.enabled = false;
                if (v != 0x6F) {
                    command(c, ptr, v - 0x6F);
                } else {
                    c.addressInPattern = ptr + 1;
                }
                return;
            }
            if (v <= 0xBF) {
                c.addressInPattern = ptr + 1;
                if (v <= 0x9F) {
                    // Transpose the held note up or down.
                    if ((v & 16) == 0) {
                        c.note = static_cast<uint8_t>(c.note + (v & 15));
                    } else {
                        c.note = static_cast<uint8_t>(c.note - (v & 15));
                    }
                } else {
                    c.holdCounter = v & 15;
                    if ((v & 16) == 0) {
                        return;
                    }
                    if (c.holdCounter != 0) {
                        c.replayLine = true;
                    }
                }
                replayLine(c);
                return;
            }
            // 0xC0..0xFF: select a sample and hold.
            c.addressInPattern = ptr + 1;
            c.lineStart = ptr;
            startSample(c, v & 31);
            return;
        }
    }

    void channelRegisters(Channel& c, uint8_t& mixer)
    {
        mixer <<= 1;
        if (!c.enabled) {
            c.amplitude = 0;
            return;
        }
        const uint8_t b0 = at(c.pointInSample);
        int amp = b0 & 15;
        if (amp != 0) {
            amp -= c.volume;
            if (amp < 0) {
                amp = 0;
            }
        } else if (c.envelopeEnabled) {
            amp = 16;
        }
        c.amplitude = static_cast<uint8_t>(amp);

        const uint8_t b1 = at(c.pointInSample + 1);
        if (b1 & 32) {
            mixer |= 8;
            int noise = (b0 & 0xF0) >> 3;
            if (static_cast<int8_t>(b1) < 0) {
                noise++;
            }
            machine_.writeRegister(6, noise & 0x1F);
        }
        if (b1 & 64) {
            mixer |= 1;
        }

        int j = c.note;
        if (c.ornamentEnabled) {
            j += at(c.pointInOrnament);
            if (--c.ornamentTickCounter == 0) {
                // An ornament of length 32 borrows its loop from the sample.
                if (at(c.ornamentPointer) != 32) {
                    c.ornamentTickCounter =
                        static_cast<int8_t>(at(c.ornamentPointer + 1));
                    c.pointInOrnament = static_cast<uint16_t>(
                        c.ornamentPointer + 2 + at(c.ornamentPointer));
                } else {
                    c.ornamentTickCounter =
                        static_cast<int8_t>(at(c.samplePointer + 1));
                    c.pointInOrnament = static_cast<uint16_t>(
                        c.ornamentPointer + 2 + at(c.samplePointer));
                }
            } else {
                c.pointInOrnament++;
            }
        }
        j += c.transposition;
        if (j > 0x5F) {
            j = 0x5F;
        }
        if (j < 0) {
            j = 0;
        }
        const int bend = ((b1 & 15) << 8) + at(c.pointInSample + 2);
        int tone = (b1 & 16) == 0 ? kNoteTable[j] - bend : kNoteTable[j] + bend;

        if (--c.sampleTickCounter == 0) {
            c.sampleTickCounter = static_cast<int8_t>(at(c.samplePointer + 1));
            if (at(c.samplePointer) == 32) {
                c.enabled = false;
                c.ornamentEnabled = false;
            }
            c.pointInSample = static_cast<uint16_t>(
                c.samplePointer + 2 + at(c.samplePointer) * 3);
        } else {
            c.pointInSample += 3;
        }

        if (c.gliss) {
            tone += c.currentTonSliding;
            c.currentTonSliding =
                static_cast<int16_t>(c.currentTonSliding + c.toneSlideStep);
        }
        c.tone = static_cast<uint16_t>(tone & 0xFFF);
    }

    // Pulls one entry off the position list for `c`. Entries are two bytes:
    // pattern number (with the top bit saying whether shared commands apply to
    // this channel) then packed volume and transposition.
    void nextPosition(Channel& c, bool skipLineCount)
    {
        if (at(positionsPointer_) == 0) {
            positionsPointer_ = loopPointer_;
        }
        c.commandsApply = static_cast<int8_t>(at(positionsPointer_)) < 0;
        uint16_t addr = word(static_cast<uint8_t>(at(positionsPointer_) * 2) +
                             patterns_);
        if (skipLineCount) {
            // Channel C reads the pattern's line count; B and A just step past
            // the same byte.
            linesCounter_ = at(addr);
        }
        c.addressInPattern = addr + 1;
        positionsPointer_++;
        c.volume = at(positionsPointer_) & 15;
        const uint8_t t = at(positionsPointer_) >> 4;
        c.transposition =
            t < 9 ? static_cast<int8_t>(t) : static_cast<int8_t>(-(t - 9) - 1);
        positionsPointer_++;
        c.holdCounter = 0;
    }

    void step()
    {
        if (--delayCounter_ == 0) {
            delayCounter_ = delay_;
            if (--linesCounter_ == 0) {
                nextPosition(chan_[2], true);
                nextPosition(chan_[1], false);
                nextPosition(chan_[0], false);
                delay_ = at(positionsPointer_);
                delayCounter_ = delay_;
                positionsPointer_++;
            }
            interpret(chan_[2]);
            interpret(chan_[1]);
            interpret(chan_[0]);
        }

        uint8_t mixer = 0;
        channelRegisters(chan_[2], mixer);
        channelRegisters(chan_[1], mixer);
        channelRegisters(chan_[0], mixer);
        mixer = static_cast<uint8_t>(~mixer & 0x3F);
        for (int i = 0; i < 3; i++) {
            if (!chan_[i].mixNoise) {
                mixer |= static_cast<uint8_t>(8 << i);
            }
            if (!chan_[i].mixTon) {
                mixer |= static_cast<uint8_t>(1 << i);
            }
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
    uint16_t base_ = 0, samples_ = 0, ornaments_ = 0, patterns_ = 0;
    uint16_t positionsPointer_ = 0, loopPointer_ = 0;
    int delay_ = 1, delayCounter_ = 1, linesCounter_ = 1;
    bool loaded_ = false;
};

} // namespace

std::unique_ptr<Source> createSqtSource(const std::vector<uint8_t>& data,
                                        int sampleRate)
{
    auto src = std::make_unique<SqtSource>(data, sampleRate);
    if (!src->start()) {
        return nullptr;
    }
    return src;
}

} // namespace musix::zxay
