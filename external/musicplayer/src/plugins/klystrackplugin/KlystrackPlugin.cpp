#include "KlystrackPlugin.h"
#include "../../chipplayer.h"
#include <coreutils/utils.h>
#include <coreutils/log.h>

extern "C" {
#include "ksnd.h"
}

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace musix {

// klystrack song files begin with the "cyd!song" signature (MUS_SONG_SIG),
// followed by a one-byte format version. Anything else on a .kt extension is not
// a klystrack tune and is declined so the format Skips rather than mis-plays.
static const char KLYSTRACK_SIG[8] = {'c', 'y', 'd', '!', 's', 'o', 'n', 'g'};

class KlystrackPlayer : public ChipPlayer {
public:
    explicit KlystrackPlayer(const std::string& fileName) {
        FILE* fp = fopen(fileName.c_str(), "rb");
        if (fp == nullptr) {
            throw player_exception("Could not open klystrack song: " + fileName);
        }
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (size <= 0) {
            fclose(fp);
            throw player_exception("Empty klystrack song: " + fileName);
        }
        data.resize((size_t)size);
        size_t got = fread(data.data(), 1, (size_t)size, fp);
        fclose(fp);
        if (got != (size_t)size) {
            throw player_exception("Short read on klystrack song: " + fileName);
        }

        // libksnd renders at whatever sample rate we request; the app's audio
        // pipeline is fixed at 44100 Hz interleaved stereo, so ask for that and
        // feed the output straight through. Unregistered = no SDL audio thread,
        // we pull samples synchronously via KSND_FillBuffer().
        player = KSND_CreatePlayerUnregistered(44100);
        if (player == nullptr) {
            throw player_exception("Could not create klystrack player");
        }

        song = KSND_LoadSongFromMemory(player, data.data(), (int)data.size());
        if (song == nullptr) {
            KSND_FreePlayer(player);
            player = nullptr;
            throw player_exception("Could not load klystrack song: " + fileName);
        }

        // The .kt format has no repeat, so play once and stop at the end; we
        // signal end-of-song from getSamples() using the computed duration.
        int length_rows = KSND_GetSongLength(song);
        int length_ms = KSND_GetPlayTime(song, length_rows);
        total_frames = (length_ms > 0)
                           ? (int64_t)length_ms * 44100 / 1000
                           : 0;

        KSongInfo info;
        KSND_GetSongInfo(song, &info);
        std::string title = (info.song_title != nullptr) ? info.song_title : "";

        KSND_SetVolume(player, 128);
        KSND_SetLooping(player, 1); // MUS_NO_REPEAT: fall silent at song end
        KSND_PlaySong(player, song, 0);

        int length_seconds = (int)(total_frames / 44100);
        if (!title.empty()) {
            setMeta("title", title, "length", length_seconds, "format",
                    "Klystrack");
        } else {
            setMeta("length", length_seconds, "format", "Klystrack");
        }
    }

    ~KlystrackPlayer() override {
        if (song != nullptr) {
            KSND_FreeSong(song);
        }
        if (player != nullptr) {
            KSND_FreePlayer(player);
        }
    }

    int getHZ() override { return 44100; }

    int getSamples(int16_t* target, int noSamples) override {
        // Length-based end: KSND_FillBuffer always fills the whole buffer (with
        // silence past the end), so short reads can't signal completion. Once we
        // have output the song's full duration, return -1 so the GUI advances
        // instead of playing endless silence.
        if (total_frames > 0 && frames_played >= total_frames) {
            return -1;
        }

        // The host counts in interleaved int16 values; KSND_FillBuffer takes a
        // byte length and returns the number of stereo frames written.
        int frames = KSND_FillBuffer(player, target, noSamples * (int)sizeof(int16_t));
        frames_played += frames;
        return frames * 2;
    }

    bool seekTo(int /*song*/, int seconds) override {
        if (seconds < 0) {
            return false;
        }
        // KSND has no seek-by-time API, so restart and re-run NO_REPEAT playback
        // from the row nearest the requested time.
        int length_rows = KSND_GetSongLength(song);
        int length_ms = KSND_GetPlayTime(song, length_rows);
        int start_row = 0;
        if (length_ms > 0 && length_rows > 0) {
            start_row = (int)((int64_t)seconds * 1000 * length_rows / length_ms);
            if (start_row >= length_rows) {
                start_row = length_rows - 1;
            }
        }
        KSND_PlaySong(player, song, start_row);
        frames_played = (int64_t)seconds * 44100;
        return true;
    }

private:
    std::vector<uint8_t> data;
    KPlayer* player = nullptr;
    KSong* song = nullptr;
    int64_t total_frames = 0;
    int64_t frames_played = 0;
};

bool KlystrackPlugin::canHandle(const std::string& name) {
    if (utils::path_extension(name) != "kt") {
        return false;
    }
    FILE* fp = fopen(name.c_str(), "rb");
    if (fp == nullptr) {
        return false;
    }
    char magic[sizeof(KLYSTRACK_SIG)];
    size_t n = fread(magic, 1, sizeof(magic), fp);
    fclose(fp);
    return n == sizeof(magic) &&
           memcmp(magic, KLYSTRACK_SIG, sizeof(KLYSTRACK_SIG)) == 0;
}

std::set<std::string> KlystrackPlugin::getSupportedExtensions() const {
    return {"kt"};
}

ChipPlayer* KlystrackPlugin::fromFile(const std::string& name) {
    return new KlystrackPlayer{name};
}

} // namespace musix

extern "C" void klystrackplugin_register() {
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::KlystrackPlugin>();
    });
}
