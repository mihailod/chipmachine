#include "musicbox_pack.h"
#include "musicbox_asm.h"

#include <array>
#include <cstdio>

namespace musix::bbsong {

namespace {

// Beepola TMB pattern data is channel-major: [0]=tone1 note, [1]=tone2 note,
// [2]=drum (with mask in the upper bits), [3]/[4] unused in practice.
enum { CH1 = 0, CH2 = 1, PERC = 2 };

constexpr uint8_t BB_HOLD = 0xFF; // continue previous note (-> code 0x29)
constexpr uint8_t BB_REST = 0x82;

// Beepola note 0 = F#1, which is the lowest note of the Music Box player's
// 53-entry note table (index 14). Verified by calibration: feeding index 14+N
// produces the F#1+N pitch (idx26->97Hz, idx38->193Hz = an exact octave).
constexpr int NOTE_BASE = 14;

// noteToCode: 1tracker note index -> player timing code. Indices 14..66 are the
// 53 real pitches (codes 0xF4..0x28); everything else is 0x29 ("no change").
std::array<uint8_t, 128> makeNoteToCode()
{
    std::array<uint8_t, 128> t;
    t.fill(41); // 0x29
    for (int i = 0; i < 53; i++) {
        t[14 + i] = static_cast<uint8_t>((244 + i) & 255);
    }
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

    // 1tracker note index: 0 (= "continue", code 0x29) for hold/rest, else
    // note + NOTE_BASE.
    int noteIndex(int row, int chan) const
    {
        uint8_t b = raw(row, chan);
        if (b == BB_HOLD || b == BB_REST) {
            return 0;
        }
        int idx = b + NOTE_BASE;
        return (idx >= 0 && idx < 128) ? idx : 0;
    }

    // Drum byte (0 = none): low nibble = type 1..9, upper bits = noise mask,
    // matching the value Compile() assembles from its 5 drum columns.
    int drum(int row) const
    {
        uint8_t b = raw(row, PERC);
        return (b == 0x00 || b == BB_HOLD) ? 0 : b;
    }

    // Flat row index of the order-list loop point.
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

void hexb(std::string& out, int v)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "\tdb #%02x\n", v & 0xFF);
    out += buf;
}

} // namespace

std::string buildMusicBoxAsm(const Song& song)
{
    static const auto noteToCode = makeNoteToCode();
    Flat flat(song);
    int len = flat.length();
    int loopOffset = flat.loopRow();

    // Music Box has one global speed; use the first pattern's tempo (player
    // counts up from 255-speed, so larger tempo = faster). Clamp to 1..26.
    int speed = 20;
    if (!song.patterns.empty() && !song.order.empty()) {
        speed = static_cast<int>(song.patterns[song.order[0]].tempo);
    }
    if (speed < 1) {
        speed = 1;
    }
    if (speed > 26) {
        speed = 26;
    }

    std::string out;
    out.reserve(64 * 1024);
    out += MUSICBOX_ASM;
    out += "\n;compiled music data\n\nmusic_data\n";
    hexb(out, 255 - speed);
    out += "\tdw .ch1\n\tdw .ch2\n\tdw .ch1loop\n\tdw .ch2loop\n";

    // tone channel 1 (drums become tom / noise codes). A note plays on each
    // row; Beepola's hold/rest (0xFF/0x82) sustains the previous pitch (Music
    // Box has no per-row silence -- a held value keeps both channels toning,
    // and a true rest sentinel would instead mask the other channel).
    out += ".ch1\n";
    int last1 = 0; // 0 = no note yet -> noteToCode[0] (quiet) until first note
    for (int i = 0; i < len; i++) {
        if (i == loopOffset) {
            out += ".ch1loop\n";
        }
        uint8_t b1 = flat.raw(i, CH1);
        if (b1 != BB_HOLD && b1 != BB_REST) {
            int idx = b1 + NOTE_BASE;
            last1 = (idx >= 0 && idx < 128) ? idx : 0;
        }
        int note = noteToCode[last1];
        int drum = flat.drum(i);
        if (drum > 0) {
            if ((drum & 15) == 9) {
                note = 0xb4; // tom
            } else {
                note = 0xe4 + (((drum >> 4) ^ 15) & 0xFF); // noise with mask
            }
        }
        hexb(out, note);
    }
    out += "\tdb #40\n"; // end of channel 1

    // tone channel 2 (drums become a different noise code)
    out += ".ch2\n";
    int last2 = 0;
    for (int i = 0; i < len; i++) {
        if (i == loopOffset) {
            out += ".ch2loop\n";
        }
        uint8_t b2 = flat.raw(i, CH2);
        if (b2 != BB_HOLD && b2 != BB_REST) {
            int idx = b2 + NOTE_BASE;
            last2 = (idx >= 0 && idx < 128) ? idx : 0;
        }
        int note = noteToCode[last2];
        int drum = flat.drum(i) & 15;
        if (drum > 0 && drum < 9) {
            note = 0xff >> (drum - 1);
        }
        hexb(out, note);
    }
    out += "\tdb #40\n"; // end of channel 2

    return out;
}

} // namespace musix::bbsong
