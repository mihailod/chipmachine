#include "phaser1_pack.h"
#include "phaser1_asm.h"

#include <cstdio>

namespace musix::bbsong {

namespace {

// --- mapping/tuning constants -------------------------------------------------
// Beepola stores P1D/P1S pattern data channel-major: for each pattern, `length`
// bytes per channel, channels in this order (inferred from real files):
//   0: channel 1 note (phasing/instrument channel)
//   1: channel 2 note (plain square)
//   2: percussion
//   3: instrument number (channel 1)
//   4: auxiliary (phase reset)
// In a note channel, 0xFF = hold/empty, 0x82 = rest (note off), other = note.
enum { CH1 = 0, CH2 = 1, PERC = 2, INSTR = 3, AUX = 4 };

constexpr uint8_t BB_HOLD = 0xFF; // no change this row
constexpr uint8_t BB_REST = 0x82; // note off

// Beepola note 0 = F#1; the 1tracker player counts semitones from C-1, so a
// Beepola note maps to player semitone (note + 6). Compile() computes the
// player byte as (value-14); we therefore pass (note + 20). Tunable if pitch
// comes out transposed.
constexpr int NOTE_BASE = 20;

// Frames-per-row scaling. Beepola's per-pattern "tempo" is used directly as the
// row delay (Compile multiplies a 1tracker speed by 4; Beepola's value is
// already in the player's frame units). Tunable if tempo is off.
int rowDelayFromTempo(int tempo) { return tempo > 0 ? tempo : 6; }

// Flattened view of the song: the order list expanded into a single row stream,
// with per-(row,channel) access to Beepola's channel-major bytes.
struct Flat
{
    const Song& song;
    std::vector<std::pair<int, int>> rows; // (patternIndex, localRow)
    std::vector<int> patStride;            // channels per pattern

    explicit Flat(const Song& s) : song(s)
    {
        patStride.resize(song.patterns.size(), 0);
        for (size_t i = 0; i < song.patterns.size(); i++) {
            const auto& p = song.patterns[i];
            patStride[i] = p.length ? static_cast<int>(p.channelData.size() /
                                                       p.length)
                                    : 0;
        }
        for (uint8_t idx : song.order) {
            if (idx >= song.patterns.size()) {
                continue;
            }
            for (uint32_t r = 0; r < song.patterns[idx].length; r++) {
                rows.emplace_back(idx, static_cast<int>(r));
            }
        }
    }

    int length() const { return static_cast<int>(rows.size()); }

    // Raw Beepola byte for (row, channel), or 0xFF (hold) if out of range.
    uint8_t raw(int row, int chan) const
    {
        if (row < 0 || row >= length()) {
            return BB_HOLD;
        }
        int pat = rows[row].first;
        int lr = rows[row].second;
        int stride = patStride[pat];
        if (chan >= stride) {
            return BB_HOLD;
        }
        const auto& d = song.patterns[pat].channelData;
        size_t off = static_cast<size_t>(chan) * song.patterns[pat].length + lr;
        return off < d.size() ? d[off] : BB_HOLD;
    }

    // Note column in the 1tracker convention Compile() expects: 0 = empty,
    // 1 = off, >=2 = note.
    int note(int row, int chan) const
    {
        uint8_t b = raw(row, chan);
        if (b == BB_HOLD) {
            return 0;
        }
        if (b == BB_REST) {
            return 1;
        }
        return b + NOTE_BASE;
    }

    // Drum number 1.. (0 = none). Beepola perc bytes are 0x8x with the drum in
    // the low bits; 0x00/0xFF = none.
    int drum(int row) const
    {
        uint8_t b = raw(row, PERC);
        if (b == 0x00 || b == BB_HOLD) {
            return 0;
        }
        return b & 0x7F;
    }

