#include "MusicPlayer.h"
#include "GZPlugin.h"
#include "modutils.h"

#include <archive/archive.h>
#include <audioplayer/audioplayer.h>
#include <coreutils/format.h>
#include <coreutils/utils.h>
#include <musicplayer/src/plugins/plugins.h>

#include <algorithm>
#include <set>

#include <variant>

namespace chipmachine {

MusicPlayer::MusicPlayer(std::shared_ptr<AudioPlayer> ap)
    : fifo(32786 * 4),
      stream_fifo(std::make_shared<utils::Fifo<uint8_t>>(32768 * 8)),
      audio_player(std::move(ap))
{
    audio_player->set_volume(80);
    volume = 0.8;

    musix::ChipPlugin::addPlugin(
        std::make_shared<GZPlugin>(musix::ChipPlugin::getPlugins()), true);

    audio_player->play([this](int16_t* ptr, int size) mutable {
        if (dont_play) {
            memset(ptr, 0, size * 2);
            return;
        }

        if (fifo.filled() >= size) {
            fifo.get(ptr, size);
            play_pos += size / 2;

            // CRITICAL LIFETIME GUARD:
            // Hold audio_cb_mutex while invoking so the std::function cannot be
            // reassigned (e.g. setAudioCallback(nullptr) from ~ChipMachine on the
            // main thread) underneath us. The lock is held only for the trivial
            // FFT feed; the main thread never holds it for long.
            if (!dont_play) {
                std::lock_guard<std::mutex> lock(audio_cb_mutex);
                if (audio_callback) {
                    audio_callback(ptr, size);
                }
            }
        } else {
            memset(ptr, 0, size * 2);
        }
    });
}

// Make sure the fifo is filled
void MusicPlayer::update()
{
    static std::vector<int16_t> temp_buf(fifo.size());

    if (!paused && player && !play_ended) {
        if(auto *s = std::get_if<std::string>(&player->meta("sub_title")))
            sub_title = *s;
        if(auto *u = std::get_if<uint32_t>(&player->meta("length")))
            length = *u;
        if(auto *s = std::get_if<std::string>(&player->meta("message")))
            message = *s;
        silent_frames = check_silence ? fifo.getSilence() : 0;

        while (!fifo.isQuitting()) {

            int space_left = fifo.left();

            if (space_left < 4096) break;

            int samples_generated =
                player->getSamples(&temp_buf[0], space_left - 1024);

            if (samples_generated <= 0) {
                play_ended = samples_generated < 0;
                break;
            }
            if (fadeout_pos != 0 && fadeout_pos >= play_pos) {
                fifo.setVolume((fadeout_pos - play_pos) / (float)fade_length);
            }

            fifo.put(&temp_buf[0], samples_generated);
            if (fifo.filled() >= fifo.size() / 2) {
                break;
            }
        }
    }
}

MusicPlayer::~MusicPlayer()
{
    // 1. Null out the callback under the mutex so fill_audio() becomes a no-op.
    //    This is safe to call before pausing — fill_audio checks callback != null.
    audio_player->play(nullptr);

    // 2. Pause the AudioQueue synchronously. After this returns, the audio thread
    //    is guaranteed not to be inside fill_audio(), so no FIFO access can race
    //    with the quit()/destructor below. Without this, AudioQueueDispose (called
    //    when audio_player destructs) would block waiting for fill_audio to finish,
    //    which in turn would be stuck waiting on fifo.m — deadlock.
    audio_player->pause();

    // 3. Quit both FIFOs to unblock any pending put() calls.
    stream_fifo->quit();
    fifo.quit();
}

void MusicPlayer::seek(int song, int seconds)
{
    if (!player) return;
    if (player->seekTo(song, seconds)) {
        if (seconds < 0)
            play_pos = 0;
        else
            play_pos = seconds * 44100;
        fifo.clear();
        // length = player->getMetaInt("length");
        updatePlayingInfo();
        currentTune = song;
    }
}

int MusicPlayer::getSilence() const
{
    return silent_frames;
}

// fadeOutPos music
void MusicPlayer::fadeOut(float secs)
{
    fade_length = secs * 44100;
    fadeout_pos = play_pos + fade_length;
}

void MusicPlayer::quit()
{
    stream_fifo->quit();
    fifo.quit();
}

void MusicPlayer::putStream(const uint8_t* ptr, int size)
{
    // LOGD("Writing %d bytes to stream", size);
    stream_fifo->put(ptr, size);
}

void MusicPlayer::setParameter(const std::string& what, int v)
{
    if (player) player->setParameter(what, v);
}

bool MusicPlayer::streamFile(const std::string& fileName)
{
    dont_play = true;
    silent_frames = 0;

    playing_info = SongInfo();
    std::string name = fileName;
    player = nullptr;

    utils::makeLower(name);
    check_silence = true;
    for (auto& plugin : musix::ChipPlugin::getPlugins()) {
        if (plugin->canHandle(name)) {
            //LOGD("Playing with %s\n", plugin->name());
            auto newPlayer = std::shared_ptr<musix::ChipPlayer>(
                plugin->fromStream(stream_fifo));
            if (newPlayer) player = newPlayer;
            check_silence = plugin->checkSilence();
            break;
        }
    }

    dont_play = false;
    play_ended = false;

    if (player) {

        clearStreamFifo();
        fifo.clear();
        fadeout_pos = 0;
        pause(false);
        play_pos = 0;
        message = "";
        length = 0;
        sub_title = "";
        currentTune = playing_info.starttune;
        return true;
    }
    return false;
}

bool MusicPlayer::playFile(const std::string& fileName)
{

    dont_play = true;
    silent_frames = 0;
    playing_info = SongInfo();
    std::string name = fileName;

    if (utils::endsWith(name, ".rar")) {
        try {
            auto* a = utils::Archive::open(name, "_files");
            for (const auto& s : *a) {
                a->extract(s);
                name = "_files/" + s;
                //LOGD("Extracted %s", name);
                break;
            }
        } catch (utils::archive_exception& ae) {
            player = nullptr;
            return false;
        }
    }

    player = nullptr;
    player = fromFile(name);

    dont_play = false;
    play_ended = false;

    if (player) {

        fifo.clear();
        fadeout_pos = 0;
        pause(false);
        play_pos = 0;
        updatePlayingInfo();
        currentTune = playing_info.starttune;
        return true;
    }
    return false;
}

void MusicPlayer::updatePlayingInfo()
{
    //printf("MusicPlayer: updatePlayingInfo started\n"); fflush(stdout);
    SongInfo info;

    // Safely get string meta
    info.title = std::get<std::string>(player->meta("title"));
    //printf("MusicPlayer: Got title '%s'\n", info.title.c_str()); fflush(stdout);

    info.composer = std::get<std::string>(player->meta("composer"));
    //printf("MusicPlayer: Got composer '%s'\n", info.composer.c_str()); fflush(stdout);

    // Safely get 'songs' (numtunes)
    auto songs_meta = player->meta("songs");
    if (auto* pVal = std::get_if<uint32_t>(&songs_meta)) {
        info.numtunes = *pVal;
        //printf("MusicPlayer: Got songs (uint32_t) %u\n", info.numtunes); fflush(stdout);
    } else {
        info.numtunes = 0;
        //printf("MusicPlayer: 'songs' not found or not a uint32_t\n"); fflush(stdout);
    }

    // Safely get 'startSong'
    auto startSong_meta = player->meta("startSong");
    int startTune = 0; // Default to 0
    if (auto* pInt = std::get_if<uint32_t>(&startSong_meta)) {
        startTune = static_cast<int>(*pInt);
        //printf("MusicPlayer: Got startSong (uint32_t) %d\n", startTune); fflush(stdout);
    } else {
         //printf("MusicPlayer: 'startSong' not found or not a uint32_t\n"); fflush(stdout);
    }
    info.starttune = startTune;

    if (info.starttune == -1) info.starttune = 0;

    // Safely get 'length'
    auto length_meta = player->meta("length");
    if (auto* pVal = std::get_if<uint32_t>(&length_meta)) {
        length = *pVal;
        auto length_val = length.load();
    //printf("MusicPlayer: Got length (uint32_t) %u\n", length_val); fflush(stdout);
    } else if (auto* pVal = std::get_if<double>(&length_meta)) {
        length = static_cast<uint32_t>(*pVal);
        //printf("MusicPlayer: Got length (double) %f\n", *pVal); fflush(stdout);
    }
    else {
        length = 0;
        //printf("MusicPlayer: 'length' not found or not a number\n"); fflush(stdout);
    }

    message = std::get<std::string>(player->meta("message"));
    //printf("MusicPlayer: Got message '%s'\n", message.c_str()); fflush(stdout);

    sub_title = std::get<std::string>(player->meta("sub_title"));
    //printf("MusicPlayer: Got sub_title '%s'\n", sub_title.c_str()); fflush(stdout);

    playing_info = info;
    //printf("MusicPlayer: updatePlayingInfo finished\n"); fflush(stdout);
}

void MusicPlayer::pause(bool do_pause)
{
    if (do_pause)
        audio_player->pause();
    else
        audio_player->resume();
    paused = do_pause;
}

std::string MusicPlayer::getMeta(const std::string& what)
{       
    if (what == "message") {
        return message;
    } else if (what == "sub_title") {
        return sub_title;
    }

    if (player) {
        auto val = player->meta(what);

        // Case 1: Variant actually holds a string
        if (auto str_ptr = std::get_if<std::string>(&val)) {
            return *str_ptr;
        }
        
        // Case 2: Variant holds a double (e.g., length)
        if (auto dbl_ptr = std::get_if<double>(&val)) {
            return std::to_string(*dbl_ptr);
        }

        // Case 3: Variant holds an unsigned int (e.g., bitrate, channels)
        if (auto uint_ptr = std::get_if<unsigned int>(&val)) {
            return std::to_string(*uint_ptr);
        }
    }

    return "";
}

void MusicPlayer::setVolume(float v)
{
    volume = utils::clamp(v);
    audio_player->set_volume(volume * 100);
}

float MusicPlayer::getVolume() const
{
    return volume;
}

std::vector<std::string> MusicPlayer::getSecondaryFiles(const std::string& name)
{
    // Delegate to the plugin that handles the file. Each plugin reports its own
    // companions (PSF "_lib" libraries, sample banks, voicesets, ...) with the
    // EXACT names its loader expects -- in particular the original case. We must
    // NOT lower-case them: Modland's FTP is case-sensitive, so a tag like
    // "ZZZ_JNA1.psf2lib" lowered to "zzz_jna1.psf2lib" 550s and the tune plays
    // silent (it only "worked" before for libs that happened to be lower-case or
    // were already in a local mirror on a case-insensitive filesystem).
    utils::File file{ name };
    if (file.exists()) {
        for (auto& plugin : musix::ChipPlugin::getPlugins()) {
            if (plugin->canHandle(name)) {
                return plugin->getSecondaryFiles(name);
            }
        }
    }
    return {};
}

// PRIVATE

std::shared_ptr<musix::ChipPlayer>
MusicPlayer::fromFile(const std::string& file_name)
{
    auto name = file_name;
    utils::makeLower(name);
    check_silence = true;
    //LOGD("Finding plugin for '%s' (%s)", file_name, name);
    for (auto& plugin : musix::ChipPlugin::getPlugins()) {
        if (plugin->canHandle(name)) {
            try {
                //LOGD("Playing with %s\n", plugin->name());
                auto player =
                    std::shared_ptr<musix::ChipPlayer>(plugin->fromFile(file_name));
                if (!player) continue;
                check_silence = plugin->checkSilence();
                return player;
            }
            catch (const std::exception& e) {
                fprintf(stderr, "MusicPlayer: Plugin '%s' failed to load '%s'. Error: %s\n", 
                        plugin->name().c_str(), file_name.c_str(), e.what());
                continue; 
            }
            catch (...) {
                fprintf(stderr, "MusicPlayer: Plugin '%s' encountered an unknown exception parsing '%s'.\n", 
                        plugin->name().c_str(), file_name.c_str());
                continue;
            }
        }
    }
    return nullptr;
}

} // namespace chipmachine

