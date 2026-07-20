#include "SunVoxPlugin.h"
#include "../../chipplayer.h"

#include <coreutils/utils.h>
#include <coreutils/log.h>
#include <coreutils/path.h>
#include <coreutils/environment.h>

// SUNVOX_MAIN turns sunvox.h into the dynamic loader: it defines the global
// sv_* function pointers plus sv_load_dll()/sv_unload_dll(). The actual engine
// ships as a prebuilt shared library (MIT licensed) that we dlopen at runtime,
// so there is no link-time dependency on it. Define it in exactly one TU.
#define SUNVOX_MAIN
#ifndef _WIN32
#include <dlfcn.h> // sunvox.h's loader uses dlopen()/dlsym() but doesn't include this
#endif
#include "sunvox.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>

namespace musix {

namespace {

// The SunVox engine is a single process-global instance addressed by up to 16
// "slots". sv_load_dll()/sv_init()/sv_deinit() are all global, so we ref-count
// them and hand each player its own slot. chipmachine plays one song at a time,
// so 16 slots is comfortably more than we need.
constexpr int kNumSlots = 16;

std::mutex g_mutex;
int g_refcount = 0;
std::array<bool, kNumSlots> g_slotUsed{};

const char* libName()
{
#if defined(_WIN32)
    return "sunvox.dll";
#elif defined(__APPLE__)
    return "sunvox.dylib";
#else
    return "sunvox.so";
#endif
}

// Locate and dlopen the engine, then init it. Must be called under g_mutex.
// Returns true with g_refcount incremented on success.
bool acquireEngine()
{
    if (g_refcount > 0) {
        g_refcount++;
        return true;
    }

    // The shared library is shipped next to the executable (build dir or
    // Contents/MacOS) and, for the macOS bundle, also under Contents/Resources.
    bool loaded = false;
    std::vector<utils::path> dirs = {Environment::getExeDir(),
                                     Environment::getAppDir()};
    for (auto const& d : dirs) {
        auto p = d / libName();
        if (utils::exists(p) && sv_load_dll2(p.string().c_str()) == 0) {
            loaded = true;
            break;
        }
    }
    // Disabled for Apple macOS hardened runtime/notarization:
    // (no loading arbitrary sunvox.dylib from search paths!)
    // Last resort: default loader search path (cwd / DYLD / LD paths).
    //if (!loaded && sv_load_dll() == 0) {
    //    loaded = true;
    //}
    if (!loaded) {
        LOGW("Could not load bundled SunVox library!");
        return false;
    }

    // NO_DEBUG_OUTPUT silences the library's stdout chatter (e.g. the
    // "SOUND: sundog_sound_deinit()" lines printed on teardown).
    if (sv_init(nullptr, 44100, 2,
                SV_INIT_FLAG_NO_DEBUG_OUTPUT |
                    SV_INIT_FLAG_USER_AUDIO_CALLBACK | SV_INIT_FLAG_ONE_THREAD |
                    SV_INIT_FLAG_AUDIO_INT16) < 0) {
        LOGW("sv_init() failed");
        sv_unload_dll();
        return false;
    }

    g_refcount = 1;
    return true;
}

// Must be called under g_mutex.
void releaseEngine()
{
    if (g_refcount > 0 && --g_refcount == 0) {
        sv_deinit();
        sv_unload_dll();
    }
}

// Must be called under g_mutex. Returns -1 if no slot is free.
int allocSlot()
{
    for (int i = 0; i < kNumSlots; i++) {
        if (!g_slotUsed[i]) {
            g_slotUsed[i] = true;
            return i;
        }
    }
    return -1;
}

void freeSlot(int slot)
{
    if (slot >= 0 && slot < kNumSlots) { g_slotUsed[slot] = false; }
}

} // namespace

class SunVoxPlayer : public ChipPlayer {
public:
    explicit SunVoxPlayer(const std::string& fileName)
    {
        std::lock_guard<std::mutex> lock(g_mutex);

        if (!acquireEngine()) {
            throw player_exception("Could not initialize SunVox engine");
        }
        engineOwned = true;

        slot = allocSlot();
        if (slot < 0) {
            releaseEngine();
            engineOwned = false;
            throw player_exception("No free SunVox slot");
        }

        if (sv_open_slot(slot) < 0) {
            freeSlot(slot);
            slot = -1;
            releaseEngine();
            engineOwned = false;
            throw player_exception("Could not open SunVox slot");
        }

        if (sv_load(slot, fileName.c_str()) < 0) {
            sv_close_slot(slot);
            freeSlot(slot);
            slot = -1;
            releaseEngine();
            engineOwned = false;
            throw player_exception("Could not load SunVox song: " + fileName);
        }

        sv_volume(slot, 256);
        // Stop emitting sound once the song ends so the host's silence
        // detection terminates playback instead of looping forever.
        sv_set_autostop(slot, 1);

        rate = sv_get_sample_rate();
        if (rate <= 0) { rate = 44100; }
        lengthFrames = sv_get_song_length_frames(slot);
        lengthLines = (int)sv_get_song_length_lines(slot);

        sv_play_from_beginning(slot);

        int lengthSeconds = rate > 0 ? (int)(lengthFrames / (uint32_t)rate) : 0;
        const char* svName = sv_get_song_name(slot);
        std::string title = svName ? svName : "";
        setMeta("title", title, "length", lengthSeconds, "format", "SunVox");
    }

