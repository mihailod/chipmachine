#include "DMFCRPlugin.h"
#include "../../chipplayer.h"

#include "dmf_file.h"
#include "dmf_player.h"

#include <cctype>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace musix {

namespace {

std::string lowerExt(std::string const& path)
{
    auto dot = path.find_last_of('.');
    if (dot == std::string::npos) { return ""; }
    std::string ext = path.substr(dot + 1);
    for (auto& c : ext) { c = static_cast<char>(tolower((unsigned char)c)); }
    return ext;
}

std::vector<uint8_t> readFile(std::string const& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) { return {}; }
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
}

} // namespace

class DMFCRPlayer : public ChipPlayer
{
public:
    explicit DMFCRPlayer(const std::string& fileName)
    {
        auto data = readFile(fileName);
        if (data.empty()) { throw player_exception("cannot read file"); }

        std::string err;
        if (!dmfcr::loadDmf(data.data(), data.size(), module_, err)) {
            throw player_exception(err);
        }
        if (!player_.init(module_, 44100, err)) { throw player_exception(err); }

        setMeta("title", module_.songName, "composer", module_.songAuthor,
                "format", "DefleMask", "channels", module_.totalChannels,
                "songs", 1, "length", 0);
    }

    int getSamples(int16_t* target, int noSamples) override
    {
        // The host asks for interleaved stereo samples, so noSamples counts
        // shorts, not frames.
        int frames = noSamples / 2;
        if (frames <= 0) { return 0; }

        // Report SONG_END only when the order list ran off its end with no jump
        // taken. A backward Bxx means the module asked to repeat, and then it
        // plays until the host stops it -- which is Furnace's documented default
        // for trackers, so the two variants agree in the common case.
        //
        // KNOWN GAP: Furnace also stops some one-shots that this does not, and
        // the exact rule is not recoverable without reading its source (see
        // README). Erring towards playing on is the safer of the two -- a jingle
        // that repeats is a nuisance, one that is cut short loses music.
        if (player_.ended()) { return -1; }

        scratch_.resize(static_cast<size_t>(frames) * 2);
        player_.render(scratch_.data(), frames);

        for (int i = 0; i < frames * 2; i++) {
            float s = scratch_[i];
            if (s > 1.0f) { s = 1.0f; }
            if (s < -1.0f) { s = -1.0f; }
            target[i] = static_cast<int16_t>(s * 32767.0f);
        }
        return frames * 2;
    }

    int getHZ() override { return 44100; }

private:
    dmfcr::Module module_;
    dmfcr::Player player_;
    std::vector<float> scratch_;
};

bool DMFCRPlugin::canHandle(const std::string& name)
{
    if (lowerExt(name) != "dmf") { return false; }
    auto data = readFile(name);
    if (data.size() < 2 || data[0] != 0x78) {
        return false; // X-Tracker "DDMF" is not zlib-wrapped; OpenMPT takes those
    }
    // Content-gate properly rather than on the zlib byte alone: this plugin
    // covers plain Genesis and SMS at DMF versions 0x11-0x18 and must DECLINE any
    // other system, so that in the plus build those files fall through to Furnace
    // instead of being claimed and played wrongly.
    dmfcr::Module m;
    std::string err;
    if (!dmfcr::loadDmf(data.data(), data.size(), m, err)) { return false; }
    return dmfcr::playableSystem(m);
}

std::set<std::string> DMFCRPlugin::getSupportedExtensions() const
{
    return { "dmf" };
}

ChipPlayer* DMFCRPlugin::fromFile(const std::string& fileName)
{
    return new DMFCRPlayer{ fileName };
}

} // namespace musix

extern "C" void dmfcrplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::DMFCRPlugin>();
    });
}
