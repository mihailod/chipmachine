// Sound Tracker 1.1 modules (.st11) -- the UNCOMPILED form.
//
// Reclaimed from ZXTune (GPL-3), and the cheapest of the four because almost
// none of it is new: an .st11 is the same music as the .stc this plugin
// already plays, just stored the way the EDITOR held it rather than the way
// the compiler emitted it. So there is no sequencer here at all. This file
// compiles the editor's format into the compiled one and hands the result to
// the existing Sound Tracker player (zxay_stc.cpp), which is exactly what
// Sound Tracker's own "ST COMPILE" did on the Spectrum.
//
// Both layouts are documented in players/source/ST11FMT.txt (RAMSOFT, 1993).
// The uncompiled side is a fixed 3009-byte header -- 15 samples of 130 bytes,
// a 256-entry position map, 17 ornaments of 32 bytes, the delay and the
// pattern length -- followed by 576 bytes per pattern (64 rows x 3 channels x
// 3 bytes). The compiled side packs samples to 99 bytes, ornaments to 33, and
// turns each channel of each pattern into a byte-coded stream.
//
// The compilation itself follows Sergey Bulba's AY_Emul (ST12STC) under his
// attribution grant, because two things about it are conventions rather than
// consequences of the format: the note-name table, and the "strange behaviour
// of the native compiler" -- his words -- that emits code 0x82 for ornament 0
// selected via effect 1, where 0x70 would be the obvious encoding.

#include "zxay_native.h"

#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace musix::zxay {

namespace {

// --- uncompiled (editor) layout ---------------------------------------------
constexpr size_t kSmpBase = 0;      // 15 samples, 130 bytes each, numbered 1..15
constexpr size_t kSmpSize = 130;
constexpr size_t kPosBase = 1950;   // 256 entries of {pattern, transposition}
constexpr size_t kPosLenAt = 2462;  // song length in patterns
constexpr size_t kOrnBase = 2463;   // 17 ornaments, 32 signed bytes each
constexpr size_t kOrnSize = 32;
constexpr size_t kDelayAt = 3007;
constexpr size_t kPatLenAt = 3008;
constexpr size_t kPatBase = 3009;
constexpr size_t kPatSize = 576;
constexpr int kMaxPatterns = 64;

// Semitone offset of each note NAME, indexed 1..7 = A B C D E F G.
constexpr int kNoteName[8] = {0, 9, 11, 0, 2, 4, 5, 7};

uint8_t at(const std::vector<uint8_t>& d, size_t i)
{
    return i < d.size() ? d[i] : 0;
}

// One row of one channel: note byte, effect/sample byte, ornament/effect byte.
struct Cell
{
    uint8_t nt, esNum, eoNum;
};

Cell cellAt(const std::vector<uint8_t>& d, int patternIndex, int row, int chan)
{
    const size_t off =
        kPatBase + static_cast<size_t>(patternIndex) * kPatSize + row * 9 + chan * 3;
    return {at(d, off), at(d, off + 1), at(d, off + 2)};
}

// Channel streams are deduplicated: two channels that compile to the same
// bytes share one copy, which is what keeps a compiled module small.
class PatternPool
{
public:
    // Returns the byte offset of `s` within the pool, appending it if new.
    size_t add(const std::string& s)
    {
        size_t offset = 0;
        for (auto const& p : pool_) {
            if (p == s) {
                return offset;
            }
            offset += p.size();
        }
        pool_.push_back(s);
        return offset;
    }
    size_t bytes() const
    {
        size_t n = 0;
        for (auto const& p : pool_) {
            n += p.size();
        }
        return n;
    }
    const std::vector<std::string>& all() const { return pool_; }

private:
    std::vector<std::string> pool_;
};

// Compiles one channel of one pattern into the byte-coded stream the compiled
// player walks. Returns false on a note the format cannot express.
bool compileChannel(const std::vector<uint8_t>& d, int patternIndex, int patLen,
                    int chan, std::string& out, bool sampleUsed[16],
                    bool ornamentUsed[16])
{
    int empty = -1, sample = -1, ornament = -1, envType = -1, envPeriod = -1;

    // How many rows AFTER `row` are empty. The count is only emitted when it
    // CHANGES, so a run of evenly-spaced notes costs one skip code, not one
    // per note.
    auto calcEmpty = [&](int row) {
        int n = 0;
        for (int k = row + 1; k < patLen; k++) {
            if ((cellAt(d, patternIndex, k, chan).nt & 0xF0) == 0) {
                n++;
            } else {
                break;
            }
        }
        if (n != empty) {
            empty = n;
            out.push_back(static_cast<char>(0xA1 + empty));
        }
        return empty;
    };

    for (int row = 0; row < patLen;) {
        const Cell c = cellAt(d, patternIndex, row, chan);
        const int note = c.nt >> 4;
        if (note == 0) {
            row += calcEmpty(row);
            out.push_back(static_cast<char>(0x81)); // empty location
        } else {
            calcEmpty(row);

            const int s = c.esNum >> 4;
            if (s >= 1 && s <= 15 && s != sample) {
                sample = s;
                out.push_back(static_cast<char>(0x60 + s));
                sampleUsed[s] = true;
            }

            const int effect = c.esNum & 15;
            if (effect >= 7 && effect <= 14) {
                // Envelope: shape in the code, period in the byte after it.
                if (envType != effect || envPeriod != c.eoNum) {
                    ornament = -1;
                    envType = effect;
                    envPeriod = c.eoNum;
                    out.push_back(static_cast<char>(0x80 + effect));
                    out.push_back(static_cast<char>(envPeriod));
                }
            } else if (effect == 1 || effect == 15) {
                const int o = effect == 1 ? 0 : (c.eoNum & 15);
                if (o != ornament) {
                    envType = -1;
                    envPeriod = -1;
                    ornament = o;
                    // Sound Tracker's own compiler emits 0x82 here rather than
                    // 0x70 -- both select ornament 0 and the player accepts
                    // either, but matching it keeps compiled and uncompiled
                    // versions of the same tune byte-identical.
                    out.push_back(static_cast<char>(
                        (effect == 1 && o == 0) ? 0x82 : 0x70 + o));
                    ornamentUsed[o] = true;
                }
            }

            if ((note & 8) == 0) {
                const int octave = c.nt & 7;
                const int sharp = (c.nt & 8) != 0 ? 1 : 0;
                // B# and E# have no encoding: the next semitone up is a
                // different letter, so the editor should never produce them.
                if ((note == 2 || note == 5) && sharp != 0) {
                    return false;
                }
                if (note > 7) {
                    return false;
                }
                const int semitone = kNoteName[note] + octave * 12 + sharp;
                if (semitone < 0 || semitone > 0x5F) {
                    return false;
                }
                out.push_back(static_cast<char>(semitone));
            } else {
                out.push_back(static_cast<char>(0x80)); // rest
            }
            row += empty;
        }
        row++;
    }
    out.push_back(static_cast<char>(0xFF)); // end of channel
    return true;
}

} // namespace

