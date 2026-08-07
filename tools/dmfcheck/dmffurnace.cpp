// dmffurnace -- render a .dmf through the EXISTING Furnace-based dmfplugin.
//
// This is the reference ("A") side of the A/B. It links the plus build's
// already-compiled libdmfplugin.a and drives it through the public
// ChipPlugin/ChipPlayer interface only.
//
// Furnace is treated strictly as a BLACK BOX here, and that is deliberate: the
// clean-room argument for dmfcrplugin depends on its author not having read
// Furnace's DMF loader or its Genesis platform code. Instrumenting Furnace's
// internals to get a register trace -- which would be a stronger oracle than
// comparing audio, and is what tools/mdxtrace does for MDX -- would mean
// reading exactly the code we must not read. So we compare rendered audio.
//
//   dmffurnace <in.dmf> <out.wav> [seconds]

#include "../../external/musicplayer/src/chipplugin.h"
#include "../../external/musicplayer/src/chipplayer.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

// Provided by libdmfplugin.a.
extern "C" void dmfplugin_register();

// ChipPlugin::createPlugins() calls this to populate the constructor list. The
// real one (plugin_register) pulls in every plugin in the build; here we want
// dmfplugin and nothing else, so we supply our own definition.
void register_plugins()
{
    dmfplugin_register();
}

namespace {

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

int main(int argc, char** argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: dmffurnace <in.dmf> <out.wav> [seconds]\n");
        return 2;
    }
    double seconds = argc > 3 ? atof(argv[3]) : 30.0;

    musix::ChipPlugin::createPlugins("");
    auto& plugins = musix::ChipPlugin::getPlugins();
    if (plugins.empty()) {
        fprintf(stderr, "dmfplugin did not register\n");
        return 1;
    }
    auto plugin = plugins.front();

    musix::ChipPlayer* player = nullptr;
    try {
        player = plugin->fromFile(argv[1]);
    } catch (const musix::player_exception& e) {
        fprintf(stderr, "furnace load failed: %s\n", e.what());
        return 1;
    }
    if (player == nullptr) {
        fprintf(stderr, "furnace returned no player\n");
        return 1;
    }

    int rate = player->getHZ();
    if (rate <= 0) { rate = 44100; }

    int frames = static_cast<int>(seconds * rate);
    std::vector<int16_t> chunk(4096);
    std::vector<int16_t> pcm;
    pcm.reserve(static_cast<size_t>(frames) * 2);

    int done = 0;
    while (done < frames) {
        int want = (frames - done) * 2;
        if (want > static_cast<int>(chunk.size())) { want = static_cast<int>(chunk.size()); }
        int got = player->getSamples(chunk.data(), want);
        if (got <= 0) { break; } // SONG_END
        pcm.insert(pcm.end(), chunk.begin(), chunk.begin() + got);
        done += got / 2;
    }
    delete player;

    // Pad to the requested length so both sides of the A/B are the same size.
    pcm.resize(static_cast<size_t>(frames) * 2, 0);

    if (!writeWav(argv[2], pcm, rate)) {
        fprintf(stderr, "cannot write %s\n", argv[2]);
        return 1;
    }
    printf("%s -> %s (%.1fs @ %d Hz, furnace)\n", argv[1], argv[2], seconds, rate);
    return 0;
}
