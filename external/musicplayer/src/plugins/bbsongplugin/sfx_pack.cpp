#include "sfx_pack.h"
#include "sfx_player.h"

#include <algorithm>
#include <map>
#include <set>

namespace musix::bbsong {

namespace {

constexpr uint8_t BB_HOLD = 0xFF; // no note this row (extends current event)
constexpr uint8_t BB_REST = 0x82; // note off

// Encode one tone channel into the SFX byte stream. Validated byte-exact vs
// Beepola: event = [value][duration] where value is the note (passthrough) or
// 0x83 for a rest, and duration = rows * (21 - tempo). A 0xFF row extends the
// current event. A non-0xFF sustain byte emits [0x81][sustain] before the
// event. Stream ends with 0x80.
std::vector<uint8_t> encodeChannel(const uint8_t* notes, const uint8_t* sus,
                                   int len, int tempo)
{
    int mult = 21 - tempo;
    if (mult < 1) {
        mult = 1;
    }
    std::vector<uint8_t> out;
    int cur = -1; // -1 = rest, else note value
    int dur = 0;
    auto flush = [&]() {
        if (dur > 0) {
            out.push_back(cur < 0 ? 0x83 : static_cast<uint8_t>(cur));
            out.push_back(static_cast<uint8_t>((dur * mult) & 0xFF));
        }
    };
    for (int r = 0; r < len; r++) {
        uint8_t b = notes[r];
        uint8_t s = sus ? sus[r] : 0xFF;
        if (b == BB_HOLD && s == BB_HOLD) {
            dur++;
        } else {
            flush();
            if (s != BB_HOLD) {
                out.push_back(0x81);
                out.push_back(s);
            }
            if (b == BB_REST) {
                cur = -1;
                dur = 1;
            } else if (b == BB_HOLD) {
                dur = 1; // keep current note, sustain change only
            } else {
                cur = b;
                dur = 1;
            }
        }
    }
    flush();
    out.push_back(0x80);
    return out;
}

// Encode the percussion channel. Same shape as a tone channel but: the "no
// event" marker is 0x00/0xFF (extends the current event), the rest command is
// 0x85, and drum codes (0x81+) pass through directly. duration = rows*(21-tempo).
std::vector<uint8_t> encodePerc(const uint8_t* perc, int len, int tempo)
{
    int mult = 21 - tempo;
    if (mult < 1) {
        mult = 1;
    }
    std::vector<uint8_t> out;
    int cur = -1; // -1 = rest (0x85), else drum code
    int dur = 0;
    auto flush = [&]() {
        if (dur > 0) {
            out.push_back(cur < 0 ? 0x85 : static_cast<uint8_t>(cur));
            out.push_back(static_cast<uint8_t>((dur * mult) & 0xFF));
        }
    };
    for (int r = 0; r < len; r++) {
        uint8_t b = perc ? perc[r] : 0x00;
        if (b == 0x00 || b == 0xFF) {
            dur++;
        } else {
            flush();
            cur = b;
            dur = 1;
        }
    }
    flush();
    out.push_back(0x80);
    return out;
}

void putWord(std::vector<uint8_t>& v, size_t off, uint16_t w)
{
    v[off] = w & 0xFF;
    v[off + 1] = w >> 8;
}

} // namespace

std::vector<uint8_t> buildSfxImage(const Song& song)
{
    const uint16_t org = SFX_ORG;
    const int N = static_cast<int>(song.order.size());

    // Unique pattern indices, laid out in index order (matches Beepola).
    std::set<int> uset(song.order.begin(), song.order.end());
    std::vector<int> uniq(uset.begin(), uset.end());

    // Encode each unique pattern's two tone streams and an empty perc pattern.
    std::map<int, std::vector<uint8_t>> c1, c2, perc;
    for (int o : uniq) {
        if (o >= static_cast<int>(song.patterns.size())) {
            c1[o] = {0x80};
            c2[o] = {0x80};
            perc[o] = {0x85, 0x04, 0x80};
            continue;
        }
        const Pattern& p = song.patterns[o];
        int L = static_cast<int>(p.length);
        int tempo = static_cast<int>(p.tempo);
        int stride = L > 0 ? static_cast<int>(p.channelData.size()) / L : 0;
        auto chan = [&](int ch) -> const uint8_t* {
            return (ch < stride) ? p.channelData.data() + ch * L : nullptr;
        };
        c1[o] = encodeChannel(chan(0), chan(3), L, tempo); // note1 + sustain1
        c2[o] = encodeChannel(chan(1), chan(4), L, tempo); // note2 + sustain2
        perc[o] = encodePerc(chan(2), L, tempo);           // percussion
    }

    // --- layout (matches Beepola; validated byte-exact for tone+empty-perc) ---
    std::vector<uint8_t> img(SFX_PLAYER, SFX_PLAYER + SFX_PLAYER_LEN); // 0..0x2B3

    size_t percSeqPtrOff = img.size();
    img.push_back(0);
    img.push_back(0); // perc-seq pointer (filled later)

    size_t toneSeqOff = img.size();
    img.resize(img.size() + (N + 1) * 2, 0); // N pattern ptrs + 0x0000
    uint16_t toneLoop = static_cast<uint16_t>(org + toneSeqOff + song.loopStart * 2);
    img.push_back(toneLoop & 0xFF);
    img.push_back(toneLoop >> 8);

    // tone pattern data
    std::map<int, uint16_t> toneAddr;
    for (int o : uniq) {
        toneAddr[o] = static_cast<uint16_t>(org + img.size());
        uint16_t ch2 =
            static_cast<uint16_t>(org + img.size() + 2 + c1[o].size());
        img.push_back(ch2 & 0xFF);
        img.push_back(ch2 >> 8);
        img.insert(img.end(), c1[o].begin(), c1[o].end());
        img.insert(img.end(), c2[o].begin(), c2[o].end());
    }

    // perc sequence
    uint16_t percSeqAddr = static_cast<uint16_t>(org + img.size());
    putWord(img, percSeqPtrOff, percSeqAddr);
    size_t percSeqOff = img.size();
    img.resize(img.size() + (N + 1) * 2, 0); // N perc ptrs + 0x0000
    uint16_t percLoop = static_cast<uint16_t>(percSeqAddr + song.loopStart * 2);
    img.push_back(percLoop & 0xFF);
    img.push_back(percLoop >> 8);

    // perc pattern data, deduplicated by content
    std::map<std::vector<uint8_t>, uint16_t> percPool;
    std::map<int, uint16_t> percAddr;
    for (int o : uniq) {
        auto& key = perc[o];
        auto it = percPool.find(key);
        if (it == percPool.end()) {
            uint16_t a = static_cast<uint16_t>(org + img.size());
            percPool[key] = a;
            img.insert(img.end(), key.begin(), key.end());
            percAddr[o] = a;
        } else {
            percAddr[o] = it->second;
        }
    }

    // fill sequence pointers (per order entry)
    for (int i = 0; i < N; i++) {
        int o = song.order[i];
        putWord(img, toneSeqOff + i * 2,
                toneAddr.count(o) ? toneAddr[o] : toneLoop);
        putWord(img, percSeqOff + i * 2,
                percAddr.count(o) ? percAddr[o] : percSeqAddr);
    }

    return img;
}

} // namespace musix::bbsong
