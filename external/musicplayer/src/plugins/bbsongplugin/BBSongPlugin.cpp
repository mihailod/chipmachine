#include "BBSongPlugin.h"
#include "../../chipplayer.h"

#include "bbsong_parser.h"
#include "musicbox_pack.h"
#include "musicstudio_pack.h"
#include "phaser1_pack.h"
#include "sfx_pack.h"
#include "sfx_player.h"
#include "z80_machine.h"
#include "z80asm.h"

#include <coreutils/file.h>
#include <coreutils/log.h>
#include <coreutils/utils.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace musix {

namespace {

constexpr char BB_MAGIC[] = {'B', 'B', 'S', 'O', 'N', 'G', '\0'};

bool hasMagic(const uint8_t* data, size_t len)
{
    return len >= sizeof(BB_MAGIC) &&
           memcmp(data, BB_MAGIC, sizeof(BB_MAGIC)) == 0;
}

// Engines with a working packer today. Phaser1 (Shiru, PD) is first; Music Box
// and Music Studio follow. Other engines parse but throw from fromFile.
// Phaser1 (P1D/P1S) and Music Box (TMB) are calibrated and correct. Music
// Studio (MSD) packer exists (buildMusicStudioAsm) but its note/slide mapping
// isn't calibrated yet, so it's held out until fixed.
const std::set<std::string> playableEngines{"P1D", "P1S", "TMB", "MSD", "SFX"};

// Builds the engine's assembly for the song. Throws if the engine isn't wired.
std::string buildAsm(const bbsong::Song& song)
{
    if (song.engine == "P1D") {
        return bbsong::buildPhaser1Asm(song, /*synthDrums=*/false);
    }
    if (song.engine == "P1S") {
        return bbsong::buildPhaser1Asm(song, /*synthDrums=*/true);
    }
    if (song.engine == "TMB") {
        return bbsong::buildMusicBoxAsm(song);
    }
    if (song.engine == "MSD") {
        return bbsong::buildMusicStudioAsm(song);
    }
    throw player_exception("bbsong: engine '" + song.engine + "' not wired");
}

} // namespace

class BBSongPlayer : public ChipPlayer
{
public:
    BBSongPlayer(const std::vector<uint8_t>& data, const std::string& fileName)
    {
        if (!hasMagic(data.data(), data.size())) {
            throw player_exception("Not a .bbsong file");
        }
        bbsong::Song song = bbsong::parse(data.data(), data.size());

        if (playableEngines.count(song.engine) == 0) {
            // "unsupported" marks this as a graceful skip (a known Beepola
            // engine we don't decode yet), not a playback failure.
            throw player_exception("bbsong: engine '" + song.engine +
                                   "' unsupported (not yet wired)");
        }

        if (song.engine == "SFX") {
            // SFX uses Beepola's compiled bytecode format + a prebuilt player
            // blob (not the assembler): build the image and run it directly.
            std::vector<uint8_t> img = bbsong::buildSfxImage(song);
            machine.poke(bbsong::SFX_ORG, img.data(), img.size());
            machine.loadSpectrumRom();
            machine.start(bbsong::SFX_ORG);
        } else {
            // Engines assembled from vendored Z80 source + a generated data
            // block (Phaser1, Music Box, Music Studio).
            std::string asmText = buildAsm(song);
            bbsong::Z80Assembler as;
            int size = as.assemble(asmText, kOrg);
            if (as.error() || size <= 0) {
                throw player_exception("bbsong: assemble failed: " +
                                       as.errorMessage());
            }
            int begin = as.label("begin");
            if (begin < 0) {
                throw player_exception("bbsong: no 'begin' entry point");
            }
            machine.poke(0, as.image().data(), 0x10000);
            machine.loadSpectrumRom(); // engines call ROM routines (KEY-SCAN)
            machine.start(static_cast<uint16_t>(begin));
        }

        std::string title = song.title.empty() ? utils::path_basename(fileName)
                                                : song.title;
        setMeta("title", title, "composer", song.author, "format",
                "Beepola " + song.engine, "length", 0);
    }

    int getHZ() override { return 44100; }

    int getSamples(int16_t* target, int noSamples) override
    {
        // The machine renders mono; fan out to interleaved stereo.
        int frames = noSamples / 2;
        int got = machine.generate(target, frames, 44100, kAmplitude);
        if (got <= 0) {
            return 0; // song ended
        }
        for (int i = got - 1; i >= 0; i--) {
            int16_t s = target[i];
            target[i * 2] = s;
            target[i * 2 + 1] = s;
        }
        return got * 2;
    }

private:
    static constexpr uint16_t kOrg = 0x8000;
    static constexpr int16_t kAmplitude = 12000;
    bbsong::Z80Machine machine;
};

static const std::set<std::string> supported_ext{"bbsong"};

bool BBSongPlugin::canHandle(const std::string& name)
{
    if (supported_ext.count(utils::path_extension(utils::toLower(name))) == 0) {
        return false;
    }
    FILE* fp = fopen(name.c_str(), "rb");
    if (fp == nullptr) {
        return false;
    }
    uint8_t magic[sizeof(BB_MAGIC)];
    size_t n = fread(magic, 1, sizeof(magic), fp);
    fclose(fp);
    return n == sizeof(magic) && hasMagic(magic, n);
}

std::set<std::string> BBSongPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* BBSongPlugin::fromFile(const std::string& fileName)
{
    return new BBSongPlayer{utils::File(fileName).readAll(), fileName};
}

} // namespace musix

extern "C" void bbsongplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::BBSongPlugin>();
    });
}
