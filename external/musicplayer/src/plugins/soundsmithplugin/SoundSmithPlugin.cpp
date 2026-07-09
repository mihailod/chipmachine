#include "SoundSmithPlugin.h"
#include "../../chipplayer.h"

#include <coreutils/file.h>
#include <coreutils/log.h>
#include <coreutils/utils.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

// SoundSmith / Apple IIgs player.
//
// Faithful in-process port of Sean Kasun's BSD-licensed SoundSmith player
// (https://github.com/mrkite/soundsmith, vendored at repo-root soundsmith/;
// see src/es5503.ts + src/player.ts and tools/ss2wav.c). The Apple IIgs sound
// hardware is the Ensoniq 5503 "DOC", a 32-oscillator (16 stereo pairs) digital
// oscillator with 64KB of sound RAM. SoundSmith uses 14 voice-pairs for music
// and one oscillator (30) as a ~50Hz interrupt timer that clocks the tracker.
//
// A tune is two files: the song (patterns/orders/effects, magic "SONGOK") and a
// separate wavebank (".W" on Modland) holding the 64KB of sample RAM plus the
// instrument and tuning tables. The DOC is emulated at its native 26320 Hz and
// the result is resampled to 44100 Hz on output (the host plays our samples into
// a fixed 44100 Hz device and ignores getHZ()).

namespace musix {

namespace {

// SoundSmith song layout constants. The first 6 bytes are a free-form ASCII
// signature the player ignores -- it varies per editor build ("SONGOK",
// "IAN9OK", "IAN92a", ...), so it is NOT a usable magic. Instead we validate by
// structure: blockLen (LE16 @6) is the size of each of the three pattern tables,
// which begin at 0x258. A table holds whole patterns of 64 rows x 14 voices, so
// blockLen is always a nonzero multiple of 896, and 3 tables must fit the file.
constexpr int VOICES = 14;
constexpr int ROWS_PER_PATTERN = 64;
constexpr int PATTERN_BYTES = ROWS_PER_PATTERN * VOICES; // 896
constexpr size_t TABLES_OFFSET = 0x258;
constexpr size_t SONGLEN_OFFSET = 0x1d6;

// Validate the song header. 'hdr'/'hdrLen' is the buffer holding the leading
// bytes (must reach SONGLEN_OFFSET); 'fileSize' is the full file length, used
// for the pattern-table fit check (canHandle reads only the header, not the
// whole file).
bool looksLikeSong(const uint8_t* hdr, size_t hdrLen, size_t fileSize)
{
    if (hdrLen <= SONGLEN_OFFSET) {
        return false;
    }
    int blockLen = hdr[6] | (hdr[7] << 8);
    if (blockLen <= 0 || (blockLen % PATTERN_BYTES) != 0) {
        return false;
    }
    if (TABLES_OFFSET + static_cast<size_t>(blockLen) * 3 > fileSize) {
        return false; // the three pattern tables must fit
    }
    int songLen = hdr[SONGLEN_OFFSET] & 0xff;
    if (songLen < 1 || songLen > 0x7f) {
        return false; // order-list length out of range
    }
    return true;
}

inline uint16_t rd16(const std::vector<uint8_t>& v, size_t off)
{
    if (off + 1 >= v.size()) {
        return 0;
    }
    return static_cast<uint16_t>(v[off] | (v[off + 1] << 8));
}

// DOC oscillator mode (control bits 2..1).
enum Mode { FreeRun = 0, OneShot = 1, Sync = 2, Swap = 3 };

struct Oscillator
{
    uint32_t pointer = 0;
    uint32_t frequency = 0;
    uint8_t size = 0;
    uint8_t control = 1; // halted
    double volume = 0.0;
    double data = 0.0;
    uint8_t resolution = 0;
    uint32_t accumulator = 0;
    uint32_t ptr = 0;
    uint8_t shift = 9;
    uint32_t max = 0xff;
};

// Ensoniq 5503 (DOC) digital oscillator emulation. Ported 1:1 from es5503.ts.
class ES5503
{
public:
    explicit ES5503(std::function<void(int)> irq) : irq_(std::move(irq))
    {
        // Sound RAM is 64KB but the wavetable address (base&max)+ptr can reach
        // ~0x1ffff; size to 0x20000 so out-of-range reads land in zero-padding
        // (silent) instead of out of bounds.
        waveTable_.assign(0x20000, 0.0);
    }

