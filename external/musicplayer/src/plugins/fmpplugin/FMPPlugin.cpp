#include "FMPPlugin.h"

#include <coreutils/utils.h>
#include <coreutils/file.h>

// fmgen OPNA emulator (from s98plugin)
#include "../s98plugin/m_s98/device/fmgen/opna.h"

// 98fmplayer FMP driver — suppress C++11 narrowing in leveldata.h
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++11-narrowing"
extern "C" {
#include "fmdriver_fmp.h"
#include "fmdriver_common.h"
}
#pragma clang diagnostic pop

#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace musix {

static const int SAMPLE_RATE = 48000;
static const int OPNA_CLOCK  = 7987200;
static const int MAX_CHUNK   = 1024; // max frames per render slice

struct FMPContext {
    FM::OPNA opna;
    fmdriver_work work;
    driver_fmp fmp;
    std::vector<uint8_t> filedata;
};

// ---------------------------------------------------------------------------
// Bridge callbacks
// ---------------------------------------------------------------------------

static void opna_writereg(fmdriver_work* work, unsigned addr, unsigned data)
{
    auto* ctx = static_cast<FMPContext*>(work->opna);
    ctx->opna.SetReg(addr, data);
}

static unsigned opna_readreg(fmdriver_work* work, unsigned addr)
{
    (void)work; (void)addr;
    return 0;
}

static uint8_t opna_status(fmdriver_work* work, bool a1)
{
    auto* ctx = static_cast<FMPContext*>(work->opna);
    (void)a1;
    return (uint8_t)ctx->opna.ReadStatus();
}

// ---------------------------------------------------------------------------
// Player
// ---------------------------------------------------------------------------

class FMPPlayer : public ChipPlayer {
public:
    explicit FMPPlayer(const std::string& fileName)
    {
        ctx = new FMPContext();
        ctx->filedata = utils::File(fileName).readAll();

        memset(&ctx->work, 0, sizeof(ctx->work));
        memset(&ctx->fmp,  0, sizeof(ctx->fmp));

        ctx->work.opna         = ctx;
        ctx->work.opna_writereg = opna_writereg;
        ctx->work.opna_readreg  = opna_readreg;
        ctx->work.opna_status   = opna_status;

        if (!ctx->opna.Init(OPNA_CLOCK, SAMPLE_RATE, false, nullptr)) {
            delete ctx;
            ctx = nullptr;
            throw player_exception();
        }
        ctx->opna.Reset();
        ctx->opna.SetReg(0x29, 0x9f); // enable all FM channels

        if (!fmp_load(&ctx->fmp, ctx->filedata.data(),
                      static_cast<uint16_t>(ctx->filedata.size()))) {
            delete ctx;
            ctx = nullptr;
            throw player_exception();
        }

        fmp_init(&ctx->work, &ctx->fmp);
    }

    ~FMPPlayer() override
    {
        delete ctx;
    }

    int getSamples(int16_t* target, int noSamples) override
    {
        if (!ctx) return -1;
        if (ctx->fmp.status.stopped) return -1;

        int32_t mixbuf[MAX_CHUNK * 2];
        int produced = 0;

        while (produced < noSamples) {
            // Ask fmgen how many microseconds until the next timer event.
            // Cap to a reasonable slice so we don't stall on idle timers.
            int32_t next_us = ctx->opna.GetNextEvent();
            if (next_us <= 0 || next_us > 20000) next_us = 1000; // ~1ms fallback

            // Convert microseconds → stereo frames, capped to available space.
            int frames = (int64_t)next_us * SAMPLE_RATE / 1000000;
            if (frames <= 0) frames = 1;
            int avail = (noSamples - produced) / 2;
            if (frames > avail) frames = avail;
            if (frames > MAX_CHUNK) frames = MAX_CHUNK;
            if (frames <= 0) break;

            // Render audio for this slice.
            memset(mixbuf, 0, sizeof(int32_t) * frames * 2);
            ctx->opna.Mix(mixbuf, frames);

            // Clip int32 → int16
            for (int i = 0; i < frames * 2; i++) {
                int32_t s = mixbuf[i];
                if (s >  32767) s =  32767;
                if (s < -32768) s = -32768;
                target[produced + i] = static_cast<int16_t>(s);
            }
            produced += frames * 2;

            // Advance the fmgen timer by the exact number of µs we rendered.
            int32_t actual_us = (int64_t)frames * 1000000 / SAMPLE_RATE;
            ctx->opna.Count(actual_us);

            // Let the FMP driver check the timer-B status flag and tick the
            // sequencer if it fired. fmp_opna_interrupt reads opna_status()
            // internally; fmp_timerb resets the flag via register 0x27.
            ctx->work.driver_opna_interrupt(&ctx->work);

            if (ctx->fmp.status.stopped) break;
        }
        return produced > 0 ? produced : -1;
    }

    bool seekTo(int /*song*/, int /*seconds*/) override { return false; }

private:
    FMPContext* ctx = nullptr;
};

// ---------------------------------------------------------------------------
// Plugin
// ---------------------------------------------------------------------------

static const std::set<std::string> supported_ext = {"opi", "ovi", "ozi"};

bool FMPPlugin::canHandle(const std::string& name)
{
    return supported_ext.count(utils::toLower(utils::path_extension(name))) > 0;
}

std::set<std::string> FMPPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* FMPPlugin::fromFile(const std::string& fileName)
{
    return new FMPPlayer{fileName};
}

} // namespace musix

extern "C" void fmpplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& config) {
        return std::make_shared<musix::FMPPlugin>();
    });
}
