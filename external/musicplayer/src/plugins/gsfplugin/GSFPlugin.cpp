#include "GSFPlugin.h"
#include "../../chipplayer.h"
#include "GsfRom.h"

#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <psf/PSFFile.h>

extern "C"
{
#include <mgba-util/audio-buffer.h>
#include <mgba-util/audio-resampler.h>
#include <mgba-util/vfs.h>
#include <mgba/core/config.h>
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/gba/core.h>
}

#include <algorithm>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace musix {

namespace {

// The GBA's own audio rate depends on what the game writes to SOUNDBIAS
// (16.78 MHz / sampleInterval -- 32768 Hz for most rips, but 65536 and 16384
// occur, and a game may change it mid-tune). getHZ() is asked once, so resample
// to a fixed rate inside the plugin instead of exposing a moving target. 44100
// is also what the old VBA core produced, which keeps the swap a like-for-like
// change everywhere downstream.
constexpr int kRate = 44100;

// A frame of GBA audio is at most a few hundred samples; this is roughly a
// third of a second of slack on either side of the resampler.
constexpr size_t kBufferFrames = 16384;

// mGBA logs through a global default logger and CALLS IT during core->init().
// Leaving it unset is a jump through a null vtable, not a silent no-op. A
// music player has nowhere to show emulator chatter, so the sink drops
// everything; route it to LOGD when debugging a specific rip.
void quietLog(struct mLogger*, int, enum mLogLevel, const char*, va_list) {}

void installLogger()
{
    static struct mLogger logger = [] {
        struct mLogger l{};
        l.log = quietLog;
        return l;
    }();
    static bool once = [] {
        mLogSetDefaultLogger(&logger);
        return true;
    }();
    (void)once;
}

} // namespace

class GSFPlayer : public ChipPlayer
{
public:
    explicit GSFPlayer(const std::string& fileName)
    {
        installLogger();

        rom = gsf::loadRom(fileName);
        if (!rom.valid) {
            throw player_exception("GSFPlugin error: Could not load gsf");
        }

        core = GBACoreCreate();
        if (core == nullptr) {
            throw player_exception("GSFPlugin error: no GBA core");
        }
        core->init(core);

        // Mandatory even though we only want audio: mGBA's software renderer
        // writes scanlines unconditionally, so without a target it stores
        // through a null pointer on the first frame.
        unsigned w = 0;
        unsigned h = 0;
        core->baseVideoSize(core, &w, &h);
        videoBuffer.resize(static_cast<size_t>(w) * h);
        core->setVideoBuffer(core, videoBuffer.data(), w);

        mCoreConfigInit(&core->config, nullptr);
        // core->opts is zero-initialised and mCoreLoadConfig only overwrites
        // keys that are actually present, so without these two the GBA core
        // sets masterVolume = 0 and every rip renders silence.
        mCoreConfigSetDefaultIntValue(&core->config, "volume", 0x100);
        mCoreConfigSetDefaultIntValue(&core->config, "mute", 0);
        // Idle-loop skipping rewrites timing to reach the next event sooner. It
        // is a speed hack for interactive play; for offline rendering it only
        // adds a way for a tune to come out subtly wrong.
        mCoreConfigSetDefaultValue(&core->config, "idleOptimization", "ignore");
        mCoreLoadConfig(core);

        core->setAudioBufferSize(core, 2048);

        // VFileFromConstMemory does not copy -- mGBA maps this pointer straight
        // into the cartridge window, so `rom` must outlive the core and must
        // never be resized after this point.
        struct VFile* vf =
            VFileFromConstMemory(rom.data.data(), rom.data.size());
        if (!core->loadROM(core, vf)) {
            // loadROM takes ownership on success only. And a throw from a
            // constructor skips ~GSFPlayer, so unwind the core by hand.
            vf->close(vf);
            mCoreConfigDeinit(&core->config);
            core->deinit(core);
            core = nullptr;
            throw player_exception("GSFPlugin error: Could not load gsf");
        }
        core->reset(core);

        if (rom.entryPoint != 0 && rom.entryPoint != 0x08000000) {
            // Every rip seen so far enters at the cartridge base, which is
            // where a reset lands anyway. Worth knowing if one does not.
            LOGD("GSF: non-standard entry point {:#x}", rom.entryPoint);
        }

        mAudioBufferInit(&outBuffer, kBufferFrames, 2);
        mAudioResamplerInit(&resampler, mINTERPOLATOR_SINC);
        sourceRate = core->audioSampleRate(core);
        mAudioResamplerSetSource(&resampler, core->getAudioBuffer(core),
                                 sourceRate, true);
        mAudioResamplerSetDestination(&resampler, &outBuffer, kRate);

        PSFFile psf{fileName};
        if (psf.valid()) {
            auto& tags = psf.tags();
            setMeta("composer", tags["artist"], "sub_title", tags["title"],
                    "game", tags["game"], "format", "Gameboy Advance", "length",
                    static_cast<int>(psf.songLength()));
        }

        LOGD("GSF:{}", fileName);
    }

