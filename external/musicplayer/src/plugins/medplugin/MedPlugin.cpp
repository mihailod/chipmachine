#include "MedPlugin.h"
#include "../../chipplayer.h"
#include <coreutils/utils.h>
#include <coreutils/log.h>

#include <cstdio>
#include <cstring>
#include <vector>

// libxmp public API. BUILDING_STATIC makes EXPORT expand to nothing so we link
// against the static slice compiled into musxplugin (this plugin only adds the
// old-MED loaders on top of that shared slice).
#define BUILDING_STATIC
extern "C" {
#include "xmp.h"

// Internal libxmp entry points. We drive a single loader directly via the
// "typed" loader instead of pulling in the full auto-detect table
// (NO_COMPOSITE_LOADER build). The med2/med3/med4 loaders live in the
// respective *_load.c files (compiled into this plugin); everything else comes
// from the musxplugin libxmp slice.
struct format_loader;
extern const struct format_loader med2_loader;
extern const struct format_loader med3_loader;
extern const struct format_loader med4_loader;
int xmp_load_typed_module_from_memory(xmp_context, void*, long,
                                      const struct format_loader*);
}

#include "../../tracker_xmp.h"

namespace musix {

// Pick the loader for an old-MED magic byte ("MED\x02".."MED\x04"). Returns
// nullptr for anything else (e.g. the MMD0..MMD3 OctaMED containers, which this
// plugin deliberately does not claim).
static const struct format_loader* medLoaderFor(unsigned char version) {
    switch (version) {
    case 2: return &med2_loader;
    case 3: return &med3_loader;
    case 4: return &med4_loader;
    default: return nullptr;
    }
}

class MedPlayer : public ChipPlayer {
public:
    explicit MedPlayer(const std::string& fileName) {
        ctx = xmp_create_context();
        if (ctx == nullptr) {
            throw player_exception("Could not create xmp context");
        }

        // Read the whole module into memory; libxmp loads from a buffer.
        FILE* fp = fopen(fileName.c_str(), "rb");
        if (fp == nullptr) {
            xmp_free_context(ctx);
            throw player_exception("Could not open med file: " + fileName);
        }
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        std::vector<uint8_t> data(size > 0 ? static_cast<size_t>(size) : 0);
        size_t got = size > 0 ? fread(data.data(), 1, data.size(), fp) : 0;
        fclose(fp);
        if (got != data.size() || data.size() < 4) {
            xmp_free_context(ctx);
            throw player_exception("Could not read med file: " + fileName);
        }

        // Select the loader by the "MED\xNN" version byte. canHandle already
        // gated this, but stay defensive (fromFile can be reached directly).
        const struct format_loader* loader =
            (data[0] == 'M' && data[1] == 'E' && data[2] == 'D')
                ? medLoaderFor(data[3])
                : nullptr;
        if (loader == nullptr) {
            xmp_free_context(ctx);
            throw player_exception("Not an old-MED module: " + fileName);
        }

        int rc = xmp_load_typed_module_from_memory(
            ctx, data.data(), static_cast<long>(data.size()), loader);
        if (rc != 0) {
            xmp_free_context(ctx);
            throw player_exception("Not a valid MED module: " + fileName);
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
        // libxmp fills mod->type with e.g. "MED 3.20 MED4" -- a useful version
        // string. Fall back to a plain label if it is empty.
        std::string format =
            (modInfo.mod && modInfo.mod->type[0]) ? modInfo.mod->type : "MED";
        int length_seconds = frmInfo.total_time / 1000;
        setMeta("title", title, "length", length_seconds, "format", format);
    }

    ~MedPlayer() override { cleanup(); }

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

bool MedPlugin::canHandle(const std::string& name) {
    // Old-MED modules start with the magic 'M','E','D' followed by a version
    // byte 0x02..0x04 (MED 2.00 / 2.10 / 3.xx). Gate purely on this 4-byte
    // signature: it is unambiguous and distinct from the MMD0..MMD3 OctaMED
    // containers ('M','M','D',...), which OpenMPT/UADE handle. Modland stores
    // these under "Music Editor" with a .med extension, but we do not require
    // the extension -- the magic alone is a safe, exclusive claim.
    FILE* fp = fopen(name.c_str(), "rb");
    if (fp == nullptr) {
        return false;
    }
    unsigned char hdr[4] = {0};
    size_t n = fread(hdr, 1, sizeof(hdr), fp);
    fclose(fp);
    return n == sizeof(hdr) && hdr[0] == 'M' && hdr[1] == 'E' &&
           hdr[2] == 'D' && hdr[3] >= 2 && hdr[3] <= 4;
}

std::set<std::string> MedPlugin::getSupportedExtensions() const {
    return {"med"};
}

ChipPlayer* MedPlugin::fromFile(const std::string& name) {
    return new MedPlayer{name};
}

} // namespace musix

extern "C" void medplugin_register() {
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::MedPlugin>();
    });
}
