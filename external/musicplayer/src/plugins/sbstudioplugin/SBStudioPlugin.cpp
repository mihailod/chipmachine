#include "SBStudioPlugin.h"
#include "../../chipplayer.h"
#include <coreutils/utils.h>
#include <coreutils/log.h>

#include <cstdio>
#include <cstring>
#include <mutex>

extern "C" {
#include "pac.h"
}

namespace musix {

namespace {
// libpac keeps its output format (rate/bits/channels) in process-global state
// set once by pac_init(); a second call returns PAC_EALREADY. We always want
// 44100 Hz / 16-bit / stereo, so initialise it exactly once and never tear it
// down. Individual modules opened with pac_open() each carry their own state,
// so multiple players can coexist under the one shared config.
std::once_flag g_pacInit;
bool g_pacReady = false;

void ensurePacInit() {
    std::call_once(g_pacInit, [] {
        g_pacReady = (pac_init(44100, 16, 2) == 0);
    });
}
} // namespace

class SBStudioPlayer : public ChipPlayer {
public:
    explicit SBStudioPlayer(const std::string& fileName) {
        ensurePacInit();
        if (!g_pacReady) {
            throw player_exception("Could not initialise libpac");
        }

        module = pac_open(fileName.c_str());
        if (module == nullptr) {
            throw player_exception("Not a valid SBStudio module: " + fileName);
        }

        const char* title = pac_title(module);
        long frames = pac_length(module); // total frames in the song
        int length_seconds = frames > 0 ? static_cast<int>(frames / 44100) : 0;
        setMeta("title", title != nullptr ? title : "", "length",
                length_seconds, "format", "SBStudio");
    }

    ~SBStudioPlayer() override {
        if (module != nullptr) {
            pac_close(module);
            module = nullptr;
        }
    }

    int getHZ() override { return 44100; }

    int getSamples(int16_t* target, int noSamples) override {
        // pac_read counts bytes and returns bytes produced; the host counts
        // interleaved int16 values. 16-bit stereo -> 2 bytes per value. A short
        // read (0 bytes) means the song ended; report -1 so the host advances
        // instead of spinning (cf. UADE SONG_END lesson).
        long got = pac_read(module, target,
                            static_cast<long>(noSamples) * sizeof(int16_t));
        if (got <= 0) {
            return -1;
        }
        return static_cast<int>(got / sizeof(int16_t));
    }

    bool seekTo(int /*song*/, int seconds) override {
        if (seconds >= 0 && module != nullptr) {
            // pac_seek works in frames from the start (SEEK_SET).
            pac_seek(module, static_cast<long>(seconds) * 44100, SEEK_SET);
        }
        return true;
    }

private:
    struct pac_module* module = nullptr;
};

bool SBStudioPlugin::canHandle(const std::string& name) {
    if (utils::path_extension(name) != "pac") {
        return false;
    }
    // SBStudio modules begin with the IFF-like magic "PACG". The cheap header
    // check keeps us from grabbing unrelated payloads on the .pac extension.
    FILE* fp = fopen(name.c_str(), "rb");
    if (fp == nullptr) {
        return false;
    }
    char magic[4] = {0};
    size_t n = fread(magic, 1, sizeof(magic), fp);
    fclose(fp);
    return n == sizeof(magic) && std::memcmp(magic, "PACG", 4) == 0;
}

std::set<std::string> SBStudioPlugin::getSupportedExtensions() const {
    return {"pac"};
}

ChipPlayer* SBStudioPlugin::fromFile(const std::string& name) {
    return new SBStudioPlayer{name};
}

} // namespace musix

extern "C" void sbstudioplugin_register() {
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::SBStudioPlugin>();
    });
}