// Compiles an uncompiled Sound Tracker 1.1 module into the compiled layout.
// Returns an empty vector if the module is malformed.
std::vector<uint8_t> compileSt1(const std::vector<uint8_t>& d)
{
    if (d.size() <= kPatBase || (d.size() - kPatBase) % kPatSize != 0) {
        return {};
    }
    const int patternsInFile = static_cast<int>((d.size() - kPatBase) / kPatSize);
    if (patternsInFile > kMaxPatterns + 1) {
        return {};
    }
    const int patLen = at(d, kPatLenAt);
    if (patLen < 1 || patLen > 64) {
        return {};
    }
    const int posLen = at(d, kPosLenAt);

    // Which patterns the position list references, and which merely exist.
    // A pattern is stored in the file for every number that EXISTS, in
    // ascending order, so the file index is a running count -- not the
    // pattern number.
    bool used[kMaxPatterns + 1] = {};
    bool exists[kMaxPatterns + 1] = {};
    int usedCount = 0, existsCount = 0;
    for (int i = 0; i < 256; i++) {
        int n = at(d, kPosBase + i * 2);
        if (n == 0) {
            return {};
        }
        n--;
        if (n > kMaxPatterns) {
            return {};
        }
        if (!used[n] && i <= posLen) {
            used[n] = true;
            usedCount++;
        }
        if (!exists[n]) {
            exists[n] = true;
            existsCount++;
        }
    }
    if (usedCount == 0) {
        return {};
    }
    // Some modules in the wild claim patterns past the last one they play.
    // Trim those before the file-order index is computed, or every pattern
    // after them reads from the wrong place.
    for (int i = kMaxPatterns; i >= 0; i--) {
        if (used[i]) {
            break;
        }
        if (exists[i]) {
            exists[i] = false;
            existsCount--;
        }
    }
    if (existsCount - 1 > patternsInFile - 1) {
        return {};
    }

    bool sampleUsed[16] = {};
    bool ornamentUsed[16] = {};
    ornamentUsed[0] = true;

    PatternPool pool;
    struct CompiledPattern
    {
        uint8_t number;
        size_t offset[3];
    };
    std::vector<CompiledPattern> compiled;

    int fileIndex = -1;
    for (int i = 0; i <= kMaxPatterns; i++) {
        if (!exists[i]) {
            continue;
        }
        fileIndex++;
        if (!used[i] || fileIndex >= patternsInFile) {
            continue;
        }
        CompiledPattern cp{static_cast<uint8_t>(i + 1), {0, 0, 0}};
        for (int c = 0; c < 3; c++) {
            std::string stream;
            if (!compileChannel(d, fileIndex, patLen, c, stream, sampleUsed,
                                ornamentUsed)) {
                return {};
            }
            cp.offset[c] = pool.add(stream);
        }
        compiled.push_back(cp);
    }
    if (compiled.empty()) {
        return {};
    }

    // --- emit the compiled module ------------------------------------------
    std::vector<uint8_t> out(27, 0);
    out[0] = at(d, kDelayAt);
    // The identifier the compiler writes; also what this plugin's detector
    // looks for, so a round-tripped module still identifies as Sound Tracker.
    static const char kId[] = "SONG BY ST COMPILE";
    std::memcpy(out.data() + 6, kId, sizeof(kId) - 1);

    for (int i = 1; i <= 15; i++) {
        if (!sampleUsed[i]) {
            continue;
        }
        const size_t smp = kSmpBase + static_cast<size_t>(i - 1) * kSmpSize;
        out.push_back(static_cast<uint8_t>(i));
        for (int j = 0; j < 32; j++) {
            const uint8_t vol = at(d, smp + j);
            const uint8_t noise = at(d, smp + 32 + j);
            const uint16_t tone = static_cast<uint16_t>(
                at(d, smp + 64 + j * 2) | (at(d, smp + 65 + j * 2) << 8));
            // Volume with the tone offset's high nibble above it; the noise
            // value with the offset's SIGN folded into bit 5.
            out.push_back(static_cast<uint8_t>((vol & 15) | ((tone & 0xF00) >> 4)));
            out.push_back(static_cast<uint8_t>((noise & 0xDF) |
                                               ((tone & 0x1000) >> 7)));
            out.push_back(static_cast<uint8_t>(tone & 0xFF));
        }
        out.push_back(at(d, smp + 128)); // repeat
        out.push_back(at(d, smp + 129)); // repeat length
    }

    const size_t positionsAt = out.size();
    out.push_back(static_cast<uint8_t>(posLen));
    for (int i = 0; i <= posLen; i++) {
        out.push_back(at(d, kPosBase + i * 2));
        out.push_back(at(d, kPosBase + i * 2 + 1));
    }

    const size_t ornamentsAt = out.size();
    for (int i = 0; i < 16; i++) {
        if (!ornamentUsed[i]) {
            continue;
        }
        out.push_back(static_cast<uint8_t>(i));
        for (size_t j = 0; j < kOrnSize; j++) {
            out.push_back(at(d, kOrnBase + i * kOrnSize + j));
        }
    }

    const size_t patternsAt = out.size();
    // Each table entry is 7 bytes and the table ends with a 0xFF, so the
    // streams begin right after both.
    const size_t streamsAt = patternsAt + compiled.size() * 7 + 1;
    for (auto const& cp : compiled) {
        out.push_back(cp.number);
        for (int c = 0; c < 3; c++) {
            const size_t abs = streamsAt + cp.offset[c];
            out.push_back(static_cast<uint8_t>(abs & 0xFF));
            out.push_back(static_cast<uint8_t>(abs >> 8));
        }
    }
    out.push_back(0xFF);
    for (auto const& s : pool.all()) {
        out.insert(out.end(), s.begin(), s.end());
    }
    if (out.size() > 0x10000) {
        return {};
    }

    out[1] = static_cast<uint8_t>(positionsAt & 0xFF);
    out[2] = static_cast<uint8_t>(positionsAt >> 8);
    out[3] = static_cast<uint8_t>(ornamentsAt & 0xFF);
    out[4] = static_cast<uint8_t>(ornamentsAt >> 8);
    out[5] = static_cast<uint8_t>(patternsAt & 0xFF);
    out[6] = static_cast<uint8_t>(patternsAt >> 8);
    return out;
}

std::unique_ptr<Source> createSt11Source(const std::vector<uint8_t>& data,
                                         int sampleRate)
{
    ZxayEntry entry{};
    if (!parseZxayContainer(data, "ST11", &entry)) {
        return nullptr;
    }
    // The song-data block is followed 8 bytes later by the module itself, and
    // the module's length is whatever is left, rounded down to whole patterns.
    const long body = entry.songData + 8;
    if (body < 0 || body >= static_cast<long>(data.size())) {
        return nullptr;
    }
    size_t len = data.size() - static_cast<size_t>(body);
    if (len < kPatBase + kPatSize) {
        return nullptr;
    }
    len = kPatBase + (len - kPatBase) / kPatSize * kPatSize;

    const std::vector<uint8_t> raw(data.begin() + body,
                                   data.begin() + body + len);
    const std::vector<uint8_t> stc = compileSt1(raw);
    if (stc.empty()) {
        return nullptr;
    }
    auto src = createStcSource(stc, sampleRate);
    if (src) {
        SongInfo info;
        info.title = zxayString(data, entry.songName);
        info.author = zxayString(data, entry.author);
        src->setInfo(info);
    }
    return src;
}

} // namespace musix::zxay