    void setEnabled(int enabled) { enabled_ = enabled >> 1; }
    int enabled() const { return enabled_; }

    void setRam(const uint8_t* bank, size_t len)
    {
        for (size_t i = 0; i < len && i < 0x10000; i++) {
            waveTable_[i] = (static_cast<int>(bank[i]) - 128) / 128.0;
        }
    }

    void setFrequency(int osc, uint32_t freq) { osc_[osc].frequency = freq; }
    void setVolume(int osc, int vol) { osc_[osc].volume = vol / 127.0; }

    void setPointer(int osc, uint8_t ptr)
    {
        osc_[osc].pointer = static_cast<uint32_t>(ptr) << 8;
        recalc(osc);
    }

    void setSize(int osc, uint8_t size)
    {
        osc_[osc].size = (size >> 3) & 7;
        osc_[osc].resolution = size & 7;
        recalc(osc);
    }

    void setControl(int osc, uint8_t ctl)
    {
        uint8_t prev = osc_[osc].control & 1;
        osc_[osc].control = ctl;
        int mode = (ctl >> 1) & 3;
        if (!(ctl & 1) && prev) { // newly triggered
            if (mode == Sync) {    // trigger pair
                osc_[osc ^ 1].control &= ~1;
                osc_[osc ^ 1].accumulator = 0;
            }
            osc_[osc].accumulator = 0;
        }
    }

    // Halt without triggering an interrupt (tracker note-off / retrigger).
    void stop(int osc)
    {
        osc_[osc].control &= 0xf7; // clear interrupt bit
        osc_[osc].control |= 1;    // halt
        osc_[osc].accumulator = 0;
    }

    // Unhalt without triggering a swap (the IRQ "keep looping" path).
    void go(int osc) { osc_[osc].control &= ~1; }

    void resetOsc(int osc)
    {
        osc_[osc].control = 1;
        osc_[osc].data = 0.0;
    }

    void tick()
    {
        for (int osc = 0; osc <= enabled_; osc++) {
            Oscillator& cur = osc_[osc];
            if (!(cur.control & 1)) { // running
                uint32_t base = cur.accumulator >> cur.shift;
                uint32_t ofs = (base & cur.max) + cur.ptr;
                double sample = (ofs < waveTable_.size()) ? waveTable_[ofs] : 0.0;
                cur.data = sample * cur.volume;
                cur.accumulator += cur.frequency;
                if (sample == -1.0) { // a 0 byte in the raw wavetable
                    halted(osc, true);
                } else if (base >= cur.max) {
                    halted(osc, false);
                }
            }
        }
    }

    void render(double& left, double& right)
    {
        double l = 0.0;
        double r = 0.0;
        for (int osc = 0; osc <= enabled_; osc++) {
            Oscillator& cur = osc_[osc];
            if (!(cur.control & 1)) {
                if (cur.control & 0x10) {
                    r += cur.data;
                } else {
                    l += cur.data;
                }
            }
        }
        double spread = (enabled_ - 2) / 4.0;
        left = l / spread;
        right = r / spread;
    }

private:
    void recalc(int osc)
    {
        Oscillator& cur = osc_[osc];
        cur.shift = (cur.resolution + 9) - cur.size;
        cur.ptr = cur.pointer & waveMasks_[cur.size];
        cur.max = waveSizes_[cur.size] - 1;
    }

