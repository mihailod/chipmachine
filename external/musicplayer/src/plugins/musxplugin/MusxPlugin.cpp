#include "MusxPlugin.h"
#include "../../chipplayer.h"
#include <coreutils/utils.h>
#include <coreutils/log.h>

#include <cstdio>
#include <cstring>
#include <vector>

// libxmp public API. BUILDING_STATIC makes EXPORT expand to nothing so we link
// against the static slice compiled in this plugin's CMakeLists.
#define BUILDING_STATIC
extern "C" {
#include "xmp.h"

// Internal libxmp entry points (declared in xmp_private.h). We only need the
// "typed" loader so we can drive the single arch_loader directly instead of
// pulling in the full 58-format auto-detect table (NO_COMPOSITE_LOADER build).
struct format_loader;
extern const struct format_loader arch_loader;
int xmp_load_typed_module_from_memory(xmp_context, void*, long,
                                      const struct format_loader*);
}

namespace musix {

class MusxPlayer : public ChipPlayer {
public:
    explicit MusxPlayer(const std::string& fileName) {
        ctx = xmp_create_context();
        if (ctx == nullptr) {
            throw player_exception("Could not create xmp context");
        }

        // Read the whole module into memory; libxmp loads from a buffer.
        FILE* fp = fopen(fileName.c_str(), "rb");
        if (fp == nullptr) {
            xmp_free_context(ctx);
            throw player_exception("Could not open musx file: " + fileName);
        }
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        std::vector<uint8_t> data(size > 0 ? static_cast<size_t>(size) : 0);
        size_t got = size > 0 ? fread(data.data(), 1, data.size(), fp) : 0;
        fclose(fp);
        if (got != data.size() || data.empty()) {
            xmp_free_context(ctx);
            throw player_exception("Could not read musx file: " + fileName);
        }

        int rc = xmp_load_typed_module_from_memory(
            ctx, data.data(), static_cast<long>(data.size()), &arch_loader);
        if (rc != 0) {
            xmp_free_context(ctx);
            throw player_exception("Not a valid Archimedes Tracker module: " +
                                   fileName);
        }
        loaded = true;

        if (xmp_start_player(ctx, getHZ(), 0) != 0) {
            cleanup();
            throw player_exception("Could not start xmp player: " + fileName);
        }
        started = true;

        xmp_module_info modInfo;
        xmp_get_module_info(ctx, &modInfo);
        xmp_frame_info frmInfo;
        xmp_get_frame_info(ctx, &frmInfo);

        std::string title = modInfo.mod ? modInfo.mod->name : "";
        int length_seconds = frmInfo.total_time / 1000;
        setMeta("title", title, "length", length_seconds, "format",
                "Archimedes Tracker");
    }

    ~MusxPlayer() override { cleanup(); }

    int getHZ() override { return 44100; }

    int getSamples(int16_t* target, int noSamples) override {
        // libxmp counts bytes; the host counts interleaved int16 values.
        // 16-bit signed stereo is the default mix format. loop=1 -> after one
        // full pass libxmp returns non-zero (XMP_END); we report -1 so the host
        // advances instead of spinning (cf. UADE SONG_END lesson).
        int rc = xmp_play_buffer(ctx, target, noSamples * 2, 1);
        if (rc != 0) {
            return -1;
        }
        return noSamples;
    }

    bool seekTo(int /*song*/, int seconds) override {
        if (seconds >= 0) {
            xmp_seek_time(ctx, seconds * 1000);
        }
        return true;
    }

private:
    void cleanup() {
        if (ctx != nullptr) {
            if (started) {
                xmp_end_player(ctx);
                started = false;
            }
            if (loaded) {
                xmp_release_module(ctx);
                loaded = false;
            }
            xmp_free_context(ctx);
            ctx = nullptr;
        }
    }

    xmp_context ctx = nullptr;
    bool loaded = false;
    bool started = false;
};

bool MusxPlugin::canHandle(const std::string& name) {
    if (utils::path_extension(name) != "musx") {
        return false;
    }
    // The Acorn "!Tracker"/Archimedes Tracker format begins with "MUSX".
    // The .musx extension is also used by Finale notation files etc., so the
    // magic check keeps us from grabbing unrelated payloads.
    FILE* fp = fopen(name.c_str(), "rb");
    if (fp == nullptr) {
        return false;
    }
    char magic[4];
    size_t n = fread(magic, 1, sizeof(magic), fp);
    fclose(fp);
    return n == sizeof(magic) && memcmp(magic, "MUSX", 4) == 0;
}

std::set<std::string> MusxPlugin::getSupportedExtensions() const {
    return {"musx"};
}

ChipPlayer* MusxPlugin::fromFile(const std::string& name) {
    return new MusxPlayer{name};
}

} // namespace musix

extern "C" void musxplugin_register() {
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::MusxPlugin>();
    });
}
