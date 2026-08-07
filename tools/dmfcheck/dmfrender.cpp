// dmfrender -- render a Genesis .dmf through the clean-room player to a WAV.
//
// Development harness only; never linked into either app. Used to produce the
// "B" side of the A/B against Furnace (whose "A" side comes out of the plus
// build's own dmfplugin, driven as a black box -- see README.md on why Furnace
// is never read or instrumented here).
//
//   dmfrender <in.dmf> <out.wav> [seconds]

#include "../../external/musicplayer/src/plugins/dmfcrplugin/dmf_file.h"
#include "../../external/musicplayer/src/plugins/dmfcrplugin/dmf_player.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <map>

namespace {

std::vector<uint8_t> readFile(const std::string& p)
{
    std::ifstream in(p, std::ios::binary);
    if (!in) { return {}; }
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
}

void put32(std::vector<uint8_t>& v, uint32_t x)
{
    v.push_back(x & 0xFF);
    v.push_back((x >> 8) & 0xFF);
    v.push_back((x >> 16) & 0xFF);
    v.push_back((x >> 24) & 0xFF);
}

void put16(std::vector<uint8_t>& v, uint16_t x)
{
    v.push_back(x & 0xFF);
    v.push_back((x >> 8) & 0xFF);
}

bool writeWav(const std::string& path, const std::vector<int16_t>& pcm, int rate)
{
    std::vector<uint8_t> h;
    uint32_t dataBytes = static_cast<uint32_t>(pcm.size() * 2);
    const char* riff = "RIFF";
    h.insert(h.end(), riff, riff + 4);
    put32(h, 36 + dataBytes);
    const char* wave = "WAVEfmt ";
    h.insert(h.end(), wave, wave + 8);
    put32(h, 16);
    put16(h, 1);
    put16(h, 2);
    put32(h, static_cast<uint32_t>(rate));
    put32(h, static_cast<uint32_t>(rate * 4));
    put16(h, 4);
    put16(h, 16);
    const char* data = "data";
    h.insert(h.end(), data, data + 4);
    put32(h, dataBytes);

    std::ofstream o(path, std::ios::binary);
    if (!o) { return false; }
    o.write(reinterpret_cast<const char*>(h.data()), h.size());
    o.write(reinterpret_cast<const char*>(pcm.data()), dataBytes);
    return true;
}

} // namespace

