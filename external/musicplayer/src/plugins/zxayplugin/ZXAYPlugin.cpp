#include "ZXAYPlugin.h"

#include "zxay_format.h"
#include "zxay_source.h"

#include <coreutils/file.h>
#include <coreutils/utils.h>

#include <algorithm>

namespace musix {

namespace {

constexpr int kSampleRate = 44100;

std::vector<uint8_t> readFile(const std::string& name)
{
    utils::File file{name};
    if (!file.exists()) {
        return {};
    }
    return file.readAll();
}

} // namespace

class ZXAYPlayer : public ChipPlayer
{
public:
    explicit ZXAYPlayer(const std::string& fileName)
    {
        auto data = readFile(fileName);
        if (data.empty()) {
            throw player_exception("ZX AY: cannot read file");
        }
        auto ext = utils::toLower(utils::path_extension(fileName));
        zxay::Format format = zxay::Format::unknown;
        source_ = zxay::createSource(std::move(data), ext, kSampleRate, &format);
        if (!source_) {
            throw player_exception("ZX AY: not a supported AY module");
        }
        const auto& info = source_->info();
        setMeta("title", info.title, "composer", info.author, "length",
                info.lengthSeconds, "format", zxay::formatName(format),
                "channels", 2);
    }

    int getSamples(int16_t* target, int noSamples) override
    {
        // noSamples counts int16s; the sources render interleaved stereo.
        const int frames = source_->render(target, noSamples / 2);
        if (frames == 0) {
            return -1; // end of song
        }
        return frames * 2;
    }

    bool seekTo(int /*song*/, int /*seconds*/) override { return false; }

private:
    std::unique_ptr<zxay::Source> source_;
};

bool ZXAYPlugin::canHandle(const std::string& name)
{
    auto ext = utils::toLower(utils::path_extension(name));
    if (!zxay::isSupportedExtension(ext)) {
        return false;
    }
    // Beyond the extension, decide on CONTENT. It is the only way to keep the
    // Picatune2 ".pt2" beeper projects out (they are XML, and nothing here or
    // anywhere else in the app can play them, so they must Skip rather than
    // fail), and it is what stops a mis-named file being claimed and then
    // failing to load.
    //
    // canHandle is called by the host OUTSIDE its try/catch, so it must never
    // throw -- swallow any IO error and decline. A file we cannot read yet
    // (a virtual path during a dry canHandle) keeps the extension's claim.
    try {
        utils::File file{name};
        if (!file.exists()) {
            return true;
        }
        auto data = file.readAll();
        return zxay::detect(data.data(), data.size(), ext) !=
               zxay::Format::unknown;
    } catch (std::exception&) {
        return true;
    }
}

std::set<std::string> ZXAYPlugin::getSupportedExtensions() const
{
    return {"pt1", "pt2", "pt3", "stc", "st13", "zxs", "stp",  "stp2",
            "asc", "psc", "sqt", "vtx", "psg",  "fxm", "amad", "vt2"};
}

ChipPlayer* ZXAYPlugin::fromFile(const std::string& name)
{
    return new ZXAYPlayer{name};
}

} // namespace musix

extern "C" void zxayplugin_register()
{
    musix::ChipPlugin::addPluginConstructor(
        [](std::string const& /*config*/) {
            return std::make_shared<musix::ZXAYPlugin>();
        });
}
