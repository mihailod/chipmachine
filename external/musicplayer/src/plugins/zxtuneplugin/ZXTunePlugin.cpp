#include "ZXTunePlugin.h"

#include <coreutils/utils.h>

// ZXTune engine (vendored under chipmachine-as/zxtune/)
#include <binary/container_factories.h>
#include <core/service.h>
#include <core/module_detect.h>
#include <core/data_location.h>
#include <core/plugin.h>
#include <module/holder.h>
#include <module/renderer.h>
#include <module/information.h>
#include <module/attributes.h>
#include <parameters/container.h>
#include <parameters/accessor.h>
#include <sound/chunk.h>
#include <sound/sample.h>
#include <time/instant.h>
#include <error.h>

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <set>
#include <vector>

namespace musix {

namespace {

constexpr int SAMPLE_RATE = 44100;


// Collects the modules ZXTune detects inside a file. The Modland Sound Tracker
// rips contain exactly one module, so we keep the first holder found.
struct FirstModuleCollector : Module::DetectCallback
{
    Module::Holder::Ptr holder;

    Parameters::Container::Ptr
    CreateInitialProperties(StringView /*subpath*/) const override
    {
        return Parameters::Container::Create();
    }
    void ProcessModule(const ZXTune::DataLocation& /*location*/,
                       const ZXTune::Plugin& /*decoder*/,
                       Module::Holder::Ptr h) override
    {
        if (!holder) { holder = std::move(h); }
    }
    Log::ProgressCallback* GetProgress() const override { return nullptr; }
};

} // namespace

class ZXTunePlayer : public ChipPlayer
{
public:
    explicit ZXTunePlayer(const std::string& fileName)
    {
        std::ifstream in(fileName, std::ios::binary);
        if (!in) { throw player_exception("Could not open file"); }
        std::vector<uint8_t> raw((std::istreambuf_iterator<char>(in)),
                                 std::istreambuf_iterator<char>());
        if (raw.empty()) { throw player_exception("Empty file"); }

        try {
            auto data = Binary::CreateContainer(
                Binary::View(raw.data(), raw.size()));
            auto service =
                ZXTune::Service::Create(Parameters::Container::Create());

            FirstModuleCollector collector;
            service->DetectModules(data, collector);
            if (!collector.holder) {
                throw player_exception("ZXTune: no playable module");
            }

            auto props = collector.holder->GetModuleProperties();
            renderer = collector.holder->CreateRenderer(SAMPLE_RATE, props);

            Parameters::StringType type, title, author;
            Parameters::FindValue(*props, Module::ATTR_TYPE, type);
            Parameters::FindValue(*props, Module::ATTR_TITLE, title);
            Parameters::FindValue(*props, Module::ATTR_AUTHOR, author);

            auto info = collector.holder->GetModuleInformation();
            uint32_t lengthSec =
                static_cast<uint32_t>(info->Duration().Get() / 1000);

            setMeta("title", title, "composer", author, "length", lengthSec,
                    "format",
                    type.empty() ? std::string("ZX (ZXTune)")
                                 : (type + " (ZXTune)"));
        } catch (const Error& e) {
            throw player_exception("ZXTune: " + e.ToString());
        }
    }

    // Host counts interleaved int16 values (stereo L,R,L,R...). ZXTune renders
    // one frame (~20ms) per Render() call, so we buffer the remainder between
    // calls. Render() yields an empty chunk when the track is done.
    int getSamples(int16_t* target, int noSamples) override
    {
        int written = 0;
        while (written < noSamples) {
            if (bufferPos >= buffer.size()) {
                if (ended) { break; }
                Sound::Chunk chunk = renderer->Render();
                if (chunk.empty()) {
                    ended = true;
                    break;
                }
                buffer.clear();
                bufferPos = 0;
                buffer.reserve(chunk.size() * Sound::Sample::CHANNELS);
                for (const auto& s : chunk) {
                    buffer.push_back(static_cast<int16_t>(s.Left()));
                    buffer.push_back(static_cast<int16_t>(s.Right()));
                }
            }
            int avail = static_cast<int>(buffer.size() - bufferPos);
            int take = std::min(avail, noSamples - written);
            std::memcpy(target + written, buffer.data() + bufferPos,
                        take * sizeof(int16_t));
            written += take;
            bufferPos += take;
        }
        if (written == 0 && ended) { return -1; }
        return written;
    }

    bool seekTo(int /*song*/, int seconds) override
    {
        if (!renderer || seconds < 0) { return false; }
        renderer->SetPosition(Time::AtMillisecond(
            static_cast<uint64_t>(seconds) * 1000));
        ended = false;
        buffer.clear();
        bufferPos = 0;
        return true;
    }

private:
    Module::Renderer::Ptr renderer;
    std::vector<int16_t> buffer;
    size_t bufferPos{0};
    bool ended{false};
};

// ZXTune ignores the extension and detects every format by content; this set
// only gates which files we hand to the engine. Each entry is verified to (a)
// actually render sound in the trimmed AY/DAC/SAA/FM build and (b) be unclaimed
// by any higher-or-equal-priority plugin (or freed up for us, see psm).
//   st11: Sound Tracker 1.1  (raw ST1 wrapped as ZXAYST11 by the modland fetch)
//   gtr:  Global Tracker (ZX Spectrum AY)      -- aym/globaltracker    (~38 mods)
//   chi:  Chip Tracker (ZX Spectrum DAC)       -- dac/chiptracker      (~21 mods)
//   tfe:  TFM Music Maker (TurboSound-FM)      -- tfm/tfmmusicmaker     (~77 mods)
//   ftc:  Fast Tracker (ZX Spectrum AY)        -- aym/fasttracker      (~51 mods)
//         Taken over from AyflyPlugin, which throws on every .ftc (0/12 vs our
//         12/12 on spread modland samples); ayfly no longer claims it.
//   psm:  ZX "Pro Sound Maker" (AY)            -- aym/prosoundmaker     (~51 mods)
//         .psm is also Epic MegaGames MASI, which OpenMPT keeps; OpenMPT now
//         content-checks the MASI magic and declines the ZX variant so it
//         reaches us here (see OpenMPTPlugin::canHandle).
// NB: ".cop" (Sam Coupe COP / SAA1099) is intentionally NOT here. ZXTune's own
// COP loader only matches the zxart 5-byte-header E-Tracker variant and fails on
// the modland corpus (10-byte-header data files + raw-Z80 "compiled" songs). Its
// sibling CopPlugin (GME Z80 + SAASound) decodes the whole family by running the
// song's compiled replayer, so it now owns the extension outright -- letting
// ZXTune claim ".cop" only produced "no playable module" errors on the songs
// whose entry preambles CopPlugin's old signature gate hadn't enumerated yet.
static const std::set<std::string> supported_ext = {
    "st11", "gtr", "chi", "tfe", "psm", "ftc"};

bool ZXTunePlugin::canHandle(const std::string& name)
{
    auto ext = utils::path_extension(name);
    if (supported_ext.count(ext) == 0) {
        return false;
    }
    return true;
}

std::set<std::string> ZXTunePlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* ZXTunePlugin::fromFile(const std::string& fileName)
{
    return new ZXTunePlayer{fileName};
}

} // namespace musix

extern "C" void zxtuneplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::ZXTunePlugin>();
    });
}
