// ASC Sound Master modules (.asc), both the 0.x and 1.x layouts.
//
// The second of the two formats with no redistributable ZX replay routine, and
// much the more elaborate of them: on top of the usual sample/ornament pair it
// has tone sliding with a computed step, portamento to a target note, a
// tremolo-ish "amplitude delay", per-sample envelope-period nudging and two
// independent loop points. There is no published format description to work
// from either, so unlike Sound Tracker this one follows Sergey Bulba's own
// AY_Emul implementation directly, under his attribution grant (see
// players/PROVENANCE.md).
//
// ASC 0.x differs from 1.x only by not having a loop-position byte at +1. It is
// normalised to the 1.x layout on load, exactly as AY_Emul does, so there is
// one sequencer rather than two.

#include "zxay_loop.h"
#include "zxay_native.h"
#include "zxay_ticksource.h"

#include <algorithm>
#include <cstring>

namespace musix::zxay {

namespace {

// Tone periods for ASC's 86 semitones.
constexpr uint16_t kNoteTable[0x56] = {
    0xedc, 0xe07, 0xd3e, 0xc80, 0xbcc, 0xb22, 0xa82, 0x9ec, 0x95c, 0x8d6,
    0x858, 0x7e0, 0x76e, 0x704, 0x69f, 0x640, 0x5e6, 0x591, 0x541, 0x4f6,
    0x4ae, 0x46b, 0x42c, 0x3f0, 0x3b7, 0x382, 0x34f, 0x320, 0x2f3, 0x2c8,
    0x2a1, 0x27b, 0x257, 0x236, 0x216, 0x1f8, 0x1dc, 0x1c1, 0x1a8, 0x190,
    0x179, 0x164, 0x150, 0x13d, 0x12c, 0x11b, 0x10b, 0x0fc, 0x0ee, 0x0e0,
    0x0d4, 0x0c8, 0x0bd, 0x0b2, 0x0a8, 0x09f, 0x096, 0x08d, 0x085, 0x07e,
    0x077, 0x070, 0x06a, 0x064, 0x05e, 0x059, 0x054, 0x050, 0x04b, 0x047,
    0x043, 0x03f, 0x03c, 0x038, 0x035, 0x032, 0x02f, 0x02d, 0x02a, 0x028,
    0x026, 0x024, 0x022, 0x020, 0x01e, 0x01c};

// The top five bits of a byte, sign-extended -- ASC's way of packing a small
// signed delta alongside three flag bits.
int8_t topFiveSigned(uint8_t v)
{
    return static_cast<int8_t>(static_cast<int8_t>(v << 3) / 8);
}

class AscSource : public TickSource
{
public:
    AscSource(std::vector<uint8_t> mod, int sampleRate)
        : machine_(sampleRate), mod_(std::move(mod))
    {
    }

    bool start()
    {
        if (!normalise()) {
            return false;
        }
        delay_ = mod_[0];
        loopPosition_ = mod_[1];
        patterns_ = word(2);
        samples_ = word(4);
        ornaments_ = word(6);
        numberOfPositions_ = mod_[8];
        if (delay_ == 0 || numberOfPositions_ == 0 ||
            patterns_ >= mod_.size() || samples_ >= mod_.size() ||
            ornaments_ >= mod_.size()) {
            return false;
        }
        delayCounter_ = 1;
        for (auto& c : chan_) {
            c = Channel{};
        }
        return setPatternAddresses(mod_[9]);
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
        uint16_t initialPointInSample = 0, pointInSample = 0,
                 loopPointInSample = 0;
        uint16_t initialPointInOrnament = 0, pointInOrnament = 0,
                 loopPointInOrnament = 0;
        uint16_t addressInPattern = 0;
        uint16_t tone = 0;
        uint16_t toneDeviation = 0;
        uint8_t note = 0, additionToNote = 0, numberOfNotesToSkip = 0;
        uint8_t initialNoise = 0, currentNoise = 0, volume = 0;
        uint8_t tonSlidingCounter = 0, amplitude = 0;
        uint8_t amplitudeDelay = 0, amplitudeDelayCounter = 0;
        int16_t currentTonSliding = 0, substructionForTonSliding = 0;
        int8_t noteSkipCounter = 0, additionToAmplitude = 0;
        bool envelopeEnabled = false, soundEnabled = false;
        bool sampleFinished = false, breakSampleLoop = false;
    };

