#include "PlayerProPlugin.h"
#include "PlayerProRender.h"
#include "../../chipplayer.h"

#include <coreutils/file.h>
#include <coreutils/utils.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

// PlayerPRO plugin. The heavy lifting is in the vendored MADDriver (repo-root
// playerpro/) behind the small C bridge in PlayerProRender.{c,h}; this file is
// just the chipmachine glue: magic detection, lifetime, and pull-model output.

namespace musix {

namespace {

// The PlayerPRO module signatures we accept. "MADG"/"MADF" are the older format
// (the entire Modland PlayerPro/ tree -- Benjamin Birney's "mantra" score) and
// "MADK" is the native format. Other MADx variants (MADH/MADI) have no loader
// compiled in, so we don't claim them.
bool isPlayerProMagic(const uint8_t* m)
{
    return memcmp(m, "MADG", 4) == 0 || memcmp(m, "MADF", 4) == 0 ||
           memcmp(m, "MADK", 4) == 0;
}

class PlayerProPlayer : public ChipPlayer
{
public:
    explicit PlayerProPlayer(const std::string& fileName)
    {
        render_ = pprender_open(fileName.c_str());
        if (render_ == nullptr) {
            throw player_exception("PlayerPRO: could not load module");
        }
        std::string title = utils::path_basename(fileName);
        setMeta("title", title, "songs", 1, "startSong", 0, "format",
                "PlayerPRO", "channels", 0);
    }

    ~PlayerProPlayer() override
    {
        if (render_ != nullptr) {
            pprender_close(render_);
        }
    }

    int getHZ() override { return pprender_hz(render_); }

    // chipmachine asks for `size` int16 values (stereo interleaved, so size/2
    // frames). The bridge serves any frame count and returns 0 at end-of-song,
    // which we report as -1 so the host stops rather than retrying.
    int getSamples(int16_t* target, int size) override
    {
        int frames = pprender_fill(render_, target, size / 2);
        if (frames <= 0) {
            return -1;
        }
        return frames * 2;
    }

    bool seekTo(int song, int /*seconds*/) override { return song == 0; }

private:
    PPRender* render_ = nullptr;
};

} // namespace

bool PlayerProPlugin::canHandle(const std::string& name)
{
    // Content-only: the ".mad" extension is shared with AdPlug's unrelated Mad
    // Tracker 2 loader, so route purely on the PlayerPRO magic.
    FILE* fp = fopen(name.c_str(), "rb");
    if (fp == nullptr) {
        return false;
    }
    std::array<uint8_t, 4> magic{};
    size_t n = fread(magic.data(), 1, magic.size(), fp);
    fclose(fp);
    return n == magic.size() && isPlayerProMagic(magic.data());
}

std::set<std::string> PlayerProPlugin::getSupportedExtensions() const
{
    return {"mad"};
}

ChipPlayer* PlayerProPlugin::fromFile(const std::string& fileName)
{
    return new PlayerProPlayer{fileName};
}

} // namespace musix

extern "C" void playerproplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::PlayerProPlugin>();
    });
}
