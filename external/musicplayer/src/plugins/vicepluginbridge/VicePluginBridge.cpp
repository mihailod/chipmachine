#include "VicePlugin.h"

#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <crypto/md5.h>

#include <filesystem>
#include <string>

extern "C" {
#include "vice.h"
#include "init.h"
#include "machine.h"
#include "psid.h"
#include "maincpu.h"
#include "sound.h"
#include "lib.h"
#include "drive.h"
#include "sysfile.h"
#include "gfxoutput.h"
#include "resources.h"

extern machine_timing_t machine_timing;
void psid_play(int16_t* buffer, int length);
}

extern "C" int psid_ui_set_tune(int tune, void *param);

// Variables normally defined in vice/main.c
int console_mode = 1;
int video_disabled_mode = 1;
int vsid_mode = 1;

namespace musix {

class VicePlayerBridge : public ChipPlayer {
public:
    VicePlayerBridge(const std::string& fileName) {
        if (psid_load_file(fileName.c_str()) != 0) {
            throw player_exception("Not a sid file");
        }

        int defaultSong = 0;
        int songs = psid_tunes(&defaultSong);
        psid_ui_set_tune(defaultSong, nullptr);

        setMeta("songs", static_cast<uint32_t>(songs));
        setMeta("startSong", static_cast<uint32_t>(defaultSong - 1));
    }

    ~VicePlayerBridge() override {
        psid_set_tune(-1);
    }

    int getSamples(int16_t* target, int size) override {
        psid_play(target, size);
        return size;
    }

    bool seekTo(int song, int seconds) override {
        if (song >= 0) {
            psid_ui_set_tune(song + 1, nullptr);
            return true;
        }
        return false;
    }
};

// VICE uses process-global/singleton engine state that is not designed to be
// initialized more than once while a prior init is live. Re-running the heavy
// init block (init_resources/resources_set_defaults/init_main/...) on a second
// VicePlugin construction operates on already-registered globals and fails.
// So we run the global engine init exactly once per process and keep it alive;
// only the cheap per-tune resource settings are (re)applied each construction.
// We deliberately never call machine_shutdown()/sound_close() from the
// destructor: another VicePlugin (or a later-constructed one) may still rely on
// the shared global engine, and VICE cannot survive a shutdown->re-init cycle.
VicePlugin::VicePlugin(const std::string& dataDir) : dataDir(dataDir) {
    static bool engineInitialized = false;
    if (!engineInitialized) {
        maincpu_early_init();
        machine_setup_context();
        drive_setup_context();
        machine_early_init();
        sysfile_init("C64");
        gfxoutput_early_init();
        if (init_resources() < 0) {
            throw player_exception("Failed to init resources");
        }
        if (resources_set_defaults() < 0) {
            throw player_exception("Cannot set defaults.");
        }
        engineInitialized = true;
    }

    resources_set_int("SidResidSampling", 0);
    resources_set_int("VICIIVideoCache", 0);
    resources_set_string("Directory", (dataDir + "/c64").c_str());

    static bool mainInitialized = false;
    if (!mainInitialized) {
        if (init_main() < 0) {
            throw player_exception("Could not init vice");
        }
        sound_init(machine_timing.cycles_per_sec,
                   machine_timing.cycles_per_rfsh);
        mainInitialized = true;
    }
}

VicePlugin::~VicePlugin() {
    // Intentionally no machine_shutdown()/sound_close(): the VICE engine is a
    // process-wide singleton initialized once and kept alive for the lifetime
    // of the process (see the constructor comment above).
}

namespace {

// Compute's Stereo Sidplayer tunes come as a pair: a ".mus" file (first SID,
// voices 1-3) and a ".str" file (second SID, voices 4-6). VICE expects to be
// handed the ".mus" file and loads the ".str" sibling itself. If we are asked
// to play the ".str" entry, redirect to its ".mus" companion.
std::string musForStr(const std::string& fileName)
{
    if (utils::endsWith(utils::toLower(fileName), ".str")) {
        return fileName.substr(0, fileName.size() - 4) + ".mus";
    }
    return fileName;
}

} // namespace

bool VicePlugin::canHandle(const std::string& name) {
    auto lname = utils::toLower(name);
    return utils::endsWith(lname, ".sid") || utils::endsWith(lname, ".rsid") ||
           utils::endsWith(lname, ".mus") || utils::endsWith(lname, ".str");
}

std::set<std::string> VicePlugin::getSupportedExtensions() const
{
    // .rsid is the "real C64" SID variant; psid_load_file accepts its "RSID"
    // magic just like a "PSID", so VICE plays it the same way.
    return {"sid", "rsid", "mus", "str"};
}

ChipPlayer* VicePlugin::fromFile(const std::string& fileName) {
    return new VicePlayerBridge(musForStr(fileName));
}

std::vector<std::string>
VicePlugin::getSecondaryFiles(const std::string& file)
{
    // The ".str" (stereo) file is useless on its own; it needs its ".mus"
    // companion, which is what VICE actually loads.
    if (utils::endsWith(utils::toLower(file), ".str")) {
        auto mus = std::filesystem::path(file).filename().string();
        mus = mus.substr(0, mus.size() - 4) + ".mus";
        return {mus};
    }
    return {};
}

} // namespace musix