    uint8_t at(size_t i) const { return i < mod_.size() ? mod_[i] : 0; }
    uint16_t word(size_t i) const { return at(i) | (at(i + 1) << 8); }

    // ASC 0.x has no loop-position byte, so splice one in and shift the three
    // pointers past it. After this the rest of the player only knows 1.x.
    bool normalise()
    {
        if (mod_.size() < 16) {
            return false;
        }
        // 1.x has the patterns pointer at +2; 0.x at +1. The 0.x shape is the
        // one where the three words starting at +1 ascend and the one starting
        // at +2 does not.
        auto ascending = [&](size_t b) {
            uint16_t a = word(b), c = word(b + 2), d = word(b + 4);
            return a != 0 && a < c && c < d && d < mod_.size();
        };
        if (ascending(2)) {
            return true; // already 1.x
        }
        if (!ascending(1)) {
            return false;
        }
        mod_.insert(mod_.begin() + 1, 0); // loop position 0
        for (int i = 0; i < 3; i++) {
            uint16_t p = word(2 + i * 2) + 1;
            mod_[2 + i * 2] = p & 0xFF;
            mod_[3 + i * 2] = p >> 8;
        }
        return true;
    }

    bool setPatternAddresses(uint8_t pattern)
    {
        size_t rec = patterns_ + 6 * pattern;
        if (rec + 6 > mod_.size()) {
            return false;
        }
        for (int i = 0; i < 3; i++) {
            chan_[i].addressInPattern = word(rec + i * 2) + patterns_;
        }
        return true;
    }