    void halted(int osc, bool interrupted)
    {
        Oscillator& cur = osc_[osc];
        int mode = (cur.control >> 1) & 3;
        if (interrupted || mode != FreeRun) {
            cur.control |= 1; // halt
        } else {
            int32_t base =
                static_cast<int32_t>(cur.accumulator >> cur.shift) - cur.max;
            cur.accumulator = (base < 0 ? 0 : base) << cur.shift;
        }
        if (mode == Swap) {
            osc_[osc ^ 1].control &= ~1; // enable pair
            osc_[osc ^ 1].accumulator = 0;
        }
        if (cur.control & 8) { // interrupt enabled
            irq_(osc);
        }
    }

    std::function<void(int)> irq_;
    std::vector<double> waveTable_;
    std::array<Oscillator, 32> osc_;
    int enabled_ = 0;
    const uint32_t waveSizes_[8] = {0x100,  0x200,  0x400,  0x800,
                                    0x1000, 0x2000, 0x4000, 0x8000};
    const uint32_t waveMasks_[8] = {0x1ff00, 0x1fe00, 0x1fc00, 0x1f800,
                                    0x1f000, 0x1e000, 0x1c000, 0x18000};
};

// Per-semitone DOC frequency table (from player.ts / ss2wav.c).
const uint16_t FREQUENCIES[] = {
    0x0000, 0x0016, 0x0017, 0x0018, 0x001a, 0x001b, 0x001d, 0x001e, 0x0020,
    0x0022, 0x0024, 0x0026, 0x0029, 0x002b, 0x002e, 0x0031, 0x0033, 0x0036,
    0x003a, 0x003d, 0x0041, 0x0045, 0x0049, 0x004d, 0x0052, 0x0056, 0x005c,
    0x0061, 0x0067, 0x006d, 0x0073, 0x007a, 0x0081, 0x0089, 0x0091, 0x009a,
    0x00a3, 0x00ad, 0x00b7, 0x00c2, 0x00ce, 0x00d9, 0x00e6, 0x00f4, 0x0102,
    0x0112, 0x0122, 0x0133, 0x0146, 0x015a, 0x016f, 0x0184, 0x019b, 0x01b4,
    0x01ce, 0x01e9, 0x0206, 0x0225, 0x0246, 0x0269, 0x028d, 0x02b4, 0x02dd,
    0x0309, 0x0337, 0x0368, 0x039c, 0x03d3, 0x040d, 0x044a, 0x048c, 0x04d1,
    0x051a, 0x0568, 0x05ba, 0x0611, 0x066e, 0x06d0, 0x0737, 0x07a5, 0x081a,
    0x0895, 0x0918, 0x09a2, 0x0a35, 0x0ad0, 0x0b75, 0x0c23, 0x0cdc, 0x0d9f,
    0x0e6f, 0x0f4b, 0x1033, 0x112a, 0x122f, 0x1344, 0x1469, 0x15a0, 0x16e9,
    0x1846, 0x19b7, 0x1b3f, 0x1cde, 0x1e95, 0x2066, 0x2254, 0x245e, 0x2688};
constexpr int FREQ_COUNT = sizeof(FREQUENCIES) / sizeof(FREQUENCIES[0]);

} // namespace

class SoundSmithPlayer : public ChipPlayer
{
public:
    SoundSmithPlayer(const std::vector<uint8_t>& song,
                     const std::vector<uint8_t>& wavebank,
                     const std::string& fileName)
        : es5503_([this](int osc) { irq(osc); })
    {
        if (!looksLikeSong(song.data(), song.size(), song.size())) {
            throw player_exception("Not a SoundSmith song");
        }
        if (wavebank.size() >= 4 && memcmp(wavebank.data(), "GSWV", 4) == 0) {
            // GSWV-packed wavebanks (used by a handful of demos) aren't produced
            // on Modland and would need a different unpacker; decline cleanly.
            throw player_exception("SoundSmith GSWV wavebank unsupported");
        }

        loadWavebank(wavebank);
        loadSong(song);

        // Mirror the player init: halt every oscillator, then set up osc 30 as a
        // free-running interrupt timer that clocks the tracker at ~50Hz.
        for (int i = 0; i < 32; i++) {
            es5503_.resetOsc(i);
        }
        es5503_.setFrequency(30, 0xfa);
        es5503_.setVolume(30, 0);
        es5503_.setPointer(30, 0);
        es5503_.setSize(30, 0);
        es5503_.setEnabled(0x3c); // 30 oscillators
        es5503_.setControl(30, 8); // free run + interrupts, not halted

        timer_ = tempo_ - 1;
        curPat_ = 0;
        curRow_ = 0;
        rowOffset_ = orders_.empty() ? 0 : orders_[0];

        std::string title = utils::path_basename(fileName);
        setMeta("title", title, "songs", 1, "startSong", 0, "format",
                "SoundSmith", "channels", 14);
    }

