#include "MonotonePlugin.h"
#include "../../chipplayer.h"

extern "C" {
#include "ptplayer/ptplayer.h"
}

#include <coreutils/file.h>
#include <coreutils/log.h>
#include <coreutils/utils.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace musix {

// MONOTONE (Trixter/Hornet) is a PC-speaker tracker: up to a dozen square-wave
// tracks summed into a single 1-bit output. Files begin with the 9-byte magic
// "\x08MONOTONE" and are only a couple of kB. PTPlayer (BSD-3, prochazkaml)
// unpacks the module and renders it to mono PCM by counting each track's square
// wave.
//
// The related POLYTONE (.pol) format is intentionally NOT supported: the only
// public sample (PTPlayer's jakim.pol) is format version 2, while both this
// engine and upstream PTPlayer only parse version 1 ("\x08POLYTONE\x01"). With
// no playable v1 fixture available, .pol is documented as unsupported in
// data/misc/not_supported_extensions.txt instead.
//
// Note: the vendored engine keeps a single global song state (one song at a
// time), which is exactly how the host drives us.

namespace {

constexpr uint8_t MONO_MAGIC[9] = {0x08, 'M', 'O', 'N', 'O', 'T', 'O', 'N', 'E'};

bool hasMagic(const uint8_t* data, size_t len)
{
    return len >= sizeof(MONO_MAGIC) &&
           memcmp(data, MONO_MAGIC, sizeof(MONO_MAGIC)) == 0;
}

} // namespace

class MonotonePlayer : public ChipPlayer
{
public:
    explicit MonotonePlayer(std::vector<uint8_t> fileData,
                            const std::string& fileName)
        : data(std::move(fileData)), buffer(std::make_unique<buffer_t>())
    {
        if (!hasMagic(data.data(), data.size())) {
            throw player_exception("Not a Monotone file");
        }
        if (PTPlayer_UnpackFile(data.data(), buffer.get()) != 0) {
            throw player_exception("Could not load Monotone: " + fileName);
        }

        std::string title = utils::path_basename(fileName);
        setMeta("title", title, "channels", buffer->channels, "songs", 1,
                "startSong", 0, "format", "Monotone");
    }

    int getHZ() override { return 44100; }

    int getSamples(int16_t* target, int noSamples) override
    {
        if (ended) { return -1; }

        // PTPlayer renders mono; the host wants interleaved stereo. Render
        // `frames` mono int16s into the front of the buffer, then fan out.
        int frames = noSamples / 2;
        PTPlayer_PlayInt16(target, frames, 44100);

        // Detect end-of-song: the order counter advances through the order
        // table and, when the tune loops (natural wrap or a Bxx jump back),
        // resets to a lower index. Treat the first such backward step as the
        // end so the tune plays through once instead of looping forever.
        songstatus_t* st = PTPlayer_GetStatus();
        if (st->order < prevOrder) { ended = true; }
        prevOrder = st->order;

        // Safety cap for tunes that never wrap (e.g. a single-order loop).
        playedFrames += frames;
        if (playedFrames > kMaxFrames) { ended = true; }

        for (int j = frames - 1; j >= 0; j--) {
            int16_t s = target[j];
            target[j * 2] = s;
            target[j * 2 + 1] = s;
        }
        return frames * 2;
    }

private:
    static constexpr int64_t kMaxFrames = 44100LL * 600; // 10 minutes

    std::vector<uint8_t> data;
    std::unique_ptr<buffer_t> buffer;
    int prevOrder = 0;
    int64_t playedFrames = 0;
    bool ended = false;
};

static const std::set<std::string> supported_ext{"mon"};

bool MonotonePlugin::canHandle(const std::string& name)
{
    auto lowerName = utils::toLower(name);
    bool nameMatches =
        supported_ext.count(utils::path_extension(lowerName)) > 0 ||
        supported_ext.count(utils::path_prefix(lowerName)) > 0;
    if (!nameMatches) {
        return false;
    }
    // .mon is shared with UADE's Maniacs of Noise player; confirm the Monotone
    // content magic so we only claim genuine Monotone files.
    utils::File f{name};
    if (!f.exists()) {
        return false;
    }
    uint8_t magic[sizeof(MONO_MAGIC)];
    auto n = f.read(magic, sizeof(magic));
    return hasMagic(magic, n);
}

std::set<std::string> MonotonePlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* MonotonePlugin::fromFile(const std::string& fileName)
{
    return new MonotonePlayer{utils::File(fileName).readAll(), fileName};
}

} // namespace musix

extern "C" void monotoneplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::MonotonePlugin>();
    });
}
