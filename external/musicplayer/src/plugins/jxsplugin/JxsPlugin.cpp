#include "JxsPlugin.h"
#include "../../chipplayer.h"

#include <coreutils/file.h>
#include <coreutils/log.h>
#include <coreutils/utils.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

extern "C" {
#include "jaytrax.h"
#include "jxs.h"
}

// JayTrax (.jxs) player.
//
// Thin in-process wrapper around the public C port of Reinier "Rhino" van
// Vliet's own JayTrax/CrossX replayer (vendored at repo-root jaytrax/, ported
// by pachuco from https://bitbucket.org/rhinoid/crossx). The replayer rebuilds
// the whole song from a memory buffer (jxsfile_readSongMem) and renders int16
// stereo at an arbitrary frequency (jaytrax_renderChunk), so there is no file or
// hardware emulation to manage here -- we just feed it bytes and pull samples.
//
// JayTrax is a software synthesizer: each instrument is either a sample or a set
// of synth waveforms driven by AM/FM/pan/arpeggio modulators, mixed across up to
// six stereo channels with a stereo echo. A module may hold several subsongs.
//
// The format has no string magic; songs begin with a 16-bit "mugiversion" tag.
// The replayer accepts 3456 and 3457 (3458 is reserved/unimplemented upstream),
// so canHandle() checks for one of those followed by the 16-bit zero pad.

namespace musix {

namespace {

constexpr int kRate = 44100;

// True if the first bytes look like a loadable JayTrax header: mugiversion in
// {3456, 3457} at offset 0 (little-endian), then the 16-bit PAD00 == 0.
bool looksLikeJxs(const uint8_t* p, size_t n)
{
    if (n < 4) {
        return false;
    }
    int version = static_cast<uint16_t>(p[0] | (p[1] << 8));
    int pad = static_cast<uint16_t>(p[2] | (p[3] << 8));
    return (version == 3456 || version == 3457) && pad == 0;
}

} // namespace

class JxsPlayer : public ChipPlayer
{
public:
    JxsPlayer(const std::vector<uint8_t>& data, const std::string& fileName)
    {
        int err = jxsfile_readSongMem(data.data(), data.size(), &song_);
        if (err != 0 || song_ == nullptr) {
            throw player_exception("Could not load JayTrax song: " + fileName);
        }
        player_ = jaytrax_init();
        if (player_ == nullptr) {
            jxsfile_freeSong(song_);
            song_ = nullptr;
            throw player_exception("Could not init JayTrax player");
        }
        jaytrax_loadSong(player_, song_);

        int songCount = song_->nrofsongs > 0 ? song_->nrofsongs : 1;

        // getLength() walks the song to its first loop point (mutating player
        // state), so measure subsong 0 first, then reset to the start for
        // playback. -1 means "longer than the 30-minute cap" -> leave unset.
        int32_t lenSamples = jaytrax_getLength(player_, 0, 1, kRate);
        jaytrax_changeSubsong(player_, 0);

        if (lenSamples > 0) {
            setMeta("length", static_cast<uint32_t>(lenSamples / kRate));
        }
        setMeta("title", utils::path_basename(fileName), "songs", songCount,
                "startSong", 0, "format", "JayTrax");
    }

    ~JxsPlayer() override
    {
        if (player_ != nullptr) {
            jaytrax_free(player_);
        }
        if (song_ != nullptr) {
            jxsfile_freeSong(song_);
        }
    }

    int getHZ() override { return kRate; }

    int getSamples(int16_t* target, int noSamples) override
    {
        // renderChunk wants a frame (stereo pair) count and always fills it.
        int frames = noSamples / 2;
        jaytrax_renderChunk(player_, target, frames, kRate);
        return frames * 2;
    }

    bool seekTo(int song, int /*seconds*/) override
    {
        if (song >= 0 && song < (song_->nrofsongs > 0 ? song_->nrofsongs : 1)) {
            jaytrax_changeSubsong(player_, song);
            return true;
        }
        return false;
    }

private:
    JT1Song* song_ = nullptr;
    JT1Player* player_ = nullptr;
};

static const std::set<std::string> supported_ext{"jxs"};

bool JxsPlugin::canHandle(const std::string& name)
{
    auto lower = utils::toLower(name);
    bool extOk = false;
    for (auto const& e : supported_ext) {
        if (lower.size() > e.size() + 1 &&
            lower.compare(lower.size() - e.size() - 1, e.size() + 1,
                          "." + e) == 0) {
            extOk = true;
            break;
        }
    }
    if (!extOk) {
        return false;
    }
    // Confirm with the header so we don't claim a foreign .jxs (e.g. JPEG XS).
    FILE* fp = fopen(name.c_str(), "rb");
    if (fp == nullptr) {
        return false;
    }
    uint8_t hdr[4];
    size_t n = fread(hdr, 1, sizeof(hdr), fp);
    fclose(fp);
    return looksLikeJxs(hdr, n);
}

std::set<std::string> JxsPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* JxsPlugin::fromFile(const std::string& fileName)
{
    auto data = utils::File(fileName).readAll();
    return new JxsPlayer{data, fileName};
}

} // namespace musix

extern "C" void jxsplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::JxsPlugin>();
    });
}
