#include "WSRPlugin.h"

#include "../../chipplayer.h"

#include <coreutils/utils.h>

// Vendored in_wsr replayer (GPL-2.0+). Only the public WSRPlayerSetUp() vtable
// entry point is used; every other symbol in the core is renamed to a wsr__*
// prefix at compile time (see CMakeLists.txt) so its generic global names
// (ROM, SampleRate, sample_buffer, ...) can't collide with the other plugins.
extern "C" {
#include "in_wsr/wsr_player.h"
}

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace musix {

namespace {
constexpr int WSR_HZ = 44100;
constexpr int WSR_NCH = 2;

// The WSRF footer is the last 32 bytes; the first playable subsong index lives
// at offset +5 within it.
constexpr long WSR_FOOTER_SIZE = 0x20;
constexpr long WSR_FIRSTSONG_OFF = 5;

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

// NOTE: the in_wsr core keeps all its state in globals, so only one WSRPlayer
// can be alive at a time. chipmachine plays a single tune at a time, which fits;
// constructing a second player while one is live would share/clobber state.
class WSRPlayer : public ChipPlayer {
public:
    explicit WSRPlayer(const std::string& fileName)
    {
        if (!readWhole(fileName, data_)) {
            throw player_exception("WSR: cannot read " + fileName);
        }
        if (!hasWsrFooter(data_)) {
            throw player_exception("WSR: missing WSRF footer");
        }

        api_ = WSRPlayerSetUp();
        if (api_ == nullptr) {
            throw player_exception("WSR: replayer init failed");
        }

        // Load_WSR copies the image into the core's own buffer and re-checks the
        // footer; it returns 0 on any problem.
        if (api_->p_Load_WSR(data_.data(),
                             static_cast<unsigned>(data_.size())) == 0) {
            api_ = nullptr;
            throw player_exception("WSR: not a valid WonderSwan sound rip");
        }

        api_->p_Set_Frequency(WSR_HZ);
        firstSong_ = api_->p_Get_FirstSong();
        api_->p_Reset_WSR(static_cast<unsigned>(firstSong_));

        uint32_t songs = std::max<uint32_t>(
            WSR_BROWSE_SONGS,
            static_cast<uint32_t>(firstSong_) + WSR_SONGS_MARGIN);
        setMeta("format", "WonderSwan", "songs", songs, "startSong",
                static_cast<uint32_t>(firstSong_), "length", WSR_DEFAULT_LENGTH);
    }

    ~WSRPlayer() override
    {
        if (api_ != nullptr) { api_->p_Close_WSR(); }
    }

    int getHZ() override { return WSR_HZ; }

    int getSamples(int16_t* target, int noSamples) override
    {
        if (api_ == nullptr) { return -1; }
        // The host counts interleaved int16 values; Update_WSR wants stereo
        // frames and the byte size of the destination buffer.
        unsigned frames = static_cast<unsigned>(noSamples / WSR_NCH);
        unsigned bytes = frames * WSR_NCH * sizeof(int16_t);
        if (api_->p_Update_WSR(target, bytes, frames) == 0) { return -1; }
        return static_cast<int>(frames) * WSR_NCH;
    }

    bool seekTo(int song, int /*seconds*/) override
    {
        if (api_ == nullptr || song < 0) { return false; }
        // The host's subsong index is the WonderSwan song number directly.
        api_->p_Reset_WSR(static_cast<unsigned>(song));
        return true;
    }

private:
    std::vector<uint8_t> data_;
    WSRPlayerApi* api_ = nullptr;
    int firstSong_ = 0;
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
