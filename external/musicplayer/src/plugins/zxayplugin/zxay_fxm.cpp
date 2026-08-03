// Fuxoft AY Language (.fxm, "FXSM") and AY Amadeus (.amad).
//
// Not a tracker. Frantisek Fuka's format is a little bytecode -- three
// independent instruction streams, one per AY channel, with jumps, subroutine
// calls, counted loops and a transposition stack -- so the "player" here is an
// interpreter rather than a sequencer. The instruction set and the note table
// follow Sergey Bulba's AY_Emul, which is where this project's earlier .fxm
// support came from too, under his attribution grant (see
// players/PROVENANCE.md). The format is by Frantisek Fuka, documented in his
// fxmasm project.
//
// .amad is the same bytecode wrapped in the big-endian ZXAY container with an
// "AMAD" type tag; the container gives the load address, a noise mask and the
// title/author, and the body is interpreted identically. Those tunes are by
// Frantisek Fuka and Patrik Rak.

#include "zxay_loop.h"
#include "zxay_native.h"
#include "zxay_ticksource.h"

#include <cstring>

namespace musix::zxay {

namespace {

constexpr uint16_t kNoteTable[0x54] = {
    0xfbf, 0xedc, 0xe07, 0xd3d, 0xc7f, 0xbcc, 0xb22, 0xa82, 0x9eb, 0x95d,
    0x8d6, 0x857, 0x7df, 0x76e, 0x703, 0x69f, 0x640, 0x5e6, 0x591, 0x541,
    0x4f6, 0x4ae, 0x46b, 0x42c, 0x3f0, 0x3b7, 0x382, 0x34f, 0x320, 0x2f3,
    0x2c8, 0x2a1, 0x27b, 0x257, 0x236, 0x216, 0x1f8, 0x1dc, 0x1c1, 0x1a8,
    0x190, 0x179, 0x164, 0x150, 0x13d, 0x12c, 0x11b, 0x10b, 0x0fc, 0x0ee,
    0x0e0, 0x0d4, 0x0c8, 0x0bd, 0x0b2, 0x0a8, 0x09f, 0x096, 0x08d, 0x085,
    0x07e, 0x077, 0x070, 0x06a, 0x064, 0x05e, 0x059, 0x054, 0x04f, 0x04b,
    0x047, 0x043, 0x03f, 0x03b, 0x038, 0x035, 0x032, 0x02f, 0x02d, 0x02a,
    0x028, 0x025, 0x023, 0x021};

class FxmSource : public TickSource
{
public:
    FxmSource(std::vector<uint8_t> ram, uint16_t address, uint8_t noiseMask,
              int sampleRate)
        : machine_(sampleRate), ram_(std::move(ram)), noiseMask_(noiseMask)
    {
        for (int i = 0; i < 3; i++) {
            chan_[i].addressInPattern = word(address + i * 2);
            chan_[i].noteSkipCounter = 1;
            chan_[i].mixer = 8;
        }
    }

protected:
    bool advance() override
    {
        for (int i = 0; i < 3; i++) {
            interpret(chan_[i], stack_[i]);
        }
        for (int i = 0; i < 3; i++) {
            machine_.writeRegister(i * 2, chan_[i].tone & 0xFF);
            machine_.writeRegister(i * 2 + 1, (chan_[i].tone >> 8) & 0x0F);
            machine_.writeRegister(8 + i, chan_[i].amplitude);
        }
        machine_.writeRegister(7, (chan_[0].mixer | (chan_[1].mixer << 1) |
                                   (chan_[2].mixer << 2)) &
                                      0x3F);
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
        uint16_t pointInSample = 0, samplePointer = 0;
        uint16_t pointInOrnament = 0, ornamentPointer = 0;
        uint16_t tone = 0;
        uint8_t mixer = 8, note = 0, volume = 0, amplitude = 0;
        int8_t transposition = 0, noteSkipCounter = 0, sampleTickCounter = 0;
        // "Keep the sample running across the next note" (0x8A/0x8B) and the
        // latch that consumes it once.
        bool holdSample = false, sampleHeld = false;
        // Ornament bytes are absolute notes rather than tone deltas.
        bool ornamentIsNote = false;
    };

    uint8_t at(size_t i) const { return i < ram_.size() ? ram_[i] : 0; }
    uint16_t word(size_t i) const { return at(i) | (at(i + 1) << 8); }

    // The audible part of a tick once the streams have been walked.
    void finish(Channel& c)
    {
        machine_.writeRegister(6, noiseBase_ & 31);
        c.amplitude = c.tone != 0 ? (c.volume & 15) : 0;
    }