    void interpret(Channel& c)
    {
        bool noSampleInit = false;
        bool noOrnamentInit = false;
        c.tonSlidingCounter = 0;
        c.amplitudeDelayCounter = 0;
        for (;;) {
            uint8_t code = at(c.addressInPattern);
            if (code <= 0x55) {
                c.note = code;
                c.addressInPattern++;
                c.currentNoise = c.initialNoise;
                if (static_cast<int8_t>(c.tonSlidingCounter) <= 0) {
                    c.currentTonSliding = 0;
                }
                if (!noSampleInit) {
                    c.additionToAmplitude = 0;
                    c.toneDeviation = 0;
                    c.pointInSample = c.initialPointInSample;
                    c.soundEnabled = true;
                    c.sampleFinished = false;
                    c.breakSampleLoop = false;
                }
                if (!noOrnamentInit) {
                    c.pointInOrnament = c.initialPointInOrnament;
                    c.additionToNote = 0;
                }
                if (c.envelopeEnabled) {
                    env11_ = at(c.addressInPattern);
                    machine_.writeRegister(11, env11_);
                    c.addressInPattern++;
                }
                break;
            }
            if (code <= 0x5D) { // no-op note codes
                c.addressInPattern++;
                break;
            }
            if (code == 0x5E) {
                c.breakSampleLoop = true;
                c.addressInPattern++;
                break;
            }
            if (code == 0x5F) {
                c.soundEnabled = false;
                c.addressInPattern++;
                break;
            }
            if (code <= 0x9F) {
                c.numberOfNotesToSkip = code - 0x60;
            } else if (code <= 0xBF) {
                c.initialPointInSample =
                    word(samples_ + (code - 0xA0) * 2) + samples_;
            } else if (code <= 0xDF) {
                c.initialPointInOrnament =
                    word(ornaments_ + (code - 0xC0) * 2) + ornaments_;
            } else if (code == 0xE0) {
                c.volume = 15;
                c.envelopeEnabled = true;
            } else if (code <= 0xEF) {
                c.volume = code - 0xE0;
                c.envelopeEnabled = false;
            } else if (code == 0xF0) {
                c.addressInPattern++;
                c.initialNoise = at(c.addressInPattern);
            } else if (code == 0xF1) {
                noSampleInit = true;
            } else if (code == 0xF2) {
                noOrnamentInit = true;
            } else if (code == 0xF3) {
                noSampleInit = true;
                noOrnamentInit = true;
            } else if (code == 0xF4) {
                c.addressInPattern++;
                delay_ = at(c.addressInPattern);
            } else if (code == 0xF5 || code == 0xF6) {
                c.addressInPattern++;
                int step = static_cast<int8_t>(at(c.addressInPattern)) * 16;
                c.substructionForTonSliding =
                    static_cast<int16_t>(code == 0xF5 ? -step : step);
                c.tonSlidingCounter = 255;
            } else if (code == 0xF7 || code == 0xF9) {
                // Portamento to the note named two bytes on, over the number
                // of ticks named in the next byte.
                c.addressInPattern++;
                if (code == 0xF7) {
                    noSampleInit = true;
                }
                uint8_t target = at(c.addressInPattern + 1);
                int delta;
                if (target < 0x56) {
                    delta = kNoteTable[c.note] - kNoteTable[target];
                    if (code == 0xF7) {
                        delta += c.currentTonSliding / 16;
                    }
                } else {
                    delta = c.currentTonSliding / 16;
                }
                delta <<= 4;
                int8_t ticks = static_cast<int8_t>(at(c.addressInPattern));
                if (ticks != 0) {
                    c.substructionForTonSliding =
                        static_cast<int16_t>(-delta / ticks);
                    c.currentTonSliding =
                        static_cast<int16_t>(delta - delta % ticks);
                }
                c.tonSlidingCounter = static_cast<uint8_t>(ticks);
            } else if (code == 0xF8) {
                machine_.writeRegister(13, 8);
            } else if (code == 0xFA) {
                machine_.writeRegister(13, 10);
            } else if (code == 0xFB) {
                c.addressInPattern++;
                uint8_t v = at(c.addressInPattern);
                if ((v & 32) == 0) {
                    c.amplitudeDelay = static_cast<uint8_t>(v << 3);
                } else {
                    // Bit 0 is the sign and bits 7..3 the magnitude.
                    c.amplitudeDelay =
                        static_cast<uint8_t>(((v << 3) ^ 0xF8) + 9);
                }
                c.amplitudeDelayCounter = c.amplitudeDelay;
            } else if (code == 0xFC) {
                machine_.writeRegister(13, 12);
            } else if (code == 0xFE) {
                machine_.writeRegister(13, 14);
            }
            c.addressInPattern++;
        }
        c.noteSkipCounter = static_cast<int8_t>(c.numberOfNotesToSkip);
    }

