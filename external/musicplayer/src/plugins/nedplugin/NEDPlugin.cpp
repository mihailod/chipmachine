#include "NEDPlugin.h"
#include "../../chipplayer.h"

#include "ned/ned_engine.h"

#include <coreutils/log.h>
#include <coreutils/utils.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>

namespace musix {

namespace {

// .ned modules start with "NED: " (v2.0/2.1) or "NED\x10" (v1.0).
bool hasNedMagic(const std::string& path)
{
    FILE* fp = fopen(path.c_str(), "rb");
    if (fp == nullptr) {
        return false;
    }
    uint8_t hdr[5] = {0, 0, 0, 0, 0};
    size_t n = fread(hdr, 1, sizeof(hdr), fp);
    fclose(fp);
    if (n < 4) {
        return false;
    }
    bool v20 = (hdr[0] == 'N' && hdr[1] == 'E' && hdr[2] == 'D' && hdr[3] == ':');
    bool v10 = (hdr[0] == 'N' && hdr[1] == 'E' && hdr[2] == 'D' && hdr[3] == 0x10);
    return v20 || v10;
}

// The replay engine has global state; serialize player instances.
std::mutex& engineMutex()
{
    static std::mutex m;
    return m;
}

} // namespace

class NEDPlayer : public ChipPlayer
{
public:
    explicit NEDPlayer(const std::string& fileName) : lock(engineMutex())
    {
        if (!ned_engine_load(fileName.c_str())) {
            throw player_exception("Could not load NED module: " + fileName);
        }
        if (!ned_engine_start()) {
            throw player_exception("Could not start NED playback: " + fileName);
        }
        setMeta("title", utils::path_basename(fileName), "songs", 1, "startSong",
                0, "format", "NerdTracker 2", "channels", 5);
    }

    ~NEDPlayer() override { ned_engine_shutdown(); }

    int getHZ() override { return 44100; }

    int getSamples(int16_t* target, int noSamples) override
    {
        // The engine renders mono; the host wants interleaved stereo. Render
        // noSamples/2 mono samples into the front of the buffer, then fan out.
        int frames = noSamples / 2;
        ned_engine_render(target, frames);
        for (int j = frames - 1; j >= 0; j--) {
            int16_t s = target[j];
            target[j * 2] = s;
            target[j * 2 + 1] = s;
        }
        return noSamples;
    }

private:
    std::lock_guard<std::mutex> lock;
};

static const std::set<std::string> supported_ext = {"ned"};

bool NEDPlugin::canHandle(const std::string& name)
{
    if (supported_ext.count(utils::path_extension(name)) == 0) {
        return false;
    }
    return hasNedMagic(name);
}

std::set<std::string> NEDPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* NEDPlugin::fromFile(const std::string& fileName)
{
    return new NEDPlayer{fileName};
}

} // namespace musix

extern "C" __attribute__((visibility("default"))) void nedplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::NEDPlugin>();
    });
}
