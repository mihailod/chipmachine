#include "LibVGMPlugin.h"
#include "../../chipplayer.h"
#include "../../vgm_opl_detect.h"

#include <coreutils/utils.h>

#include "player/playera.hpp"
#include "player/playerbase.hpp"
#include "player/vgmplayer.hpp"
#include "utils/DataLoader.h"
#include "utils/FileLoader.h"

#include <cstdint>
#include <set>
#include <string>

namespace musix {

namespace {
constexpr uint32_t kSampleRate = 44100;
constexpr uint32_t kBufferFrames = 2048; // internal libvgm render buffer
constexpr uint32_t kLoopCount = 2;       // loops before fade for looping tunes
constexpr double kFadeSeconds = 4.0;
} // namespace

class LibVGMPlayer : public ChipPlayer
{
public:
    explicit LibVGMPlayer(const std::string& fileName)
    {
        loader = FileLoader_Init(fileName.c_str());
        if (loader == nullptr) {
            throw player_exception("libvgm: could not open file");
        }
        // libvgm needs the header preloaded to identify the format.
        DataLoader_SetPreloadBytes(loader, 0x100);
        if (DataLoader_Load(loader) != 0) {
            DataLoader_Deinit(loader);
            loader = nullptr;
            throw player_exception("libvgm: could not load file");
        }

        player.RegisterPlayerEngine(new VGMPlayer);
        if (player.SetOutputSettings(kSampleRate, 2, 16, kBufferFrames) != 0) {
            cleanup();
            throw player_exception("libvgm: unsupported output settings");
        }

        PlayerA::Config cfg = player.GetConfiguration();
        cfg.masterVol = 0x10000; // 1.0
        cfg.loopCount = kLoopCount;
        cfg.fadeSmpls = static_cast<uint32_t>(kSampleRate * kFadeSeconds);
        cfg.endSilenceSmpls = kSampleRate / 2;
        cfg.pbSpeed = 1.0;
        player.SetConfiguration(cfg);

        if (player.LoadFile(loader) != 0) {
            cleanup();
            throw player_exception("libvgm: LoadFile failed");
        }

        PlayerBase* engine = player.GetPlayer();
        if (engine->GetPlayerType() == FCC_VGM) {
            auto* vgm = dynamic_cast<VGMPlayer*>(engine);
            if (vgm != nullptr) {
                player.SetLoopCount(vgm->GetModifiedLoopCount(kLoopCount));
            }
        }

        // Start() must run before any timing query (it sets the rate multipliers).
        player.Start();
        started = true;

        int length = static_cast<int>(player.GetTotalTime(
            PLAYTIME_LOOP_INCL | PLAYTIME_TIME_FILE | PLAYTIME_WITH_FADE));
        setMeta("length", length, "format", "VGM");
    }

    ~LibVGMPlayer() override { cleanup(); }

    int getSamples(int16_t* target, int size) override
    {
        if (!started) {
            player.Start();
            started = true;
        }
        if ((player.GetState() & PLAYSTATE_FIN) != 0) {
            return -1; // SONG_END: tell the host to stop (returning 0 loops it)
        }
        // Render takes a byte count; size is the number of int16 samples.
        uint32_t bytes =
            player.Render(size * sizeof(int16_t), static_cast<void*>(target));
        return static_cast<int>(bytes / sizeof(int16_t));
    }

    bool seekTo(int /*song*/, int seconds) override
    {
        if (seconds >= 0) {
            player.Seek(PLAYPOS_SAMPLE,
                        static_cast<uint32_t>(seconds) * kSampleRate);
        }
        return true;
    }

private:
    void cleanup()
    {
        player.Stop();
        player.UnloadFile();
        player.UnregisterAllPlayers();
        if (loader != nullptr) {
            DataLoader_Deinit(loader);
            loader = nullptr;
        }
    }

    PlayerA player;
    DATA_LOADER* loader = nullptr;
    bool started = false;
};

static const std::set<std::string> supported_ext = {"vgm", "vgz"};

bool LibVGMPlugin::canHandle(const std::string& name)
{
    auto ext = utils::path_extension(name);
    if (supported_ext.count(ext) == 0) { return false; }
    // Claim any VGM whose chips GME can't decode (OPL, YM2151, the OPN family,
    // HuC6280, NES APU, GameBoy, the Neo Geo / arcade sample chips, ...). GME
    // keeps the Sega/AY logs it renders well.
    return vgmNeedsLibVGM(name);
}

std::set<std::string> LibVGMPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* LibVGMPlugin::fromFile(const std::string& fileName)
{
    return new LibVGMPlayer{fileName};
}

} // namespace musix

extern "C" void libvgmplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::LibVGMPlugin>();
    });
}