    // The host plays our samples straight into a fixed 44100 Hz device and does
    // NOT consult getHZ() (every other plugin renders at 44100), so we must emit
    // 44100 too. The DOC is emulated at its native 26320 Hz and the resulting
    // stream is linearly resampled to 44100 here; reporting the chip rate instead
    // made tunes play ~1.68x too fast and pitched-up.
    static constexpr double NATIVE_HZ = 26320.0;
    static constexpr double OUTPUT_HZ = 44100.0;

    int getHZ() override { return static_cast<int>(OUTPUT_HZ); }

    int getSamples(int16_t* target, int noSamples) override
    {
        int frames = noSamples / 2;
        int produced = 0;

        // Prime the two native frames we interpolate between.
        if (!primed_) {
            sourceEnded_ = !nextNative(s0l_, s0r_);
            if (!nextNative(s1l_, s1r_)) {
                s1l_ = s0l_;
                s1r_ = s0r_;
                sourceEnded_ = true;
            }
            primed_ = true;
        }

        const double step = NATIVE_HZ / OUTPUT_HZ; // native frames per output frame
        for (int i = 0; i < frames; i++) {
            if (sourceEnded_) {
                break;
            }
            double l = s0l_ + (s1l_ - s0l_) * phase_;
            double r = s0r_ + (s1r_ - s0r_) * phase_;
            target[produced++] = toS16(l);
            target[produced++] = toS16(r);

            phase_ += step;
            while (phase_ >= 1.0) {
                phase_ -= 1.0;
                s0l_ = s1l_;
                s0r_ = s1r_;
                double nl = 0.0;
                double nr = 0.0;
                if (nextNative(nl, nr)) {
                    s1l_ = nl;
                    s1r_ = nr;
                } else {
                    sourceEnded_ = true;
                }
            }
        }

        // Signal end-of-song (so the playlist advances) once fully drained.
        if (produced == 0 && sourceEnded_) {
            return -1;
        }
        return produced;
    }

    bool seekTo(int song, int /*seconds*/) override { return song == 0; }

private:
    // Produce one native-rate (26320 Hz) stereo frame. Returns false once the
    // tracker has reached the end of the order list (mirrors the old top-of-loop
    // stopped_ check: no frame is emitted after the song ends).
    bool nextNative(double& l, double& r)
    {
        if (stopped_) {
            return false;
        }
        es5503_.tick();
        l = 0.0;
        r = 0.0;
        es5503_.render(l, r);
        return true;
    }

    static int16_t toS16(double v)
    {
        double s = v * 32000.0;
        if (s > 32767.0) {
            s = 32767.0;
        } else if (s < -32768.0) {
            s = -32768.0;
        }
        return static_cast<int16_t>(s);
    }

