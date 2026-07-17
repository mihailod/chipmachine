#include "VTPlugin.h"
#include "vt_machine.h"

#include <coreutils/file.h>
#include <coreutils/utils.h>

#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace musix {

class VTPlayer : public ChipPlayer
{
public:
    VTPlayer(const std::vector<uint8_t>& data, const std::string& fileName)
        : machine(44100)
    {
        if (!machine.init(data.data(), data.size())) {
            throw player_exception("Vic-Tracker: not a VIC-TRACKER tune");
        }
        setMeta("title", utils::path_basename(fileName), "format", "Vic-Tracker",
                "channels", 4, "songs", machine.subsongCount(), "length", 0);
    }

    int getHZ() override { return 44100; }

    int getSamples(int16_t* target, int noSamples) override
    {
        int frames = noSamples / 2; // interleaved stereo pairs
        int got = machine.generate(target, frames);
        if (got <= 0) {
            return 0; // play cap reached
        }
        return got * 2;
    }

private:
    victracker::VTMachine machine;
};

// True for a VIC-TRACKER image: a $3300-load VIC-20 PRG whose stripped body
// begins with the "T1" or "T0" tune magic.
static bool looksLikeVicTracker(const uint8_t* d, size_t len)
{
    return len >= 6 && d[0] == 0x00 && d[1] == 0x33 && d[2] == 'T' &&
           (d[3] == '1' || d[3] == '0');
}

static const std::set<std::string> supported_ext{"vt"};

bool VTPlugin::canHandle(const std::string& name)
{
    if (supported_ext.count(utils::path_extension(utils::toLower(name))) == 0) {
        return false;
    }
    utils::File f{name};
    if (!f.exists()) {
        return false;
    }
    auto data = f.readAll();
    return looksLikeVicTracker(data.data(), data.size());
}

std::set<std::string> VTPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* VTPlugin::fromFile(const std::string& fileName)
{
    return new VTPlayer{utils::File(fileName).readAll(), fileName};
}

} // namespace musix

extern "C" void victrackerplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::VTPlugin>();
    });
}