    ~GSFPlayer() override
    {
        if (core != nullptr) {
            mAudioResamplerDeinit(&resampler);
            mAudioBufferDeinit(&outBuffer);
            mCoreConfigDeinit(&core->config);
            core->deinit(core);
        }
    }

    GSFPlayer(const GSFPlayer&) = delete;
    GSFPlayer& operator=(const GSFPlayer&) = delete;

    int getHZ() override { return kRate; }

    int getSamples(int16_t* target, int noSamples) override
    {
        // `noSamples` counts interleaved int16s; mAudioBuffer counts frames.
        size_t wanted = static_cast<size_t>(noSamples) / 2;
        if (wanted == 0) { return 0; }

        // Drain as we go rather than filling the whole request first: the host
        // asks for as much as fits in its audio FIFO (~65k frames), which is
        // far more than outBuffer holds, so waiting for it to hold `wanted`
        // would spin forever once the resampler's destination filled up.
        size_t written = 0;
        int stalls = 0;
        while (written < wanted) {
            size_t avail = mAudioBufferAvailable(&outBuffer);
            if (avail == 0) {
                // A game may reprogram SOUNDBIAS mid-tune; the resampler has to
                // be told, or everything after the change plays at the wrong
                // pitch.
                unsigned rate = core->audioSampleRate(core);
                if (rate != sourceRate && rate != 0) {
                    sourceRate = rate;
                    mAudioResamplerSetSource(&resampler,
                                             core->getAudioBuffer(core),
                                             sourceRate, true);
                }
                core->runFrame(core);
                mAudioResamplerProcess(&resampler);
                if (mAudioBufferAvailable(&outBuffer) == 0) {
                    // A rip whose driver has wedged produces no samples at all.
                    // Silence still counts as samples, so this only fires on a
                    // genuinely dead core.
                    if (++stalls > 100) {
                        return written > 0 ? static_cast<int>(written * 2) : -1;
                    }
                }
                continue;
            }
            stalls = 0;
            size_t take = std::min(avail, wanted - written);
            size_t got =
                mAudioBufferRead(&outBuffer, target + written * 2, take);
            if (got == 0) { break; }
            written += got;
        }
        return static_cast<int>(written * 2);
    }

private:
    gsf::RomImage rom;
    struct mCore* core = nullptr;
    std::vector<mColor> videoBuffer;
    struct mAudioBuffer outBuffer{};
    struct mAudioResampler resampler{};
    unsigned sourceRate = 0;
};

static const std::set<std::string> supported_ext{"gsf", "minigsf"};

bool GSFPlugin::canHandle(const std::string& name)
{
    return supported_ext.count(utils::path_extension(name)) > 0;
}

std::set<std::string> GSFPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

std::vector<std::string> GSFPlugin::getSecondaryFiles(const std::string& name)
{
    // .gsf/.minigsf rips reference a shared .gsflib via PSF "_lib" tags; the
    // loader opens it from the file's own dir, so it must be fetched alongside
    // when streaming (else loading fails: "Could not load gsf").
    return psfLibFiles(name);
}

ChipPlayer* GSFPlugin::fromFile(const std::string& fileName)
{
    return new GSFPlayer{fileName};
}

} // namespace musix

extern "C" void gsfplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::GSFPlugin>();
    });
}
