#include "TEDCRPlugin.h"
#include "ted_machine.h"

#include <coreutils/file.h>
#include <coreutils/utils.h>

#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace musix {

class TEDCRPlayer : public ChipPlayer
{
public:
    TEDCRPlayer(const std::vector<uint8_t>& data, const std::string& fileName)
        : machine_(44100)
    {
        if (!machine_.load(data.data(), data.size())) {
            throw player_exception("TED: not a 264-series tune");
        }
        setMeta("title", utils::path_basename(fileName), "format", "TED",
                "channels", 2, "songs", 1, "length", 0);
    }

    int getHZ() override { return 44100; }

    int getSamples(int16_t* target, int noSamples) override
    {
        int frames = noSamples / 2; // interleaved stereo pairs
        int got = machine_.generate(target, frames);
        if (got <= 0) {
            return -1; // playback backstop reached -- end of song, not a stall
        }
        return got * 2;
    }

private:
    tedcr::TedMachine machine_;
};

// `.prg` is a bare Commodore executable: two bytes of load address and nothing
// else that says which machine it is for, so the gate has to be a content check.
//
// $1001 is where BASIC starts on the 264 series, and a tune is a one-line BASIC
// stub that SYSes into machine code. Requiring both rejects the C64 files that
// share the extension -- they load at $0801 -- and rejects 264 files that are
// BASIC programs rather than machine code, whose music is BASIC 3.5 SOUND
// statements only an interpreter can play.
//
// $1000 is the same thing saved one byte lower, including the zero byte BASIC
// expects to sit below its program; the program itself is still at $1001. 24
// files in the library are stored that way and they are ordinary tunes.
//
// What it CANNOT separate is the VIC-20: an unexpanded VIC-20 also starts BASIC
// at $1001, so its .prg files look identical here. Those are handled further up,
// by MusicDatabase matching on the DB format string.
static bool looksLikeTedTune(const uint8_t* data, size_t len)
{
    if (data == nullptr || len < 16) {
        return false;
    }
    uint16_t load = static_cast<uint16_t>(data[0] | (data[1] << 8));
    if (load != 0x1001 && load != 0x1000) {
        return false;
    }
    return tedcr::basicSysTarget(data, len) != 0;
}

static const std::set<std::string> supported_ext{"prg"};

bool TEDCRPlugin::canHandle(const std::string& name)
{
    if (supported_ext.count(utils::path_extension(utils::toLower(name))) == 0) {
        return false;
    }
    utils::File f{name};
    if (!f.exists()) {
        return false;
    }
    auto data = f.readAll();
    return looksLikeTedTune(data.data(), data.size());
}

std::set<std::string> TEDCRPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* TEDCRPlugin::fromFile(const std::string& fileName)
{
    return new TEDCRPlayer{utils::File(fileName).readAll(), fileName};
}

} // namespace musix

extern "C" void tedcrplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::TEDCRPlugin>();
    });
}
