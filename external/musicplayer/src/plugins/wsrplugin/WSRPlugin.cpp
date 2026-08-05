#include "WSRPlugin.h"

#include "../../chipplayer.h"

#include <coreutils/utils.h>

// The WonderSwan itself: ares' ISC-licensed V30MZ (vendored under v30mz/) plus
// chipmachine's own machine and APU (wswan/). This replaces the in_wsr replayer
// that used to sit here, which was GPL-2-or-later -- see wswan/README.md.
#include "wswan/ws_machine.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace musix {

namespace {
constexpr int WSR_HZ = 44100;
constexpr int WSR_NCH = 2;

// WSR stores neither a length nor a subsong count. Like foo_input_wsr (the
// reference player) we hand the host a default play length so it fades and
// advances instead of looping forever, and advertise a window of subsongs the
// user can step through with next/previous. These are heuristics, not values
// read from the file. The window is widened to always include the start song
// (e.g. Klonoa's first subsong is 26), since the footer's first-song index can
// sit well above a small fixed default.
constexpr int WSR_DEFAULT_LENGTH = 180;
constexpr uint32_t WSR_BROWSE_SONGS = 32;
constexpr uint32_t WSR_SONGS_MARGIN = 8;

constexpr long WSR_FOOTER_SIZE = 0x20;

// Read an entire file into a byte vector; false if it can't be opened/read.
bool readWhole(const std::string& path, std::vector<uint8_t>& out)
{
    FILE* f = fopen(path.c_str(), "rb");
    if (f == nullptr) { return false; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= WSR_FOOTER_SIZE) {
        fclose(f);
        return false;
    }
    out.resize(static_cast<size_t>(sz));
    size_t got = fread(out.data(), 1, static_cast<size_t>(sz), f);
    fclose(f);
    out.resize(got);
    return static_cast<long>(got) > WSR_FOOTER_SIZE;
}

// True if the buffer ends in the "WSRF" footer signature.
bool hasWsrFooter(const std::vector<uint8_t>& d)
{
    if (static_cast<long>(d.size()) <= WSR_FOOTER_SIZE) { return false; }
    const uint8_t* f = d.data() + (d.size() - WSR_FOOTER_SIZE);
    return f[0] == 'W' && f[1] == 'S' && f[2] == 'R' && f[3] == 'F';
}
} // namespace

class WSRPlayer : public ChipPlayer {
public:
    explicit WSRPlayer(const std::string& fileName)
    {
        std::vector<uint8_t> data;
        if (!readWhole(fileName, data)) {
            throw player_exception("WSR: cannot read " + fileName);
        }
        if (!hasWsrFooter(data)) {
            throw player_exception("WSR: missing WSRF footer");
        }

        machine_ = std::make_unique<wswan::Machine>();
        if (!machine_->load(data.data(), data.size(), WSR_HZ)) {
            throw player_exception("WSR: not a valid WonderSwan sound rip");
        }

        auto first = machine_->firstSong();
        machine_->reset(first);

        uint32_t songs = std::max<uint32_t>(
            WSR_BROWSE_SONGS, static_cast<uint32_t>(first) + WSR_SONGS_MARGIN);
        setMeta("format", "WonderSwan", "songs", songs, "startSong",
                static_cast<uint32_t>(first), "length", WSR_DEFAULT_LENGTH);
    }

    int getHZ() override { return WSR_HZ; }

    int getSamples(int16_t* target, int noSamples) override
    {
        if (machine_ == nullptr) { return -1; }
        // The host counts interleaved int16 values; the machine renders frames.
        int frames = noSamples / WSR_NCH;
        machine_->render(target, static_cast<unsigned>(frames));
        return frames * WSR_NCH;
    }

    bool seekTo(int song, int /*seconds*/) override
    {
        if (machine_ == nullptr || song < 0) { return false; }
        // The host's subsong index is the WonderSwan song number directly.
        machine_->reset(static_cast<unsigned>(song));
        return true;
    }

private:
    std::unique_ptr<wswan::Machine> machine_;
};

bool WSRPlugin::canHandle(const std::string& name)
{
    if (utils::toLower(utils::path_extension(name)) != "wsr") { return false; }
    // Confirm the WSRF footer so we don't grab unrelated files that merely use
    // the .wsr extension.
    std::vector<uint8_t> data;
    if (!readWhole(name, data)) { return false; }
    return hasWsrFooter(data);
}

std::set<std::string> WSRPlugin::getSupportedExtensions() const
{
    return {"wsr"};
}

ChipPlayer* WSRPlugin::fromFile(const std::string& fileName)
{
    return new WSRPlayer{fileName};
}

} // namespace musix

extern "C" void wsrplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::WSRPlugin>();
    });
}
