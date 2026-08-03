// Recorded AY register streams: .vtx (Vortex) and .psg.
//
// These are not modules and there is no replay routine to run -- somebody
// already ran one and wrote down what came out of the CPU each 1/50 s. All we
// do is push the recorded writes into the chip on schedule, which means these
// two formats are exactly as accurate as the chip emulation and no more.

#include "zx_ay_machine.h"
#include "zxay_native.h"

#include "LZH/LZH.H"

#include <cstring>
#include <string>

namespace musix::zxay {

namespace {

uint16_t rd16(const uint8_t* d) { return d[0] | (d[1] << 8); }
uint32_t rd32(const uint8_t* d)
{
    return static_cast<uint32_t>(d[0]) | (static_cast<uint32_t>(d[1]) << 8) |
           (static_cast<uint32_t>(d[2]) << 16) |
           (static_cast<uint32_t>(d[3]) << 24);
}

// VTX's stereo byte enumerates all six channel permutations; the machine
// offers the two that matter (and mono), so the rarer ones fall back to ABC.
Stereo vtxStereo(uint8_t v)
{
    switch (v) {
    case 0: return Stereo::mono;
    case 2: return Stereo::acb;
    default: return Stereo::abc;
    }
}

// A stream of per-tick register frames, whatever produced it.
class DumpSource : public Source
{
public:
    DumpSource(std::vector<uint8_t> frames, int frameCount, int sampleRate,
               Stereo stereo, double ayClock)
        : machine_(sampleRate, stereo, ayClock), frames_(std::move(frames)),
          frameCount_(frameCount)
    {
    }

    int render(int16_t* out, int frames) override
    {
        int done = 0;
        while (done < frames) {
            if (pending_.empty()) {
                if (at_ >= frameCount_) {
                    break;
                }
                const uint8_t* regs = &frames_[static_cast<size_t>(at_) * 14];
                for (int r = 0; r < 14; r++) {
                    // R13 restarts the envelope on every write, so a dump that
                    // holds the same shape for a hundred frames must not have
                    // it re-poked each tick. 0xFF is the recorded "no write".
                    if (r == 13 && regs[13] == 0xFF) {
                        continue;
                    }
                    machine_.writeRegister(r, regs[r]);
                }
                at_++;
                pending_.resize(static_cast<size_t>(machine_.samplesPerTick()) *
                                2);
                machine_.renderTick(pending_.data(), machine_.samplesPerTick());
                cursor_ = 0;
            }
            int avail = static_cast<int>(pending_.size()) / 2 - cursor_;
            int n = std::min(frames - done, avail);
            std::memcpy(out + done * 2, pending_.data() + cursor_ * 2,
                        static_cast<size_t>(n) * 2 * sizeof(int16_t));
            cursor_ += n;
            done += n;
            if (cursor_ * 2 >= static_cast<int>(pending_.size())) {
                pending_.clear();
            }
        }
        return done;
    }