// Summarise what a module actually contains, so a silent render can be traced
// to the feature that produced it rather than guessed at.
static int info(const char* path)
{
    auto raw = readFile(path);
    if (raw.empty()) {
        fprintf(stderr, "cannot read %s\n", path);
        return 1;
    }
    dmfcr::Module m;
    std::string err;
    if (!dmfcr::loadDmf(raw.data(), raw.size(), m, err)) {
        fprintf(stderr, "load failed: %s\n", err.c_str());
        return 1;
    }
    printf("ver 0x%02X sys 0x%02X  \"%s\" / \"%s\"\n", m.version, m.system,
           m.songName.c_str(), m.songAuthor.c_str());
    printf("channels %d  rowsPerPattern %u  matrixRows %d  timeBase %d  tick %d/%d  %s\n",
           m.totalChannels, m.rowsPerPattern, (int)m.matrixRows, (int)m.timeBase,
           (int)m.tickTime1, (int)m.tickTime2, m.framesMode ? "NTSC" : "PAL");
    printf("baseHz %.2f  instruments %zu  wavetables %zu  samples %zu\n",
           m.baseHz(), m.instruments.size(), m.wavetables.size(), m.samples.size());

    for (size_t i = 0; i < m.instruments.size(); i++) {
        const auto& ins = m.instruments[i];
        printf("  ins %2zu %-20s %s", i, ins.name.c_str(), ins.fm ? "FM " : "STD");
        if (ins.fm) {
            printf(" alg %d fb %d  TL", ins.alg, ins.fb);
            for (int o = 0; o < 4; o++) { printf(" %3d", ins.ops[o].tl); }
            printf("  AR");
            for (int o = 0; o < 4; o++) { printf(" %3d", ins.ops[o].ar); }
        } else {
            printf(" vol[%zu] arp[%zu] duty[%zu] wave[%zu]", ins.volume.values.size(),
                   ins.arpeggio.values.size(), ins.duty.values.size(),
                   ins.wavetable.values.size());
        }
        printf("\n");
    }

    // Per-channel note and effect census.
    for (int c = 0; c < m.totalChannels; c++) {
        int notes = 0, offs = 0, withIns = 0;
        std::map<int, int> effects;
        for (int o = 0; o < m.matrixRows; o++) {
            int pat = m.matrix[c][o];
            if (pat >= (int)m.channels[c].patterns.size()) { continue; }
            for (const auto& row : m.channels[c].patterns[pat].rows) {
                if (row.note == dmfcr::kNoteOff) { offs++; }
                else if (row.note != 0 || row.octave != 0) { notes++; }
                if (row.instrument >= 0) { withIns++; }
                for (int e = 0; e < m.channels[c].effectColumns; e++) {
                    if (row.effects[e].code >= 0) { effects[row.effects[e].code]++; }
                }
            }
        }
        printf("  ch %2d (%s) notes %4d off %3d ins %4d  eff:", c,
               c < 6 ? "FM " : "PSG", notes, offs, withIns);
        for (auto const& kv : effects) { printf(" %02X:%d", kv.first, kv.second); }
        printf("\n");
        // Structural effects change where the song goes next, so print them
        // with their values and positions -- an off-by-one here diverges
        // everything after it and is invisible in a per-effect count.
        for (int o = 0; o < m.matrixRows; o++) {
            int pat = m.matrix[c][o];
            if (pat >= (int)m.channels[c].patterns.size()) { continue; }
            auto const& rows = m.channels[c].patterns[pat].rows;
            for (size_t rw = 0; rw < rows.size(); rw++) {
                for (int e = 0; e < m.channels[c].effectColumns; e++) {
                    int code = rows[rw].effects[e].code;
                    int val = rows[rw].effects[e].value;
                    if (code == 0x0B || code == 0x0D || code == 0x09 ||
                        code == 0x0F) {
                        printf("        order %d (pat %d) row %zu: %02Xxx = %d "
                               "(0x%02X)\n",
                               o, pat, rw, code, val, val & 0xFF);
                    }
                }
            }
        }
    }
    return 0;
}

int main(int argc, char** argv)
{
    if (argc >= 3 && strcmp(argv[1], "--info") == 0) { return info(argv[2]); }
    if (argc < 3) {
        fprintf(stderr, "usage: dmfrender <in.dmf> <out.wav> [seconds]\n"
                        "       dmfrender --info <in.dmf>\n");
        return 2;
    }
    double seconds = argc > 3 ? atof(argv[3]) : 30.0;
    const int rate = 44100;

    auto raw = readFile(argv[1]);
    if (raw.empty()) {
        fprintf(stderr, "cannot read %s\n", argv[1]);
        return 1;
    }

    dmfcr::Module m;
    std::string err;
    if (!dmfcr::loadDmf(raw.data(), raw.size(), m, err)) {
        fprintf(stderr, "load failed: %s\n", err.c_str());
        return 1;
    }

    dmfcr::Player p;
    if (!p.init(m, rate, err)) {
        fprintf(stderr, "init failed: %s\n", err.c_str());
        return 1;
    }

    int frames = static_cast<int>(seconds * rate);
    std::vector<float> buf(2048 * 2);
    std::vector<int16_t> pcm;
    pcm.reserve(static_cast<size_t>(frames) * 2);

    int done = 0;
    while (done < frames) {
        int n = frames - done;
        if (n > 2048) { n = 2048; }
        p.render(buf.data(), n);
        for (int i = 0; i < n * 2; i++) {
            float s = buf[i];
            if (s > 1.0f) { s = 1.0f; }
            if (s < -1.0f) { s = -1.0f; }
            pcm.push_back(static_cast<int16_t>(s * 32767.0f));
        }
        done += n;
    }

    if (!writeWav(argv[2], pcm, rate)) {
        fprintf(stderr, "cannot write %s\n", argv[2]);
        return 1;
    }
    printf("%s: ver 0x%02X sys 0x%02X  %s / %s  -> %s (%.1fs)\n", argv[1],
           m.version, m.system, m.songName.c_str(), m.songAuthor.c_str(), argv[2],
           seconds);
    return 0;
}
