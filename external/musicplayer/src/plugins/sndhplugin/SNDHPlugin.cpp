#include "SNDHPlugin.h"
#include "../../chipplayer.h"

#include "atariaudio/SndhFile.h"

#include <coreutils/file.h>
#include <coreutils/log.h>
#include <coreutils/utils.h>

#include <cstdint>
#include <cstring>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace musix {

namespace {

constexpr int kRate = 44100;

// AtariAudio drives Musashi, whose CPU state is a file-scope global, and
// AtariMachine.cpp routes its bus callbacks through a single `gCurrentMachine`
// pointer. Two live SndhFile instances would therefore trample each other's
// registers. chipmachine plays one tune at a time, but construction happens on
// the main thread while rendering happens on the audio thread, so serialise both
// against this. Held only for the duration of a call, never across one.
std::mutex& engineMutex()
{
    static std::mutex m;
    return m;
}

// Cheap, authoritative SNDH signature test on an UNPACKED buffer: a bra to the
// init entry point, then "SNDH" at offset 12. Same gate SndhFile::Load applies
// after depacking (see atariaudio/SndhFile.cpp).
bool hasSndhMagic(const uint8_t* d, size_t len)
{
    return len > 16 && d[0] == 0x60 && memcmp(d + 12, "SNDH", 4) == 0;
}

} // namespace

class SNDHPlayer : public ChipPlayer
{
public:
    SNDHPlayer(std::vector<uint8_t> const& data, std::string const& fileName)
    {
        std::lock_guard<std::mutex> lock(engineMutex());

        if (!sndh.Load(data.data(), static_cast<int>(data.size()), kRate)) {
            throw player_exception("SNDH: not an SNDH file");
        }

        subsongCount = sndh.GetSubsongCount();
        if (subsongCount < 1) { subsongCount = 1; }

        // AtariAudio numbers subsongs from 1; the host uses 0-based indices
        // everywhere (see MusicPlayerList's starttune and MusicPlayer::seek).
        int defaultSubsong = sndh.GetDefaultSubsong();
        if (defaultSubsong < 1 || defaultSubsong > subsongCount) {
            defaultSubsong = 1;
        }

        if (!initSubsong(defaultSubsong)) {
            throw player_exception("SNDH: subsong init failed");
        }

        SndhFile::SubSongInfo info{};
        sndh.GetSubsongInfo(defaultSubsong, info);

        // Every string in SubSongInfo points into the depacked file and may be
        // null -- SndhFile only fills them if the matching tag was present.
        std::string title = info.musicName != nullptr
                                ? std::string(info.musicName)
                                : utils::path_basename(fileName);
        std::string composer =
            info.musicAuthor != nullptr ? std::string(info.musicAuthor) : "";

        setMeta("title", title, "composer", composer, "format",
                "SNDH (Atari ST)", "songs", subsongCount, "startSong",
                defaultSubsong - 1, "length", trackLengthSeconds());
    }

    int getHZ() override { return kRate; }

    int getSamples(int16_t* target, int noSamples) override
    {
        std::lock_guard<std::mutex> lock(engineMutex());

        if (ended) { return -1; }

        // The host buffer is interleaved stereo; AtariAudio renders mono.
        int frames = noSamples / 2;
        if (frames <= 0) { return 0; }

        if (static_cast<int>(mono.size()) < frames) { mono.resize(frames); }

        int loops = sndh.AudioRender(mono.data(), frames);

        for (int i = frames - 1; i >= 0; i--) {
            target[i * 2] = mono[i];
            target[i * 2 + 1] = mono[i];
        }

        // AudioRender counts how many times playback passed the subsong's
        // declared length. That counter is only meaningful when a length was
        // actually declared: with no TIME/FRMS tag SndhFile leaves the frame
        // count at 0, so its `m_frame >= m_frameCount` test is true from the
        // first tick and `loops` climbs every frame. Tunes like that get no
        // end-of-song signal from us at all -- the host's own song-length and
        // silence detection ends them, same as it does for every other plugin
        // that reports length 0.
        if (hasKnownLength && loops >= 1) { ended = true; }

        return frames * 2;
    }

    bool seekTo(int song, int seconds) override
    {
        // AtariAudio has no seek: the ST replay routine is driven tick by tick
        // from its own init, with no way to fast-forward short of rendering and
        // discarding. Subsong changes restart the machine, which IS supported;
        // a seek within a subsong is declined so the host keeps its position.
        if (song < 0) { return false; }
        if (seconds > 0) { return false; }

        std::lock_guard<std::mutex> lock(engineMutex());

        if (song >= subsongCount) { return false; }
        if (!initSubsong(song + 1)) { return false; }

        SndhFile::SubSongInfo info{};
        sndh.GetSubsongInfo(song + 1, info);
        setMeta("song", song, "length", trackLengthSeconds());
        return true;
    }

private:
    // Caller must hold engineMutex(). Resets the whole Atari machine and runs
    // the tune's init entry point.
    bool initSubsong(int subsong)
    {
        if (!sndh.InitSubSong(subsong)) { return false; }
        SndhFile::SubSongInfo info{};
        sndh.GetSubsongInfo(subsong, info);
        hasKnownLength = info.playerTickCount > 0 && info.playerTickRate > 0;
        currentLength = hasKnownLength
                            ? info.playerTickCount / info.playerTickRate
                            : 0;
        ended = false;
        return true;
    }

    uint32_t trackLengthSeconds() const
    {
        return static_cast<uint32_t>(currentLength);
    }

    SndhFile sndh;
    std::vector<int16_t> mono;
    int subsongCount{ 1 };
    int currentLength{ 0 };
    bool hasKnownLength{ false };
    bool ended{ false };
};

// ".snd" is shared with Westwood ADL tunes (AdPlug) and a pile of vgmstream
// containers, so it is content-gated below rather than claimed outright -- the
// same arrangement SC68Plugin had, and the reason for priority() 2.
static const std::set<std::string> supported_ext{ "sndh", "snd" };

bool SNDHPlugin::canHandle(const std::string& name)
{
    auto ext = utils::toLower(utils::path_extension(name));
    if (ext == "sndh") { return true; }
    if (ext != "snd") { return false; }

    // Only an UNPACKED SNDH is claimed here. Recognising an ICE!-packed one
    // would mean depacking the whole file just to answer canHandle, and the
    // exposure is 3 rows in the whole catalog (the other 100 ".snd" are AdLib
    // and belong to AdPlug). An ICE!-packed ".snd" therefore still reaches
    // libsc68 in the plus build and is simply unclaimed in mas.
    if (!utils::File::exists(name)) { return false; }
    try {
        utils::File f{ name };
        uint8_t header[17];
        auto size = f.read(header, sizeof(header));
        return hasSndhMagic(header, static_cast<size_t>(size));
    } catch (utils::io_exception const&) {
        return false;
    }
}

std::set<std::string> SNDHPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* SNDHPlugin::fromFile(const std::string& fileName)
{
    auto data = utils::File(fileName).readAll();
    if (data.empty()) { return nullptr; }
    try {
        return new SNDHPlayer{ data, fileName };
    } catch (player_exception const& e) {
        // Not ours after all (or a broken rip). Returning null lets
        // MusicPlayer::fromFile fall through to the next claimer, which in the
        // plus build is SC68Plugin.
        LOGD("SNDH: declining '{}': {}", fileName, e.what());
        return nullptr;
    }
}

} // namespace musix

extern "C" void sndhplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::SNDHPlugin>();
    });
}