    void runSampleAndOrnament(Channel& c)
    {
        if (--c.sampleTickCounter == 0) {
            for (;;) {
                uint8_t v = at(c.pointInSample);
                if (v <= 0x1D) {
                    c.volume = v;
                    c.pointInSample++;
                    c.sampleTickCounter =
                        static_cast<int8_t>(at(c.pointInSample));
                    c.pointInSample++;
                    break;
                }
                if (v == 0x80) {
                    c.pointInSample = word(c.pointInSample + 1);
                    continue;
                }
                c.volume = static_cast<uint8_t>(v - 0x32);
                c.pointInSample++;
                c.sampleTickCounter = 1;
                break;
            }
        }
        if (c.tone != 0) {
            for (;;) {
                uint8_t v = at(c.pointInOrnament);
                if (v == 0x80) {
                    c.pointInOrnament = word(c.pointInOrnament + 1);
                } else if (v == 0x82) {
                    c.pointInOrnament++;
                    c.ornamentIsNote = true;
                } else if (v == 0x83) {
                    c.pointInOrnament++;
                    c.ornamentIsNote = false;
                } else if (v == 0x84) {
                    c.pointInOrnament++;
                    c.mixer ^= 9;
                } else {
                    if (c.ornamentIsNote) {
                        c.note = static_cast<uint8_t>(c.note + v);
                        c.tone = kNoteTable[c.note > 0x53 ? 0x53 : c.note];
                    } else {
                        c.tone = static_cast<uint16_t>(
                            c.tone + static_cast<int8_t>(v));
                    }
                    c.pointInOrnament++;
                    break;
                }
            }
        }
        finish(c);
    }

    void interpret(Channel& c, std::vector<int>& stack)
    {
        if (--c.noteSkipCounter != 0) {
            runSampleAndOrnament(c);
            return;
        }
        for (;;) {
            uint8_t op = at(c.addressInPattern);
            if (op < 0x80) {
                if (op != 0) {
                    c.note = static_cast<uint8_t>(op - 1 + c.transposition);
                    c.tone = kNoteTable[c.note > 0x53 ? 0x53 : c.note];
                    c.ornamentIsNote = false;
                } else {
                    c.tone = 0;
                }
                c.addressInPattern++;
                c.noteSkipCounter = static_cast<int8_t>(at(c.addressInPattern));
                c.addressInPattern++;
                c.pointInOrnament = c.ornamentPointer;
                if (!c.sampleHeld) {
                    c.sampleHeld = c.holdSample;
                    c.pointInSample = c.samplePointer;
                    c.volume = at(c.pointInSample);
                    c.pointInSample++;
                    c.sampleTickCounter =
                        static_cast<int8_t>(at(c.pointInSample));
                    c.pointInSample++;
                    finish(c);
                } else {
                    runSampleAndOrnament(c);
                }
                return;
            }
            switch (op) {
            case 0x80: // jump
                c.addressInPattern = word(c.addressInPattern + 1);
                break;
            case 0x81: // call
                stack.push_back(c.addressInPattern + 3);
                c.addressInPattern = word(c.addressInPattern + 1);
                break;
            case 0x82: // open a counted loop: push count, then loop top
                c.addressInPattern++;
                stack.push_back(at(c.addressInPattern));
                c.addressInPattern++;
                stack.push_back(c.addressInPattern);
                break;
            case 0x83: { // close the loop
                if (stack.size() < 2) {
                    c.addressInPattern++;
                    break;
                }
                int& count = stack[stack.size() - 2];
                count--;
                if ((count & 255) != 0) {
                    c.addressInPattern =
                        static_cast<uint16_t>(stack[stack.size() - 1]);
                } else {
                    stack.resize(stack.size() - 2);
                    c.addressInPattern++;
                }
                break;
            }
            case 0x84:
                c.addressInPattern++;
                noiseBase_ = at(c.addressInPattern);
                c.addressInPattern++;
                break;
            case 0x85:
                c.addressInPattern++;
                c.mixer = at(c.addressInPattern);
                c.addressInPattern++;
                break;
            case 0x86:
                c.addressInPattern++;
                c.ornamentPointer = word(c.addressInPattern);
                c.addressInPattern += 2;
                break;
            case 0x87:
                c.addressInPattern++;
                c.samplePointer = word(c.addressInPattern);
                c.addressInPattern += 2;
                break;
            case 0x88:
                c.addressInPattern++;
                c.transposition = static_cast<int8_t>(at(c.addressInPattern));
                c.addressInPattern++;
                break;
            case 0x89: // return
                if (stack.empty()) {
                    c.addressInPattern++;
                } else {
                    c.addressInPattern = static_cast<uint16_t>(stack.back());
                    stack.pop_back();
                }
                break;
            case 0x8A:
                c.addressInPattern++;
                c.holdSample = true;
                c.sampleHeld = false;
                break;
            case 0x8B:
                c.addressInPattern++;
                c.holdSample = false;
                c.sampleHeld = false;
                break;
            case 0x8C:
                c.addressInPattern += 3;
                break;
            case 0x8D:
                c.addressInPattern++;
                noiseBase_ = static_cast<uint8_t>(
                    (noiseBase_ + at(c.addressInPattern)) & noiseMask_);
                c.addressInPattern++;
                break;
            case 0x8E:
                c.addressInPattern++;
                c.transposition = static_cast<int8_t>(
                    c.transposition + at(c.addressInPattern));
                c.addressInPattern++;
                break;
            case 0x8F:
                stack.push_back(c.transposition);
                c.addressInPattern++;
                break;
            case 0x90:
                if (!stack.empty()) {
                    c.transposition = static_cast<int8_t>(stack.back());
                    stack.pop_back();
                }
                c.addressInPattern++;
                break;
            default:
                c.addressInPattern++;
                break;
            }
        }
    }