    // Speed for the row: the pattern tempo on the pattern's first row, else 0
    // (Compile() treats speed as sticky).
    int speed(int row) const
    {
        if (row < 0 || row >= length()) {
            return 0;
        }
        if (rows[row].second == 0) {
            return rowDelayFromTempo(
                static_cast<int>(song.patterns[rows[row].first].tempo));
        }
        return 0;
    }
};

void put(std::string& out, const char* s) { out += s; }
void putf(std::string& out, const char* fmt, int v)
{
    char buf[32];
    snprintf(buf, sizeof(buf), fmt, v);
    out += buf;
}

} // namespace

std::string buildPhaser1Asm(const Song& song, bool synthDrums)
{
    Flat flat(song);
    int songLength = flat.length();

    int loopStart = static_cast<int>(song.loopStart); // order-entry granularity
    // Convert the order-index loop point to a flat row index.
    int loopRowAbs = 0;
    {
        int acc = 0;
        for (size_t i = 0; i < song.order.size(); i++) {
            if (static_cast<int>(i) == loopStart) {
                loopRowAbs = acc;
                break;
            }
            uint8_t idx = song.order[i];
            if (idx < song.patterns.size()) {
                acc += song.patterns[idx].length;
            }
        }
    }
    bool hasLoop = true; // .bbsong songs loop by default

    std::string out;
    out.reserve(64 * 1024);

    put(out, PHASER1_COMMON_ASM);
    put(out, synthDrums ? PHASER1_SYNTH_ASM : PHASER1_DIGITAL_ASM);

    put(out, "\nmusicData\n dw .sequence\n");

    // Instruments: first cut emits a single default instrument (multiple=1,
    // detune=0, phase=0). Refined to read the :P1INSTR chunk once audio is
    // confirmed.
    put(out, " db 1\n dw 0\n db 0\n");

    put(out, ".sequence\n");

    // --- note stream: faithful port of phaser1.1te Compile() ---
    int speed = flat.speed(0);
    if (speed == 0) {
        speed = 6;
    }
    int emptyLen = 0;
    auto flushEmpty = [&]() {
        while (emptyLen > 0) {
            if (emptyLen > 117) {
                put(out, " db 117\n");
                emptyLen -= 117;
            } else {
                putf(out, " db %d\n", emptyLen);
                emptyLen = 0;
            }
        }
    };

    put(out, " db #fd,0\n"); // default instrument

    int row = 0;
    int len = songLength;
    int loopRow = hasLoop ? loopRowAbs : -1;
    int ins = 0;

    while (len > 0) {
        if (row == loopRow) {
            flushEmpty();
            put(out, " db #ff\n");
        }

        int n = flat.speed(row);
        if (n > 0 && n != speed) {
            speed = n;
        }

        int ch1note = flat.note(row, CH1);
        int ch1ins = 0; // instrument variety deferred; always default for now
        int ch2note = flat.note(row, CH2);
        int drum = flat.drum(row);

        if (ch1note > 0 || ch1ins > 0 || ch2note > 0 || drum > 0) {
            flushEmpty();
        }

        if (drum > 0) {
            putf(out, " db %d\n", 117 + drum);
        }

        if (ch1note == 0) {
            put(out, " db #fe\n");
        } else if (ch1note == 1) {
            put(out, " db #fc\n");
        } else {
            int nn = ch1note - 14;
            if (nn < 0) {
                nn = 0;
            }
            if (nn > 59) {
                nn = 59;
            }
            if (flat.raw(row, AUX) != BB_HOLD) {
                nn |= 0x40; // phase reset
            }
            putf(out, " db %d\n", nn | 0x80);
        }

        if (ch2note == 1) {
            put(out, " db #fc\n");
        } else if (ch2note > 1) {
            int nn = ch2note - 14;
            if (nn < 0) {
                nn = 0;
            }
            if (nn > 59) {
                nn = 59;
            }
            putf(out, " db %d\n", 0xc0 | nn);
        }

        int delay = speed;
        if (drum > 0) {
            delay -= 4;
        }
        if (delay < 1) {
            delay = 1;
        }
        emptyLen += delay;

        --len;
        ++row;
    }

    flushEmpty();
    if (!hasLoop) {
        put(out, " db #fc,#fc,#ff,117\n");
    }
    put(out, " db 0\n");

    (void)ins;
    return out;
}

} // namespace musix::bbsong
