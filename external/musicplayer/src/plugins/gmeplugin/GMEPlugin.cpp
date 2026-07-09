#include "GMEPlugin.h"
#include "../../chipplayer.h"
#include "../../vgm_opl_detect.h"

#include <coreutils/log.h>
#include <coreutils/ptr.h>
#include <coreutils/utils.h>

#include "gme/gme.h"

#include <cstring>
#include <fstream>
#include <set>

namespace musix {

class GMEPlayer : public ChipPlayer
{
public:
    explicit GMEPlayer(const std::string& fileName)
        : started(false), ended(false)
    {
        gme_err_t err =
            gme_open_file(fileName.c_str(), &utils::raw_ptr(emu), 44100);
        if (err != nullptr) {
            throw player_exception(std::string("Could not load GME music: ") + err);
        }

        gme_info_t* track0 = nullptr;

        gme_track_info(emu.get(), &track0, 0);

        setMeta("game", track0->game, "composer", track0->author, "copyright",
                track0->copyright, "length",
                track0->length > 0 ? track0->length / 1000 : 0, "sub_title",
                track0->song, "format", track0->system, "songs",
                gme_track_count(emu.get()));
        gme_free_info(track0);
    }

    int getSamples(int16_t* target, int noSamples) override
    {
        gme_err_t err = nullptr;
        if (!started) {
            err = gme_start_track(emu.get(), 0);
            started = true;
        }

        if (!ended && (gme_track_ended(emu.get()) != 0)) {
            LOGD("## GME HAS ENDED");
            ended = true;
        }
        if (ended) {
            memset(target, 0, noSamples * 2);
            return noSamples;
        }

        err = gme_play(emu.get(), noSamples, target);

        return noSamples;
    }

    bool seekTo(int song, int seconds) override
    {
        if (song >= 0) {

            if (ended) {
                // err = gme_start_track(emu, 0);
                ended = false;
            }

            gme_info_t* track = nullptr;
            if (gme_track_info(emu.get(), &track, song) != nullptr) {
                return false;
            }
            setMeta("sub_title", track->song, "length",
                    track->length > 0 ? track->length / 1000 : 0, "song",
                    static_cast<uint32_t>(song));

            gme_start_track(emu.get(), song);
            started = true;
            gme_free_info(track);
        }
        if (seconds >= 0) { gme_seek(emu.get(), seconds); }
        return true;
    }

private:
    std::unique_ptr<Music_Emu, void (*)(Music_Emu*)> emu{nullptr, gme_delete};
    bool started;
    bool ended;
};

static const std::set<std::string> supported_ext = {
    "emul", "spc", "gym", "nsf", "nsfe", "gbs", "gbr", "ay",
    "sap",  "vgm", "vgz", "hes", "kss",  "sgc"};

// Read the SAP "TYPE" tag from the text header. Returns the type letter (e.g.
// 'B', 'C', 'D', 'S') or 0 if the file is not a readable SAP header. GME's
// Sap_Emu only plays the POKEY register types B and C (see Sap_Emu.cpp); the
// others -- notably 'D' (Digimusic, digitized samples) -- it rejects outright.
static char sapType(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) { return 0; }
    char buf[1024];
    in.read(buf, sizeof(buf));
    auto n = static_cast<size_t>(in.gcount());
    if (n < 4 || std::memcmp(buf, "SAP", 3) != 0) { return 0; }
    for (size_t i = 0; i + 5 < n; i++) {
        // The ASCII header ends at the 0xFF 0xFF binary marker.
        if (static_cast<unsigned char>(buf[i]) == 0xFF &&
            static_cast<unsigned char>(buf[i + 1]) == 0xFF) {
            break;
        }
        if (std::memcmp(buf + i, "TYPE ", 5) == 0) {
            char t = buf[i + 5];
            return (t >= 'A' && t <= 'Z') ? t : 0;
        }
    }
    return 0;
}

bool GMEPlugin::canHandle(const std::string& name)
{
    auto ext = utils::path_extension(name);
    if (supported_ext.count(ext) == 0) { return false; }
    // GME plays only SAP types B and C. Let the other types fall through to the
    // ASAP-based PokeyNoise plugin, which handles every SAP type. If the header
    // is unreadable (e.g. a dry canHandle on a virtual path) keep claiming it so
    // nothing regresses.
    if (ext == "sap") {
        char t = sapType(name);
        if (t != 0 && t != 'B' && t != 'C') { return false; }
    }
    // GME's Vgm_Emu only decodes SN76489/YM2413/YM2612/AY8910; any other chip
    // (OPL, the OPN family, HuC6280, NES APU, GameBoy, the Neo Geo / arcade
    // sample chips, ...) renders silent or aborts (Blip_Buffer assertion on some
    // OPL2). Decline those so libvgmplugin claims them; Sega/AY console VGZ (the
    // bulk of what GME does well) still play here.
    if (ext == "vgm" || ext == "vgz") {
        if (vgmNeedsLibVGM(name)) { return false; }
    }
    return true;
}

std::set<std::string> GMEPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* GMEPlugin::fromFile(const std::string& name)
{
    return new GMEPlayer{name};
};

} // namespace musix
//
extern "C" void gmeplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& config) {
        return std::make_shared<musix::GMEPlugin>();
    });
}