    ZxAyMachine machine_;
    std::vector<uint8_t> ram_; // 64K image, body at its load address
    LoopDetector loop_;
    Channel chan_[3];
    std::vector<int> stack_[3];
    uint8_t noiseBase_ = 0;
    uint8_t noiseMask_ = 31;
};

// Places `body` at `address` in a fresh 64K image, the way the ZX Spectrum
// would have -- every pointer inside this format is an absolute Z80 address.
std::vector<uint8_t> imageAt(const uint8_t* body, size_t len, uint16_t address)
{
    std::vector<uint8_t> ram(0x10000, 0);
    size_t n = std::min(len, static_cast<size_t>(0x10000 - address));
    std::memcpy(ram.data() + address, body, n);
    return ram;
}

uint16_t be16(const uint8_t* p) { return (p[0] << 8) | p[1]; }

} // namespace

std::unique_ptr<Source> createFxmSource(Format f,
                                        const std::vector<uint8_t>& data,
                                        int sampleRate)
{
    if (f == Format::fxm) {
        // "FXSM", load address, then the bytecode.
        if (data.size() < 12) {
            return nullptr;
        }
        uint16_t address = data[4] | (data[5] << 8);
        auto src = std::make_unique<FxmSource>(
            imageAt(data.data() + 6, data.size() - 6, address), address, 31,
            sampleRate);
        return src;
    }

    // ZXAY/AMAD. Big-endian throughout, and every pointer is relative to its
    // OWN position in the file rather than to the start.
    if (data.size() < 24) {
        return nullptr;
    }
    auto rel = [&](size_t at) -> long {
        return static_cast<long>(at) +
               static_cast<int16_t>(be16(data.data() + at));
    };
    auto zstring = [&](long at) {
        std::string s;
        while (at >= 0 && at < static_cast<long>(data.size()) && data[at] != 0) {
            s.push_back(static_cast<char>(data[at++]));
        }
        return s;
    };

    const long author = rel(12);
    // Song 0 only: every .amad in this corpus is single-song, and the format
    // has no way to surface a subsong index through this plugin.
    const long songs = rel(18);
    if (songs < 0 || songs + 4 > static_cast<long>(data.size())) {
        return nullptr;
    }
    const long cursor = songs + 4; // AY_Emul's "position after the structure"
    const long nameAt = cursor - 4 + static_cast<int16_t>(be16(&data[songs]));
    const long dataAt = cursor - 2 + static_cast<int16_t>(be16(&data[songs + 2]));
    if (dataAt < 0 || dataAt + 14 > static_cast<long>(data.size())) {
        return nullptr;
    }

    // The song-data block is: load address (big-endian), the noise mask, a
    // byte and a word that multiply out to the playing time, and then the
    // bytecode -- which starts 14 bytes in, not 6. The extra 8 are what a
    // standalone .fxm spends on its "FXSM" tag and load address, so the two
    // paths converge on the same body layout.
    const uint16_t address = be16(&data[dataAt]);
    const uint8_t noiseMask = data[dataAt + 2];
    const long body = dataAt + 14;
    if (body >= static_cast<long>(data.size())) {
        return nullptr;
    }

    auto src = std::make_unique<FxmSource>(
        imageAt(data.data() + body, data.size() - body, address), address,
        noiseMask, sampleRate);
    SongInfo info;
    info.title = zstring(nameAt);
    info.author = zstring(author);
    src->setInfo(info);
    return src;
}

} // namespace musix::zxay
