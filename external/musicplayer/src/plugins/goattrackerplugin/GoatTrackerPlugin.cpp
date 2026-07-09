#include "GoatTrackerPlugin.h"

#include "../../chipplayer.h"
#include <coreutils/utils.h>
#include <coreutils/file.h>

#include <array>
#include <cstring>
#include <vector>

extern "C" {
#include "gt2_engine.h"
}

namespace {

constexpr int SAMPLERATE = 44100;
// GoatTracker default: PAL, 1x speed multiplier -> 50 Hz play rate.
constexpr int FRAMERATE = 50;
constexpr int SAMPLES_PER_FRAME = SAMPLERATE / FRAMERATE; // 882
constexpr int MAX_SECONDS = 600;
constexpr int MIN_SECONDS = 2;

bool isGoatTracker(const uint8_t* d, size_t len)
{
    if (len < 4) { return false; }
    // Every GoatTracker song format: GTS! (v1), GTS2 (3-table), GTS3/GTS4/GTS5.
    return !memcmp(d, "GTS!", 4) || !memcmp(d, "GTS2", 4) || !memcmp(d, "GTS3", 4) ||
           !memcmp(d, "GTS4", 4) || !memcmp(d, "GTS5", 4);
}

} // namespace

namespace musix {

class GoatTrackerPlayer : public ChipPlayer
{
public:
    explicit GoatTrackerPlayer(const std::string& fileName)
    {
        subsongs = gt2_load(fileName.c_str());
        if (subsongs < 1) {
            throw player_exception("Not a supported GoatTracker .sng");
        }

        // reSID: sample rate, chip model (0 = 6581), PAL, no interpolation.
        sid_init(SAMPLERATE, sidmodel, ntsc, interpolate, 0, 0);

        startSubsong(0);

        std::string title = songname[0] ? std::string(songname) : utils::path_basename(fileName);
        setMeta("title", title, "composer", std::string(authorname), "copyright",
                std::string(copyrightname), "format", "GoatTracker (C64)", "songs",
                subsongs, "startSong", 0, "song", 0, "length", 0);
    }

    int getHZ() override { return SAMPLERATE; }

    int getSamples(int16_t* target, int noSamples) override
    {
        int wanted = noSamples / 2; // mono samples requested
        if (mono.size() < static_cast<size_t>(wanted)) { mono.resize(wanted); }

        int produced = 0;
        while (produced < wanted) {
            if (frameLeft == 0) {
                if (ended) { break; }
                playroutine();
                framesPlayed++;
                updateEnd();
                frameLeft = SAMPLES_PER_FRAME;
            }
            int n = wanted - produced;
            if (n > frameLeft) { n = frameLeft; }
            int got = sid_fillbuffer(mono.data() + produced, n);
            if (got <= 0) { break; }
            produced += got;
            frameLeft -= got;
        }

        if (produced <= 0) { return 0; } // song ended

        // reSID renders mono; fan out to interleaved stereo.
        for (int i = produced - 1; i >= 0; i--) {
            int16_t s = mono[i];
            target[i * 2] = s;
            target[i * 2 + 1] = s;
        }
        return produced * 2;
    }

    bool seekTo(int song, int /*seconds*/) override
    {
        if (song < 0 || song >= subsongs) { return false; }
        startSubsong(song);
        setMeta("song", song, "length", 0);
        return true;
    }

private:
    void startSubsong(int song)
    {
        current = song;
        initchannels();
        initsong(song, PLAY_BEGINNING);
        for (int c = 0; c < MAX_CHN; c++) { prevSongptr[c] = 0; looped[c] = false; }
        framesPlayed = 0;
        frameLeft = 0;
        ended = false;
    }

    // GoatTracker songs loop forever via the LOOPSONG order-list marker. End on
    // one full pass (every channel's order position has wrapped backwards at
    // least once), on an explicit stopsong (isplaying() false), or on a hard
    // time cap -- but never before a short floor so short looping intros aren't
    // cut off instantly.
    void updateEnd()
    {
        if (!isplaying()) { ended = true; return; }

        for (int c = 0; c < MAX_CHN; c++) {
            if (chn[c].songptr < prevSongptr[c]) { looped[c] = true; }
            prevSongptr[c] = chn[c].songptr;
        }

        if (framesPlayed < MIN_SECONDS * FRAMERATE) { return; }

        bool allLooped = looped[0] && looped[1] && looped[2];
        if (allLooped || framesPlayed >= MAX_SECONDS * FRAMERATE) { ended = true; }
    }

    int subsongs = 0;
    int current = 0;
    int framesPlayed = 0;
    int frameLeft = 0;
    bool ended = false;
    unsigned char prevSongptr[MAX_CHN] = {0, 0, 0};
    bool looped[MAX_CHN] = {false, false, false};
    std::vector<int16_t> mono;
};

static const std::set<std::string> supported_ext{"sng"};

bool GoatTrackerPlugin::canHandle(const std::string& name)
{
    if (utils::path_extension(utils::toLower(name)) != "sng") { return false; }
    utils::File f{name};
    if (!f.exists()) { return false; }
    auto data = f.readAll();
    return isGoatTracker(data.data(), data.size());
}

std::set<std::string> GoatTrackerPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* GoatTrackerPlugin::fromFile(const std::string& fileName)
{
    return new GoatTrackerPlayer{fileName};
}

} // namespace musix

extern "C" void goattrackerplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::GoatTrackerPlugin>();
    });
}
