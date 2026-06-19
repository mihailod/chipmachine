#pragma once

#include "SongInfo.h"

#include <coreutils/fifo.h>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace musix {
class ChipPlugin;
class ChipPlayer;
} // namespace musix

class AudioPlayer;

namespace chipmachine {

class MusicPlayer
{
public:
    explicit MusicPlayer(std::shared_ptr<AudioPlayer> ap);
    MusicPlayer(MusicPlayer const& other) = delete;
    ~MusicPlayer();
    bool playFile(const std::string& fileName);
    bool streamFile(const std::string& fileName);
    // Stream a radio URL by letting ffmpeg fetch and decode it directly. This
    // handles Shoutcast/ICY, redirects and any codec (mp3/ogg/aac) far more
    // robustly than the curl->fifo->mpg123 path (e.g. bare ICY 200 mounts).
    bool streamUrl(const std::string& url);
    [[nodiscard]] bool playing() const
    {
        return !play_ended && player != nullptr;
    }
    void stop() { player = nullptr; }
    [[nodiscard]] uint32_t getPosition() const { return play_pos / 44100; };
    [[nodiscard]] uint32_t getLength() const { return length; }

    void putStream(const uint8_t* ptr, int size);
    void clearStreamFifo() { stream_fifo->clear(); }
    std::shared_ptr<utils::Fifo<uint8_t>> getStreamFifo() { return stream_fifo; }

    // Tells the current streaming player no more bytes are coming (download
    // finished), so it can flush and end. See streamFile().
    void endStream();
    // Unblocks any producer blocked in stream_fifo->put() (e.g. the web thread
    // feeding a now-cancelled stream) by quitting the current fifo. streamFile()
    // allocates a fresh fifo for the next session.
    void abortStream();
    // The active player, if any -- used by the streaming feeder to target the
    // exact session it belongs to (a weak_ptr so a song switch invalidates it).
    std::weak_ptr<musix::ChipPlayer> getPlayer() { return player; }

    void setParameter(const std::string& what, int v);

    // Asks the plugin if the given file requires secondary files.
    // Can be called several times, normally first with non-existing
    // file, and later with the loaded file
    std::vector<std::string> getSecondaryFiles(const std::string& name);

    void pause(bool dopause = true);

    [[nodiscard]] bool isPaused() const { return paused; }

    void seek(int song, int seconds = -1);

    [[nodiscard]] int getTune() const { return currentTune; }

    [[nodiscard]] SongInfo getPlayingInfo() const { return playing_info; }

    std::string getMeta(const std::string& what);

    // Returns silence (from now) in seconds
    [[nodiscard]] int getSilence() const;

    void setVolume(float v);
    [[nodiscard]] float getVolume() const;

    // Fadeout music
    void fadeOut(float secs);
    [[nodiscard]] float getFadeVolume() const { return fifo.getVolume(); }

    void quit();
    void update();

    void setAudioCallback(const std::function<void(int16_t*, int)>& cb)
    {
        // Guarded because the CoreAudio thread reads/invokes audio_callback
        // concurrently (see the play() lambda in the constructor). ~ChipMachine
        // calls this with nullptr from the main thread during shutdown while
        // audio is still live — without the lock that is a data race on the
        // std::function object itself (UB), not just on what it points at.
        std::lock_guard<std::mutex> lock(audio_cb_mutex);
        audio_callback = cb;
    }

private:
    std::shared_ptr<musix::ChipPlayer> fromFile(const std::string& fileName);
    void updatePlayingInfo();

    utils::AudioFifo<int16_t> fifo;
    SongInfo playing_info;
    // Fifo fifo;
    std::function<void(int16_t*, int)> audio_callback;
    mutable std::mutex audio_cb_mutex;

    std::atomic<bool> paused{ false };

    std::shared_ptr<musix::ChipPlayer> player;
    std::string message;
    std::string sub_title;
    std::atomic<int> play_pos{ 0 };
    std::atomic<int> length{ 0 };
    int fade_length = 0;
    int fadeout_pos = 0;
    int silent_frames = 0;
    int currentTune = 0;
    std::atomic<float> volume = 1.0F;

    // Feed silence to audio player
    std::atomic<bool> dont_play{ false };
    std::atomic<bool> play_ended{ false };
    bool check_silence = true;

    std::shared_ptr<utils::Fifo<uint8_t>> stream_fifo;

    std::shared_ptr<AudioPlayer> audio_player;
};
} // namespace chipmachine
