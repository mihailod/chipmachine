
#include "StSoundPlugin.h"
#include "../../chipplayer.h"
#include "StSoundLibrary/StSoundLibrary.h"

#include <coreutils/log.h>
#include <coreutils/utils.h>

#include <coreutils/file.h>

#include <set>
#include <stdexcept>
//#include <unordered_map>

namespace musix {

class StSoundPlayer : public ChipPlayer
{
public:
    explicit StSoundPlayer(std::vector<uint8_t> data) : ymMusic(ymMusicCreate())
    {
        // ymMusicLoadMemory fails (returns YMFALSE) on anything that isn't a
        // YM/MIX stream the bundled depacker+decoder can parse -- e.g. a raw
        // LHA-wrapped .ym whose inner member wasn't extracted first, or a
        // truncated download. On failure the ymMusicInfo_t strings are left
        // NULL, so constructing std::string from info.pSongName below would
        // strlen(NULL) and segfault. Bail out so the host (MusicPlayer::fromFile)
        // catches it and Skips the file gracefully.
        if (ymMusicLoadMemory(ymMusic, &data[0], data.size()) == YMFALSE) {
            throw std::runtime_error("StSound: not a valid YM file");
        }

        ymMusicInfo_t info;
        ymMusicGetInfo(ymMusic, &info);

        std::string name = info.pSongName != nullptr ? info.pSongName : "";
        std::string author = info.pSongAuthor != nullptr ? info.pSongAuthor : "";
        if (name == "Unknown") {
            name = "";
        }
        if (author == "Unknown") {
            author = "";
        }

        setMeta("title", name, "composer", author, "length",
                info.musicTimeInSec, "format", info.pSongType);
        LOGD("TYPE {} PLAYER {}", info.pSongType, info.pSongPlayer);
        // printf("Name.....: %s\n",info.pSongName);
        // printf("Author...: %s\n",info.pSongAuthor);
        // printf("Comment..: %s\n",info.pSongComment);
        // printf("Duration.:
        // %d:%02d\n",info.musicTimeInSec/60,info.musicTimeInSec%60);
        // printf("Driver...: %s\n", info.pSongPlayer);
        // ymMusicSetLoopMode(pMusic,YMTRUE);
        ymMusicPlay(ymMusic);

        // setMetaData("length", ModPlug_GetLength(mod) / 1000);
        // ymMusicStop(ymMusic);
    }
    ~StSoundPlayer() override
    {
        if (ymMusic != nullptr) {
            ymMusicDestroy(ymMusic);
        }
        ymMusic = nullptr;
    }

    int getSamples(int16_t* target, int noSamples) override
    {

        noSamples /= 2;
        ymMusicCompute(ymMusic, target, noSamples);
        // Mono to stereo
        for (int i = noSamples - 1; i >= 0; i--) {
            target[i * 2] = target[i];
            target[i * 2 + 1] = target[i];
        }
        return noSamples * 2;
    }

    bool seekTo(int /*song*/, int seconds) override
    {
        // if(mod)
        //	ModPlug_Seek(mod, seconds * 1000);
        ymMusicSeek(ymMusic, seconds * 1000);
        return true;
    }

private:
    YMMUSIC* ymMusic;
};

static const std::set<std::string> supported_ext{"ym", "mix"};

bool StSoundPlugin::canHandle(const std::string& name)
{
    return supported_ext.count(utils::path_extension(name)) > 0;
}

std::set<std::string> StSoundPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

ChipPlayer* StSoundPlugin::fromFile(const std::string& fileName)
{
    // return new StSoundPlayer{utils::read_file(fileName)};
    return new StSoundPlayer{utils::File(fileName).readAll()};
};

} // namespace musix
extern "C" void stsoundplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& config) {
        return std::make_shared<musix::StSoundPlugin>();
    });
}
