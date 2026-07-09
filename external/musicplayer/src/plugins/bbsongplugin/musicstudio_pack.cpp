#include "musicstudio_pack.h"
#include "musicstudio_asm.h"

#include <array>
#include <cstdio>

namespace musix::bbsong {

namespace {

// Beepola MSD pattern data, channel-major, in the authoritative per-pattern
// order: tone1 note, tone2 note, percussion, ch1 additional (slide), ch2
// additional (slide).
enum { CH1_NOTE = 0, CH2_NOTE = 1, PERC = 2, CH1_SLIDE = 3, CH2_SLIDE = 4 };

constexpr uint8_t BB_HOLD = 0xFF;
constexpr uint8_t BB_REST = 0x82;

// Beepola note -> noteToPeriodTable index. Calibrated against the real MSD
// player: feeding periodTable[N+20] yields the correct pitch (note 30 -> C4
// 260Hz, note 24 -> F#3, note 36 -> F#4). NOTE_BASE=8 (the naive C-1=index2
// derivation) landed every note on an invalid period-0 entry -> constant tone.
constexpr int NOTE_BASE = 20;

// noteToPeriodTable, computed exactly as musicstudio.1te Init() does from the
// equal-tempered note frequencies. Index 2+octave*12+note holds the player
// period; periods of 1 or >255 are invalid (0).
const std::array<float, 12> NOTE_FREQ = {2093.0f, 2217.4f, 2349.2f, 2489.0f,
                                         2637.0f, 2793.8f, 2960.0f, 3136.0f,
                                         3322.4f, 3520.0f, 3729.2f, 3951.0f};

std::array<int, 128> makePeriodTable()
{
    std::array<int, 128> t{};
    const float cpuTime = 115.0f;
    const float cpuClock = 3500000.0f;
    float div = 64.0f;
    for (int octave = 0; octave < 8; octave++) {
        for (int note = 0; note < 12; note++) {
            int period =
                static_cast<int>((cpuClock / (cpuTime / 2.0f)) /
                                 (NOTE_FREQ[note] / div));
            if (period == 1 || period > 255) {
                period = 0;
            }
            t[2 + octave * 12 + note] = period;
        }
        div /= 2.0f;
    }
    t[0] = 0;
    t[1] = 0;
    return t;
}

struct Flat
{
    const Song& song;
    std::vector<std::pair<int, int>> rows;
    std::vector<int> patStride;

    explicit Flat(const Song& s) : song(s)
    {
        patStride.resize(song.patterns.size(), 0);
        for (size_t i = 0; i < song.patterns.size(); i++) {
            const auto& p = song.patterns[i];
            patStride[i] =
                p.length ? static_cast<int>(p.channelData.size() / p.length) : 0;
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

    uint8_t raw(int row, int chan) const
    {
        if (row < 0 || row >= length()) {
            return BB_HOLD;
        }
        int pat = rows[row].first;
        int lr = rows[row].second;
        if (chan >= patStride[pat]) {
            return BB_HOLD;
        }
        const auto& d = song.patterns[pat].channelData;
        size_t off = static_cast<size_t>(chan) * song.patterns[pat].length + lr;
        return off < d.size() ? d[off] : BB_HOLD;
    }

    int slide(int row, int chan) const
    {
        uint8_t b = raw(row, chan);
        return b == BB_HOLD ? 0 : (b & 0x0F);
    }

    int loopRow() const
    {
        int acc = 0;
        for (size_t i = 0; i < song.order.size(); i++) {
            if (static_cast<int>(i) == static_cast<int>(song.loopStart)) {
                return acc;
            }
            uint8_t idx = song.order[i];
            if (idx < song.patterns.size()) {
                acc += song.patterns[idx].length;
            }
        }
        return 0;
    }
};

} // namespace

std::string buildMusicStudioAsm(const Song& song)
{
    static const auto period = makePeriodTable();
    Flat flat(song);
    int len = flat.length();
    int loopOffset = flat.loopRow();

    int speed = 24;
    if (!song.patterns.empty() && !song.order.empty()) {
        speed = static_cast<int>(song.patterns[song.order[0]].tempo);
    }
    if (speed < 1) {
        speed = 1;
    }
    if (speed > 99) {
        speed = 99;
    }

    std::string out;
    out.reserve(64 * 1024);
    out += MUSICSTUDIO_ASM;
    out += "\n;compiled music data\n\nmusic_data\n";
    char hdr[32];
    snprintf(hdr, sizeof(hdr), "\tdb #%02x\n", speed & 0xFF);
    out += hdr;

    // Per row, two period bytes (channel 1, channel 2). A 0xFF (hold) note
    // sustains the previous channel period; otherwise period = table[note] -
    // slide, clamped to >=1 (matching Compile()).
    int prev1 = 1, prev2 = 1;
    for (int i = 0; i < len; i++) {
        if (i == loopOffset) {
            out += ".loop\n";
        }
        uint8_t b1 = flat.raw(i, CH1_NOTE);
        int n1;
        if (b1 == BB_HOLD || b1 == BB_REST) {
            n1 = prev1; // hold/rest sustains the previous period
        } else {
            int idx = (b1 & 0x7F) + NOTE_BASE;
            int p = (idx >= 0 && idx < 128) ? period[idx] : 0;
            n1 = p - flat.slide(i, CH1_SLIDE);
            if (n1 < 1) {
                n1 = 1;
            }
        }
        uint8_t b2 = flat.raw(i, CH2_NOTE);
        int n2;
        if (b2 == BB_HOLD || b2 == BB_REST) {
            n2 = prev2;
        } else {
            int idx = (b2 & 0x7F) + NOTE_BASE;
            int p = (idx >= 0 && idx < 128) ? period[idx] : 0;
            n2 = p - flat.slide(i, CH2_SLIDE);
            if (n2 < 1) {
                n2 = 1;
            }
        }
        prev1 = n1;
        prev2 = n2;
        char buf[32];
        snprintf(buf, sizeof(buf), "\tdb #%02x,#%02x\n", n1 & 0xFF, n2 & 0xFF);
        out += buf;
    }

    out += "\tdb #0\n\tdw .loop\n";
    return out;
}

} // namespace musix::bbsong
