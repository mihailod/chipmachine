#include "MgtPlugin.h"
#include "../../chipplayer.h"
#include <coreutils/utils.h>
#include <coreutils/log.h>

#include <cstdio>
#include <cstring>
#include <vector>

// libxmp public API. BUILDING_STATIC makes EXPORT expand to nothing so we link
// against the static slice compiled into musxplugin (this plugin only adds the
// Megatracker loader on top of that shared slice).
#define BUILDING_STATIC
extern "C" {
#include "xmp.h"

// Internal libxmp entry points. We drive the single mgt_loader directly via the
// "typed" loader instead of pulling in the full auto-detect table
// (NO_COMPOSITE_LOADER build). mgt_loader lives in mgt_load.c (compiled into
// this plugin); everything else comes from the musxplugin libxmp slice.
struct format_loader;
extern const struct format_loader mgt_loader;
int xmp_load_typed_module_from_memory(xmp_context, void*, long,
                                      const struct format_loader*);
}

#include "../../tracker_xmp.h"

namespace musix {

class MgtPlayer : public ChipPlayer {
public:
    explicit MgtPlayer(const std::string& fileName) {
        ctx = xmp_create_context();
        if (ctx == nullptr) {
            throw player_exception("Could not create xmp context");
        }

        // Read the whole module into memory; libxmp loads from a buffer.
        FILE* fp = fopen(fileName.c_str(), "rb");
        if (fp == nullptr) {
            xmp_free_context(ctx);
            throw player_exception("Could not open mgt file: " + fileName);
        }
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        std::vector<uint8_t> data(size > 0 ? static_cast<size_t>(size) : 0);
        size_t got = size > 0 ? fread(data.data(), 1, data.size(), fp) : 0;
        fclose(fp);
        if (got != data.size() || data.empty()) {
            xmp_free_context(ctx);
            throw player_exception("Could not read mgt file: " + fileName);
        }

        int rc = xmp_load_typed_module_from_memory(
            ctx, data.data(), static_cast<long>(data.size()), &mgt_loader);
        if (rc != 0) {
            xmp_free_context(ctx);
            throw player_exception("Not a valid Megatracker module: " + fileName);
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
                "Megatracker");
    }

    ~MgtPlayer() override { cleanup(); }

    int getHZ() override { return 44100; }

    int getSamples(int16_t* target, int noSamples) override {
        // libxmp counts bytes; the host counts interleaved int16 values.
        // 16-bit signed stereo is the default mix format. loop=1 -> after one
        // full pass libxmp returns non-zero (XMP_END); we report -1 so the host
        // advances instead of spinning (cf. UADE SONG_END lesson).
        //
        // Played in short slices only so the tracker view can see every row go
        // by: the host asks for tens of thousands of samples at a time, and
        // reading the play position once per call would show one row in ten.
        const int slice = 512 * 2; // interleaved values == 512 frames
        int done = 0;
        while (done < noSamples) {
            TrackerRow tr;
            if (rowWatcher.capture(ctx, done / 2, tr)) { pushTrackerRow(tr); }
            int n = noSamples - done;
            if (n > slice) { n = slice; }
            int rc = xmp_play_buffer(ctx, target + done, n * 2, 1);
            if (rc != 0) {
                return done > 0 ? done : -1;
            }
            done += n;
        }
        return done;
    }

    bool hasTrackerRows() const override { return true; }

    bool seekTo(int /*song*/, int seconds) override {
        if (seconds >= 0) {
            xmp_seek_time(ctx, seconds * 1000);
        }
        rowWatcher.reset();
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
    tracker::XmpRowWatcher rowWatcher;
    bool loaded = false;
    bool started = false;
};

bool MgtPlugin::canHandle(const std::string& name) {
    if (utils::path_extension(name) != "mgt") {
        return false;
    }
    // Megatracker files start with the magic "\x00MGT" (read as 'M','G','T' over
    // the low 24 bits), a version byte, then "\xbdMCS". Mirror mgt_test's gate so
    // we only claim genuine Megatracker modules.
    FILE* fp = fopen(name.c_str(), "rb");
    if (fp == nullptr) {
        return false;
    }
    unsigned char hdr[8] = {0};
    size_t n = fread(hdr, 1, sizeof(hdr), fp);
    fclose(fp);
    return n == sizeof(hdr) && hdr[0] == 'M' && hdr[1] == 'G' &&
           hdr[2] == 'T' && hdr[4] == 0xbd && hdr[5] == 'M' &&
           hdr[6] == 'C' && hdr[7] == 'S';
}

std::set<std::string> MgtPlugin::getSupportedExtensions() const {
    return {"mgt"};
}

ChipPlayer* MgtPlugin::fromFile(const std::string& name) {
    return new MgtPlayer{name};
}

} // namespace musix

extern "C" void mgtplugin_register() {
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::MgtPlugin>();
    });
}