    // Regular (non-GSWV) wavebank: numInst, 64KB sound RAM, then per-instrument
    // 12-byte split records (0x5c stride) and the compact (tuning) table.
    void loadWavebank(const std::vector<uint8_t>& wb)
    {
        numInst_ = wb.empty() ? 0 : wb[0];

        size_t ramLen = wb.size() > 2 ? wb.size() - 2 : 0;
        es5503_.setRam(wb.data() + 2, ramLen);

        // Each stored record is 12 bytes; pad with a trailing zero record so the
        // split-point walks below can read one entry past the end harmlessly.
        instruments_.assign((numInst_ + 1) * INST_REC, 0);
        for (int i = 0; i < numInst_; i++) {
            size_t src = 0x10022 + static_cast<size_t>(i) * 0x5c;
            for (int b = 0; b < INST_REC; b++) {
                instruments_[i * INST_REC + b] =
                    (src + b < wb.size()) ? wb[src + b] : 0;
            }
        }

        size_t ct = 0x10022 + static_cast<size_t>(numInst_) * 0x5c + 0x3c;
        for (int i = 0; i < 16; i++) {
            size_t off = ct + static_cast<size_t>(i) * 2;
            compactTable_[i] = (off + 1 < wb.size())
                                   ? (wb[off] | (wb[off + 1] << 8))
                                   : 0;
        }
    }

    void loadSong(const std::vector<uint8_t>& s)
    {
        uint16_t blockLen = rd16(s, 6);
        tempo_ = rd16(s, 8);

        for (int i = 0; i < 15; i++) {
            volTable_[i] = rd16(s, 0x2c + static_cast<size_t>(i) * 0x1e);
        }

        int songLen = (0x1d6 < s.size()) ? (s[0x1d6] & 0xff) : 0;
        orders_.clear();
        for (int i = 0; i < songLen; i++) {
            size_t off = 0x1d8 + static_cast<size_t>(i);
            uint8_t pat = (off < s.size()) ? s[off] : 0;
            orders_.push_back(pat * 64 * 14);
        }

        auto slice = [&](size_t off, size_t len) {
            std::vector<uint8_t> out(len, 0);
            for (size_t i = 0; i < len && off + i < s.size(); i++) {
                out[i] = s[off + i];
            }
            return out;
        };
        notes_ = slice(0x258, blockLen);
        effects1_ = slice(0x258 + blockLen, blockLen);
        effects2_ = slice(0x258 + static_cast<size_t>(blockLen) * 2, blockLen);

        size_t st = 0x258 + static_cast<size_t>(blockLen) * 3;
        for (int i = 0; i < 16; i++) {
            stereoTable_[i] = rd16(s, st + static_cast<size_t>(i) * 2);
        }
    }

    uint8_t noteAt(size_t off) const
    {
        return off < notes_.size() ? notes_[off] : 0;
    }
    uint8_t fx1At(size_t off) const
    {
        return off < effects1_.size() ? effects1_[off] : 0;
    }
    uint8_t fx2At(size_t off) const
    {
        return off < effects2_.size() ? effects2_[off] : 0;
    }
    uint16_t freqFor(int tone, int inst) const
    {
        if (tone < 0 || tone >= FREQ_COUNT) {
            return 0;
        }
        int shift = (inst >= 0 && inst < 16) ? compactTable_[inst] : 0;
        return FREQUENCIES[tone] >> shift;
    }

