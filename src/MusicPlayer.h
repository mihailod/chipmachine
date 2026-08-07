#pragma once

#include "SongInfo.h"

#include <musicplayer/src/chipplayer.h>

#include <coreutils/fifo.h>

#include <atomic>
#include <cstdint>
#include <deque>
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
    void stop()
    {
        player = nullptr;
        tracker_view = false;
    }
    [[nodiscard]] uint32_t getPosition() const { return play_pos / 44100; };
    [[nodiscard]] uint32_t getLength() const { return length; }
    // True once the first decoded samples have reached the audio callback, i.e.
    // real audio is flowing. For a progressively-streamed track this flips only
    // when ffmpeg's prebuffer has produced enough PCM, so it is the precise
    // "buffering finished" signal (play_pos is reset to 0 on each new song).
    // Sample-accurate, unlike getPosition() which is integer seconds.
    [[nodiscard]] bool hasAudioStarted() const { return play_pos > 0; }

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

    // --- Tracker pattern view -----------------------------------------------
    // True when the current format feeds pattern rows (see musix::TrackerRow).
    [[nodiscard]] bool hasTrackerView() const { return tracker_view; }

    // Hands over every row whose start has actually reached the speakers since
    // the last call, oldest first, and sets `fraction` to how far playback is
    // between the newest of those and the row after it (0..1) -- the decoder
    // runs ahead of the audio device, so the next row's timestamp is normally
    // already known and the display can interpolate instead of stepping.
    //
    // `upcoming` is filled with the rows that have been decoded but not yet
    // heard, nearest first, and left in the queue -- that lookahead is what lets
    // the display show notes scrolling in before they sound. It is a snapshot,
    // so it is overwritten (not appended to) on every call.
    //
    // Returns false if there is no tracker view. Safe to call from the render
    // thread; it only touches the row queue and atomics, never the player.
    bool takeTrackerRows(std::vector<musix::TrackerRow>& out,
                         std::vector<musix::TrackerRow>& upcoming,
                         float& fraction);

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

    // Drains the rows the player pushed during the getSamples() call that
    // started at frame `chunkStart` and queues them at absolute positions.
    void collectTrackerRows(uint64_t chunkStart);
    // Forgets everything queued and re-bases the generator position (song
    // change, seek -- anything that clears the audio fifo).
    void resetTrackerRows(uint64_t position);

    // How many not-yet-heard rows takeTrackerRows() reports. More than fills a
    // screen below the play line at any sane row height.
    static constexpr size_t kTrackerLookahead = 64;

    struct PendingRow
    {
        uint64_t pos; // absolute frame at which this row starts sounding
        musix::TrackerRow row;
    };

    std::mutex tracker_mutex;
    std::deque<PendingRow> tracker_rows;
    std::vector<musix::TrackerRow> tracker_scratch;
    // Frames handed to the fifo so far -- runs ahead of play_pos by whatever is
    // buffered, which is exactly the offset the row timestamps correct for.
    uint64_t gen_pos = 0;
    // The row currently sounding, and where it started.
    uint64_t tracker_current_pos = 0;
    bool tracker_has_current = false;
    std::atomic<bool> tracker_view{ false };

    utils::AudioFifo<int16_t> fifo;
    SongInfo playing_info;
    // Fifo fifo;
    std::function<void(int16_t*, int)> audio_callback;
    mutable std::mutex audio_cb_mutex;

    std::atomic<bool> paused{ false };

    std::shared_ptr<musix::ChipPlayer> player;
    std::string message;
    std::string sub_title;
    // Playback position in frames, interpolated between audio callbacks.
    //
    // play_pos only moves inside the CoreAudio callback, once per buffer --
    // 2048 frames, ~46ms. A pattern row at a normal tempo lasts ~120ms, so read
    // raw it updates barely twice per row and the tracker display lurches a
    // third of a row at a time instead of scrolling. Filling the gap from the
    // wall clock costs nothing and is exact between buffers; the clamp to one
    // buffer means a stalled or paused audio thread can never run it forward.
    [[nodiscard]] uint64_t interpolatedPos() const;

    std::atomic<int> play_pos{ 0 };
    std::atomic<int> play_chunk{ 0 };
    std::atomic<uint64_t> play_pos_ms{ 0 };
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
