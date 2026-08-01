#include "MusPlugin.h"

#include <coreutils/file.h>
#include <coreutils/utils.h>

extern "C" {
#include "mus.h"
}

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace musix {

namespace {

// The sequencer drives cSID's chip emulation, whose state lives in file-scope
// globals -- so, exactly like csidplugin and the VICE bridge before it, the
// engine is a process-wide singleton. Guard it and let whichever player renders
// next take ownership by reloading its own tune.
std::mutex engineMutex;
void const* engineOwner = nullptr;

// cSID runs hotter than VICE/reSID; csidplugin measured the ratio at ~1.83 and
// applies the same correction, so SID material sits at one consistent level
// across both plugins.
constexpr int kGainNum = 55;
constexpr int kGainDen = 100;

std::string strForMus(std::string const& file)
{
    auto lower = utils::toLower(file);
    if (utils::endsWith(lower, ".mus")) {
        return file.substr(0, file.size() - 4) + ".str";
    }
    return {};
}

// A .str is only ever the second half of a pair; the .mus is what we load.
std::string musForStr(std::string const& file)
{
    auto lower = utils::toLower(file);
    if (utils::endsWith(lower, ".str")) {
        return file.substr(0, file.size() - 4) + ".mus";
    }
    return file;
}

} // namespace

// Named to avoid clashing with the C sequencer's own MusPlayer struct.
class MusChipPlayer : public ChipPlayer
{
public:
    MusChipPlayer(std::vector<uint8_t> mus, std::vector<uint8_t> str)
        : musData(std::move(mus)), strData(std::move(str))
    {
        std::lock_guard<std::mutex> lk{ engineMutex };
        player = mus_create(musData.data(), static_cast<int>(musData.size()),
                            strData.empty() ? nullptr : strData.data(),
                            static_cast<int>(strData.size()), 44100);
        if (player == nullptr) {
            throw player_exception("Sidplayer: not a .mus file");
        }
        engineOwner = this;

        // The trailing text block is free-form credit lines rather than a
        // structured title, and the catalog's own metadata is better curated,
        // so publish only what the player actually knows.
        setMeta("songs", static_cast<uint32_t>(1));
        setMeta("startSong", static_cast<uint32_t>(0));
    }

    ~MusChipPlayer() override
    {
        std::lock_guard<std::mutex> lk{ engineMutex };
        if (player != nullptr) { mus_destroy(player); }
        if (engineOwner == this) { engineOwner = nullptr; }
    }

    int getSamples(int16_t* target, int size) override
    {
        int const frames = size / 2;
        if (frames <= 0) { return 0; }
        // Interleaved stereo straight from the sequencer -- a Stereo Sidplayer
        // pair is genuinely two-channel (first SID left, second right), so this
        // must NOT be a mono render duplicated across both.
        if (static_cast<int>(mono.size()) < frames * 2) { mono.resize(frames * 2); }

        {
            std::lock_guard<std::mutex> lk{ engineMutex };
            if (engineOwner != this) {
                // Another player took the engine; rebuild ours and reclaim it.
                mus_destroy(player);
                player = mus_create(
                    musData.data(), static_cast<int>(musData.size()),
                    strData.empty() ? nullptr : strData.data(),
                    static_cast<int>(strData.size()), 44100);
                if (player == nullptr) { return -1; }
                engineOwner = this;
            }
            mus_render(player, mono.data(), frames);
        }

        for (int i = 0; i < frames * 2; i++) {
            target[i] = static_cast<int16_t>(mono[i] * kGainNum / kGainDen);
        }
        return frames * 2;
    }

private:
    std::vector<uint8_t> musData;
    std::vector<uint8_t> strData;
    std::vector<int16_t> mono;
    ::MusPlayer* player = nullptr; // the C sequencer instance
};

static const std::set<std::string> supported_ext{ "mus", "str" };

bool MusPlugin::canHandle(const std::string& name)
{
    return supported_ext.count(utils::path_extension(utils::toLower(name))) > 0;
}

std::set<std::string> MusPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

std::vector<std::string> MusPlugin::getSecondaryFiles(const std::string& file)
{
    // A ".str" is useless alone -- it holds voices 4-6 and needs its ".mus".
    // Mirrors what the VICE bridge reports, so the loader fetches both.
    if (utils::endsWith(utils::toLower(file), ".str")) {
        auto mus = std::filesystem::path(file).filename().string();
        mus = mus.substr(0, mus.size() - 4) + ".mus";
        return { mus };
    }
    return {};
}

ChipPlayer* MusPlugin::fromFile(const std::string& fileName)
{
    auto musPath = musForStr(fileName);
    utils::File mf{ musPath };
    if (!mf.exists()) {
        throw player_exception("Sidplayer: .mus not found");
    }
    auto musData = mf.readAll();

    // Pick up the stereo companion when it is sitting next to the .mus.
    std::vector<uint8_t> strData;
    auto strPath = strForMus(musPath);
    if (!strPath.empty()) {
        utils::File sf{ strPath };
        if (sf.exists()) { strData = sf.readAll(); }
    }
    return new MusChipPlayer{ std::move(musData), std::move(strData) };
}

} // namespace musix

extern "C" void musplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::MusPlugin>();
    });
}
