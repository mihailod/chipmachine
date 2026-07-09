#include "OrgPlugin.h"
#include "../../chipplayer.h"
#include <coreutils/utils.h>
#include <coreutils/log.h>

#include "organya.h"
#include "default_wdb.h"

#include <cstdio>
#include <cstring>

namespace musix {

class OrgPlayer : public ChipPlayer {
public:
    explicit OrgPlayer(const std::string& fileName) {
        if (organya_context_init(&ctx) != ORG_RESULT_SUCCESS) {
            throw player_exception("Could not init organya context");
        }
        inited = true;

        organya_context_set_sample_rate(&ctx, 44100);
        organya_context_set_volume(&ctx, 1.0f);
        organya_context_set_interpolation(&ctx, ORG_INTERPOLATION_CUBIC);
        organya_context_set_output_format(&ctx, ORG_OUTPUT_FORMAT_S16);

        // The wavetable + drum samples are a universal constant shared by every
        // .org file; it is embedded here so nothing has to be fetched at runtime.
        if (organya_context_read_soundbank(&ctx, default_wdb, default_wdb_len) !=
            ORG_RESULT_SUCCESS) {
            cleanup();
            throw player_exception("Could not load organya soundbank");
        }

        if (organya_context_load_song_file(&ctx, fileName.c_str()) !=
            ORG_RESULT_SUCCESS) {
            cleanup();
            throw player_exception("Could not load organya song: " + fileName);
        }

        // .org has no embedded title; length is the time to the loop point.
        tempo_ms = ctx.song.tempo_ms;
        int length_seconds = 0;
        if (tempo_ms > 0 && ctx.song.repeat_end > 0) {
            length_seconds =
                (int)((int64_t)ctx.song.repeat_end * tempo_ms / 1000);
        }

        setMeta("length", length_seconds, "format", "Organya");
    }

    ~OrgPlayer() override { cleanup(); }

    int getHZ() override { return 44100; }

    int getSamples(int16_t* target, int noSamples) override {
        // The host counts in interleaved int16 values; the library counts in
        // stereo frames (2 int16 each).
        size_t frames = (size_t)noSamples / 2;
        size_t got = organya_context_generate_samples(&ctx, target, frames);
        return (int)(got * 2);
    }

    bool seekTo(int /*song*/, int seconds) override {
        if (seconds >= 0 && tempo_ms > 0) {
            org_int32 tick = (org_int32)((int64_t)seconds * 1000 / tempo_ms);
            organya_context_seek(&ctx, tick);
        }
        return true;
    }

private:
    void cleanup() {
        if (inited) {
            organya_context_deinit(&ctx);
            inited = false;
        }
    }

    organya_context ctx{};
    bool inited = false;
    int tempo_ms = 0;
};

bool OrgPlugin::canHandle(const std::string& name) {
    if (utils::path_extension(name) != "org") {
        return false;
    }
    // Validate the "Org-0[1-3]" signature so we don't grab unrelated .org files.
    FILE* fp = fopen(name.c_str(), "rb");
    if (!fp) {
        return false;
    }
    unsigned char magic[6];
    size_t n = fread(magic, 1, sizeof(magic), fp);
    fclose(fp);
    if (n < sizeof(magic)) {
        return false;
    }
    return memcmp(magic, "Org-0", 5) == 0 && magic[5] >= '1' && magic[5] <= '3';
}

std::set<std::string> OrgPlugin::getSupportedExtensions() const {
    return {"org"};
}

ChipPlayer* OrgPlugin::fromFile(const std::string& name) {
    return new OrgPlayer{name};
}

} // namespace musix

extern "C" void orgplugin_register() {
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::OrgPlugin>();
    });
}