    ~SunVoxPlayer() override
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (slot >= 0) {
            sv_stop(slot);
            sv_close_slot(slot);
            freeSlot(slot);
        }
        if (engineOwned) { releaseEngine(); }
    }

    int getHZ() override { return rate; }

    int getSamples(int16_t* target, int noSamples) override
    {
        // The host counts interleaved int16 values; SunVox counts stereo frames.
        int frames = noSamples / 2;
        sv_audio_callback(target, frames, 0, sv_get_ticks());
        return frames * 2;
    }

    bool seekTo(int /*song*/, int seconds) override
    {
        if (seconds >= 0 && slot >= 0 && lengthFrames > 0 && lengthLines > 0) {
            // SunVox seeks by pattern line, not by frame, so this is an
            // approximate proportional mapping from time to line.
            int64_t targetFrame = (int64_t)seconds * rate;
            int line = (int)(targetFrame * lengthLines / lengthFrames);
            if (line < 0) { line = 0; }
            if (line >= lengthLines) { line = lengthLines - 1; }
            std::lock_guard<std::mutex> lock(g_mutex);
            sv_lock_slot(slot);
            sv_rewind(slot, line);
            sv_unlock_slot(slot);
        }
        return true;
    }

private:
    int slot = -1;
    int rate = 44100;
    uint32_t lengthFrames = 0;
    int lengthLines = 0;
    bool engineOwned = false;
};

bool SunVoxPlugin::canHandle(const std::string& name)
{
    if (utils::path_extension(name) != "sunvox") { return false; }

    FILE* fp = fopen(name.c_str(), "rb");
    if (!fp) { return false; }
    unsigned char magic[4];
    size_t n = fread(magic, 1, sizeof(magic), fp);
    fclose(fp);
    if (n < sizeof(magic)) { return false; }

    // Uncompressed projects start with "SVOX"; SunVox also accepts zlib-deflated
    // projects, which begin with the zlib header byte 0x78.
    if (memcmp(magic, "SVOX", 4) == 0) { return true; }
    return magic[0] == 0x78;
}

std::set<std::string> SunVoxPlugin::getSupportedExtensions() const
{
    return {"sunvox"};
}

ChipPlayer* SunVoxPlugin::fromFile(const std::string& name)
{
    return new SunVoxPlayer{name};
}

} // namespace musix

extern "C" void sunvoxplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::SunVoxPlugin>();
    });
}