    void channelRegisters(Channel& c, uint8_t& mixer)
    {
        if (c.sampleFinished || !c.soundEnabled) {
            c.amplitude = 0;
            mixer >>= 1;
            return;
        }

        if (c.amplitudeDelayCounter != 0) {
            if (c.amplitudeDelayCounter >= 16) {
                c.amplitudeDelayCounter -= 8;
                if (c.additionToAmplitude < -15) {
                    c.additionToAmplitude++;
                } else if (c.additionToAmplitude > 15) {
                    c.additionToAmplitude--;
                }
            } else {
                if (c.amplitudeDelayCounter & 1) {
                    if (c.additionToAmplitude > -15) {
                        c.additionToAmplitude--;
                    }
                } else if (c.additionToAmplitude < 15) {
                    c.additionToAmplitude++;
                }
                c.amplitudeDelayCounter = c.amplitudeDelay;
            }
        }

        const uint8_t s0 = at(c.pointInSample);
        const uint8_t s1 = at(c.pointInSample + 1);
        const uint8_t s2 = at(c.pointInSample + 2);
        if (s0 & 128) {
            c.loopPointInSample = c.pointInSample;
        }
        if ((s0 & 96) == 32) {
            c.sampleFinished = true;
        }
        c.toneDeviation += static_cast<int8_t>(s1);
        mixer |= static_cast<uint8_t>((s2 & 9) << 3);
        const bool sampleOkForEnvelope = (s2 & 6) == 2;
        if ((s2 & 6) == 4 && c.additionToAmplitude > -15) {
            c.additionToAmplitude--;
        }
        if ((s2 & 6) == 6 && c.additionToAmplitude < 15) {
            c.additionToAmplitude++;
        }
        int amp = static_cast<uint8_t>(c.additionToAmplitude) + (s2 >> 4);
        if (static_cast<int8_t>(amp) < 0) {
            amp = 0;
        } else if (amp > 15) {
            amp = 15;
        }
        c.amplitude = static_cast<uint8_t>((amp * (c.volume + 1)) >> 4);
        if (sampleOkForEnvelope && (mixer & 64) != 0) {
            env11_ = static_cast<uint8_t>(env11_ + topFiveSigned(s0));
            machine_.writeRegister(11, env11_);
        } else {
            c.currentNoise =
                static_cast<uint8_t>(c.currentNoise + topFiveSigned(s0));
        }
        c.pointInSample += 3;
        if (s0 & 64) {
            if (!c.breakSampleLoop) {
                c.pointInSample = c.loopPointInSample;
            } else if (s0 & 32) {
                c.sampleFinished = true;
            }
        }

        const uint8_t o0 = at(c.pointInOrnament);
        if (o0 & 128) {
            c.loopPointInOrnament = c.pointInOrnament;
        }
        c.additionToNote =
            static_cast<uint8_t>(c.additionToNote + at(c.pointInOrnament + 1));
        c.currentNoise = static_cast<uint8_t>(
            c.currentNoise +
            (static_cast<uint8_t>(-static_cast<int8_t>(o0 & 0x10)) | o0));
        c.pointInOrnament += 2;
        if (o0 & 64) {
            c.pointInOrnament = c.loopPointInOrnament;
        }

        if ((mixer & 64) == 0) {
            machine_.writeRegister(
                6, ((c.currentTonSliding >> 8) + c.currentNoise) & 0x1F);
        }
        int j = static_cast<int8_t>(c.note + c.additionToNote);
        j = std::clamp(j, 0, 0x55);
        c.tone = static_cast<uint16_t>(
            (kNoteTable[j] + c.toneDeviation + (c.currentTonSliding / 16)) &
            0xFFF);
        if (c.tonSlidingCounter != 0) {
            if (static_cast<int8_t>(c.tonSlidingCounter) > 0) {
                c.tonSlidingCounter--;
            }
            c.currentTonSliding = static_cast<int16_t>(
                c.currentTonSliding + c.substructionForTonSliding);
        }
        if (c.envelopeEnabled && sampleOkForEnvelope) {
            c.amplitude |= 0x10;
        }
        mixer >>= 1;
    }

    void step()
    {
        if (--delayCounter_ == 0) {
            if (--chan_[0].noteSkipCounter < 0) {
                if (at(chan_[0].addressInPattern) == 0xFF) {
                    if (++currentPosition_ >= numberOfPositions_) {
                        currentPosition_ = loopPosition_;
                    }
                    setPatternAddresses(at(currentPosition_ + 9));
                    for (auto& c : chan_) {
                        c.initialNoise = 0;
                    }
                }
                interpret(chan_[0]);
            }
            for (int i = 1; i < 3; i++) {
                if (--chan_[i].noteSkipCounter < 0) {
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
    }

    ZxAyMachine machine_;
    std::vector<uint8_t> mod_;
    LoopDetector loop_;
    Channel chan_[3];
    uint16_t patterns_ = 0, samples_ = 0, ornaments_ = 0;
    uint8_t delay_ = 1, loopPosition_ = 0, numberOfPositions_ = 0;
    uint8_t env11_ = 0;
    int delayCounter_ = 1;
    int currentPosition_ = 0;
};

} // namespace

std::unique_ptr<Source> createAscSource(const std::vector<uint8_t>& data,
                                        int sampleRate)
{
    auto src = std::make_unique<AscSource>(data, sampleRate);
    if (!src->start()) {
        return nullptr;
    }
    return src;
}

} // namespace musix::zxay