    // Interrupt handler. osc 30 is the tracker timer; every other interrupting
    // oscillator just gets unhalted (its sample keeps looping). Ported from the
    // irq() in player.ts / ss2wav.c.
    void irq(int osc)
    {
        if (osc != 30) {
            es5503_.go(osc);
            return;
        }

        timer_++;
        if (timer_ == tempo_) {
            timer_ = 0;
            for (int ch = 0; ch < 14; ch++) {
                uint8_t semitone = noteAt(rowOffset_);
                if (semitone == 0 || (semitone & 0x80)) {
                    rowOffset_++;
                    if (semitone == 0x80) {
                        es5503_.setControl(ch * 2, 1);
                        es5503_.setControl(ch * 2 + 1, 1);
                    } else if (semitone == 0x81) {
                        curRow_ = 0x3f;
                    }
                    continue;
                }

                uint8_t fx = fx1At(rowOffset_);
                if (fx & 0xf0) { // change instrument
                    curInst_[ch] = (fx >> 4) - 1;
                }
                int inst = curInst_[ch];
                int volume = (inst >= 0 && inst < 15) ? (volTable_[inst] >> 1) : 0;
                fx &= 0xf;
                if (fx == 0) {
                    arpeggio_[ch] = fx2At(rowOffset_);
                    tone_[ch] = semitone;
                } else {
                    arpeggio_[ch] = 0;
                    if (fx == 3) {
                        volume = fx2At(rowOffset_) >> 1;
                        setPairVolume(ch, volume);
                    } else if (fx == 6) {
                        volume -= fx2At(rowOffset_) >> 1;
                        if (volume < 0) {
                            volume = 0;
                        }
                        setPairVolume(ch, volume);
                    } else if (fx == 5) {
                        volume += fx2At(rowOffset_) >> 1;
                        if (volume > 0x7f) {
                            volume = 0x7f;
                        }
                        setPairVolume(ch, volume);
                    } else if (fx == 0xf) {
                        tempo_ = fx2At(rowOffset_);
                    }
                }

                int addr = ch * 2;
                es5503_.stop(addr);
                es5503_.stop(addr + 1);
                if (inst >= 0 && inst < numInst_) {
                    triggerNote(ch, inst, semitone, volume);
                }
                rowOffset_++;
            }

            curRow_++;
            if (curRow_ < 0x40) {
                return;
            }
            curRow_ = 0;
            curPat_++;
            if (curPat_ < static_cast<int>(orders_.size())) {
                rowOffset_ = orders_[curPat_];
                return;
            }
            stopped_ = true;
            return;
        }

        // Between rows: apply arpeggio.
        for (int ch = 0; ch < 14; ch++) {
            uint8_t a = arpeggio_[ch];
            if (!a) {
                continue;
            }
            switch (timer_ % 6) {
            case 1:
            case 4:
                tone_[ch] += a >> 4;
                break;
            case 2:
            case 5:
                tone_[ch] += a & 0xf;
                break;
            case 0:
            case 3:
                tone_[ch] -= a >> 4;
                tone_[ch] -= a & 0xf;
                break;
            }
            uint16_t freq = freqFor(tone_[ch], ch);
            es5503_.setFrequency(ch * 2, freq);
            es5503_.setFrequency(ch * 2 + 1, freq);
        }
    }

    void setPairVolume(int ch, int volume)
    {
        es5503_.setVolume(ch * 2, volume);
        es5503_.setVolume(ch * 2 + 1, volume);
    }

    // Resolve the two split records (osc A and its pair B) for a semitone and
    // arm the oscillator pair. Walks are bounded to the stored 12-byte record.
    void triggerNote(int ch, int inst, uint8_t semitone, int volume)
    {
        const uint8_t* rec = &instruments_[inst * INST_REC];
        int x = 0;
        while (x < INST_REC && rec[x] < semitone) {
            x += 6;
        }
        uint8_t oscAptr = recByte(rec, x + 1);
        uint8_t oscAsiz = recByte(rec, x + 2);
        uint8_t oscActl = recByte(rec, x + 3) & 0xf;
        if (stereoTable_[ch]) {
            oscActl |= 0x10;
        }
        while (x < INST_REC && rec[x] != 0x7f) {
            x += 6;
        }
        x += 6; // skip the 0x7f terminator
        while (x < INST_REC && rec[x] < semitone) {
            x += 6;
        }
        uint8_t oscBptr = recByte(rec, x + 1);
        uint8_t oscBsiz = recByte(rec, x + 2);
        uint8_t oscBctl = recByte(rec, x + 3) & 0xf;
        if (stereoTable_[ch]) {
            oscBctl |= 0x10;
        }

        uint16_t freq = freqFor(semitone, inst);
        int addr = ch * 2;
        es5503_.setFrequency(addr, freq);
        es5503_.setFrequency(addr + 1, freq);
        es5503_.setVolume(addr, volume);
        es5503_.setVolume(addr + 1, volume);
        es5503_.setPointer(addr, oscAptr);
        es5503_.setPointer(addr + 1, oscBptr);
        es5503_.setSize(addr, oscAsiz);
        es5503_.setSize(addr + 1, oscBsiz);
        es5503_.setControl(addr, oscActl);
        es5503_.setControl(addr + 1, oscBctl);
    }