    bool ended() const override { return at_ >= frameCount_ && pending_.empty(); }

private:
    ZxAyMachine machine_;
    std::vector<uint8_t> frames_; // frameCount_ * 14, tick-major
    int frameCount_;
    int at_ = 0;
    std::vector<int16_t> pending_;
    int cursor_ = 0;
};

std::unique_ptr<Source> loadVtx(const std::vector<uint8_t>& d, int sampleRate)
{
    if (d.size() < 20) {
        return nullptr;
    }
    const Stereo stereo = vtxStereo(d[2]);
    const uint32_t chipFreq = rd32(&d[5]);
    const uint8_t playerFreq = d[9];
    const uint32_t unpackedSize = rd32(&d[12]);
    if (unpackedSize == 0 || unpackedSize % 14 != 0 ||
        unpackedSize > 14u * 50u * 60u * 30u) {
        return nullptr;
    }

    // Five NUL-terminated strings (title, author, program, tracker, comment)
    // sit between the header and the packed data.
    size_t at = 16;
    std::string text[5];
    for (auto& s : text) {
        size_t start = at;
        while (at < d.size() && d[at] != 0) {
            at++;
        }
        s.assign(reinterpret_cast<const char*>(d.data() + start), at - start);
        if (at < d.size()) {
            at++;
        }
    }
    if (at >= d.size()) {
        return nullptr;
    }

    // The payload is LH5, the same packing the .ym files this repo already
    // plays use, so it depacks with StSound's depacker.
    std::vector<uint8_t> planes(unpackedSize);
    CLzhDepacker lzh;
    if (!lzh.LzUnpack(const_cast<uint8_t*>(d.data() + at),
                      static_cast<int>(d.size() - at), planes.data(),
                      static_cast<int>(unpackedSize))) {
        return nullptr;
    }

    // VTX stores the dump REGISTER-major -- every frame's R0, then every
    // frame's R1, and so on -- which packs far better than interleaving. Undo
    // that so playback can walk one frame at a time.
    const int frameCount = static_cast<int>(unpackedSize / 14);
    std::vector<uint8_t> frames(unpackedSize);
    for (int r = 0; r < 14; r++) {
        for (int f = 0; f < frameCount; f++) {
            frames[static_cast<size_t>(f) * 14 + r] =
                planes[static_cast<size_t>(r) * frameCount + f];
        }
    }

    auto src = std::make_unique<DumpSource>(
        std::move(frames), frameCount, sampleRate, stereo,
        chipFreq != 0 ? chipFreq : ZX128_AY_CLOCK);
    SongInfo info;
    info.title = text[0];
    info.author = text[1];
    info.lengthSeconds = frameCount / (playerFreq != 0 ? playerFreq : PLAY_HZ);
    src->setInfo(info);
    return src;
}

std::unique_ptr<Source> loadPsg(const std::vector<uint8_t>& d, int sampleRate)
{
    if (d.size() < 17) {
        return nullptr;
    }
    // +0 "PSG\x1A", +4 version, +5 player frequency (0 means the 50 Hz
    // default), +6..15 reserved. The stream starts at 16.
    const uint8_t playerFreq = d[5] != 0 ? d[5] : PLAY_HZ;

    std::vector<uint8_t> frames;
    uint8_t regs[14];
    std::memset(regs, 0, sizeof(regs));
    // A fresh chip is silent, and a dump that never writes R7 must not start
    // by unmuting everything.
    regs[7] = 0x3F;
    int frameCount = 0;

    auto flush = [&](int repeat) {
        for (int i = 0; i < repeat; i++) {
            frames.insert(frames.end(), regs, regs + 14);
            frameCount++;
        }
    };

    // Register writes accumulate into the frame being built; 0xFF ends it.
    size_t at = 16;
    while (at < d.size()) {
        uint8_t b = d[at++];
        if (b == 0xFF) {
            flush(1);
        } else if (b == 0xFE) {
            if (at >= d.size()) {
                break;
            }
            // The format's run-length for a held state, counted in groups of
            // four frames.
            flush(d[at++] * 4);
        } else if (b == 0xFD) {
            break;
        } else {
            if (at >= d.size()) {
                break;
            }
            uint8_t v = d[at++];
            if (b < 14) {
                regs[b] = v;
            }
        }
    }
    flush(1); // whatever was accumulating when the stream ran out
    if (frameCount == 0) {
        return nullptr;
    }

    auto src = std::make_unique<DumpSource>(std::move(frames), frameCount,
                                            sampleRate, Stereo::abc,
                                            ZX128_AY_CLOCK);
    SongInfo info;
    info.lengthSeconds = frameCount / playerFreq;
    src->setInfo(info);
    return src;
}

} // namespace

std::unique_ptr<Source> createDumpSource(Format f,
                                         const std::vector<uint8_t>& data,
                                         int sampleRate)
{
    return f == Format::vtx ? loadVtx(data, sampleRate)
                            : loadPsg(data, sampleRate);
}

} // namespace musix::zxay