    static uint8_t recByte(const uint8_t* rec, int i)
    {
        return (i >= 0 && i < INST_REC) ? rec[i] : 0;
    }

    static constexpr int INST_REC = 12;

    ES5503 es5503_;

    std::vector<uint8_t> notes_;
    std::vector<uint8_t> effects1_;
    std::vector<uint8_t> effects2_;
    std::vector<int> orders_;
    std::vector<uint8_t> instruments_;
    uint16_t volTable_[15] = {0};
    uint16_t compactTable_[16] = {0};
    uint16_t stereoTable_[16] = {0};
    uint8_t curInst_[16] = {0};
    uint8_t arpeggio_[16] = {0};
    uint8_t tone_[16] = {0};

    int numInst_ = 0;
    int tempo_ = 0;
    int timer_ = 0;
    int curRow_ = 0;
    int curPat_ = 0;
    size_t rowOffset_ = 0;
    bool stopped_ = false;

    // 26320 Hz -> 44100 Hz linear resampler state.
    bool primed_ = false;
    bool sourceEnded_ = false;
    double phase_ = 0.0;
    double s0l_ = 0.0, s0r_ = 0.0;
    double s1l_ = 0.0, s1r_ = 0.0;
};

namespace {

// Append ".W" (Modland's uppercase wavebank extension) to the song path. The
// loader tries this and the lower-case variant.
std::string wavebankName(const std::string& songPath)
{
    return songPath + ".W";
}

std::vector<uint8_t> readWavebank(const std::string& songPath)
{
    for (const auto& ext : {".W", ".w"}) {
        utils::File f{songPath + ext};
        if (f.exists()) {
            return f.readAll();
        }
    }
    return {};
}

} // namespace

bool SoundSmithPlugin::canHandle(const std::string& name)
{
    // Identify by the song's own header structure (blockLen multiple of 896 +
    // tables fit + sane order count). The 6-byte ASCII signature is not a fixed
    // magic, and the ".W" wavebank is fetched as a secondary file so it may not
    // be present yet when this is first called -- hence a content-only check.
    FILE* fp = fopen(name.c_str(), "rb");
    if (fp == nullptr) {
        return false;
    }
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint8_t hdr[SONGLEN_OFFSET + 1];
    size_t n = fread(hdr, 1, sizeof(hdr), fp);
    fclose(fp);
    return fileSize > 0 && looksLikeSong(hdr, n, static_cast<size_t>(fileSize));
}

std::vector<std::string> SoundSmithPlugin::getSecondaryFiles(const std::string& name)
{
    // The wavebank lives next to the song as "<song>.W"; return the bare name so
    // the host fetches it into the same directory.
    return {utils::path_filename(wavebankName(name))};
}

std::set<std::string> SoundSmithPlugin::getSupportedExtensions() const
{
    // SoundSmith songs are bare-named (no extension) on Modland; routing is by
    // the "SONGOK" magic in canHandle(). The ".W" here documents the companion.
    return {"w"};
}

ChipPlayer* SoundSmithPlugin::fromFile(const std::string& fileName)
{
    auto song = utils::File(fileName).readAll();
    auto wavebank = readWavebank(fileName);
    if (wavebank.empty()) {
        throw player_exception("SoundSmith wavebank (.W) not found for " +
                               fileName);
    }
    return new SoundSmithPlayer{song, wavebank, fileName};
}

} // namespace musix

extern "C" void soundsmithplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::SoundSmithPlugin>();
    });
}
