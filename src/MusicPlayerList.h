#pragma once

#include "CueSheet.h"
#include "MusicDatabase.h"
#include "MusicPlayer.h"
#include "RemoteLoader.h"
#include "SongInfo.h"

#include <coreutils/thread.h>
#include <cstdint>
#include <deque>

struct log_guard
{
    log_guard(std::mutex& m, const char* f, int l) : m(m)
    {
        if (!m.try_lock()) {
            logging::log2(logging::xbasename(f), l, logging::Debug,
                          "Waiting for lock");
            m.lock();
        }
    }
    ~log_guard() { m.unlock(); }
    std::mutex& m;
};

#define LOCK_GUARD(x) std::lock_guard guard(x)
//#define LOCK_GUARD(x) log_guard guard(x, __FILE__, __LINE__)

#define SET_STATE(x) (/*LOGD("STATE: " #x),*/ state = x)

namespace chipmachine {

class ChipMachine;

class MusicPlayerList : public std::enable_shared_from_this<MusicPlayerList>
{
public:
    // (song, audio) extension sets the ZIP track picker accepts, derived from
    // the registered plugins so they can't drift from what the app can actually
    // decode as a loose file. "song" (chip/console/tracker) is preferred; "audio"
    // (ffmpeg renderings) is the fallback. Requires createPlugins() to have run.
    static const std::pair<std::set<std::string>, std::set<std::string>>&
    archiveExtensions();

    enum State
    {
        Stopped,
        Error,
        Waiting,
        Loading,
        Started,
        Playstarted,
        Playing,
        Fading,
        Playnow,
        Playmulti
    };

    MusicPlayerList(MusicDatabase& mdb, RemoteLoader& rl, std::shared_ptr<AudioPlayer> ap);

    ~MusicPlayerList();

    void addSong(const SongInfo& si, bool shuffle = false);
    void playSong(const SongInfo& si);
    void clearSongs();
    void nextSong();

    SongInfo getInfo(int index = 0) const;
    SongInfo getDBInfo() const;

    // Single source of truth for the stream-vs-download decision. Returns true if
    // `info` will be played by progressive streaming (curl->fifo->ffmpeg, or the
    // YouTube/radio direct-URL path) -- i.e. it plays after a short prebuffer --
    // and false if it must be downloaded whole before playback. playCurrent()
    // routes on exactly this, and the UI reads it to show "BUFFERING..." vs
    // "LOADING...", so the message always matches the path actually taken.
    static bool willStream(const SongInfo& info);
    // The set of file extensions the ffmpeg progressive-stream path accepts.
    // Shared between willStream() and playCurrent() so the two never drift.
    static bool isStreamableExt(const std::string& ext);
    // Streamable containers whose ffmpeg demuxer needs a seekable input, so they
    // take the direct-URL (HTTP Range) path instead of the one-way pipe.
    static bool needsSeekableInput(const std::string& ext);
    int getLength() const;
    int getPosition() const;
    int listSize() const;

    bool isPlaying() const { return playing; }
    // True once the current song's first samples have reached the DAC -- the
    // precise "buffering finished" signal for progressive streams (see
    // MusicPlayer::hasAudioStarted). Used by the UI to clear "BUFFERING...".
    bool hasAudioStarted() const { return mp.hasAudioStarted(); }

    int getTune() const { return currentTune; }

    void pause(bool dopause = true)
    {
        LOCK_GUARD(plMutex);
        mp.pause(dopause);
    }

    bool isPaused() const { return paused; }

    void seek(int song, int seconds = -1);

    int getBitrate() const { return bitRate; }

    std::string getMeta(const std::string& what)
    {
        if (what == "sub_title") {
            std::string sub = std::string(subtitlePtr);
            return sub;
        }
        // "message" is snapshotted by the player thread in update() so the
        // render thread never has to reach into the decoder (mp) here. The
        // decode now runs outside plMutex, so mp.message can be written
        // concurrently -- reading it directly would be a data race.
        if (what == "message") {
            LOCK_GUARD(plMutex);
            return message;
        }
        LOCK_GUARD(plMutex);
        LOGD("META %s", what);
        return mp.getMeta(what);
    }

    State getState()
    {
        // LOCK_GUARD(plMutex);
        State rc = state;
        if (rc == Playstarted) {
            SET_STATE(Playing);
        }
        return rc;
    }

    bool hasError() const { return !errors.empty(); }

    std::string getError()
    {
        LOCK_GUARD(plMutex);
        auto e = errors.front();
        errors.pop_front();
        return e;
    }

    void setVolume(float volume)
    {
        onThisThread([=] { mp.setVolume(volume); });
    }

    float getVolume()
    {
        LOCK_GUARD(plMutex);
        return mp.getVolume();
    }

    void stop()
    {
        onThisThread([=] {
            SET_STATE(Stopped);
            mp.stop();
        });
    }

    void setAudioCallback(const std::function<void(int16_t*, int)>& cb)
    {
        mp.setAudioCallback(cb);
    }

    bool wasFromQueue() const { return playedNext; }

    const std::vector<utils::File>& getSongFiles() const { return songFiles; }

    bool playlistUpdated() { return playList.wasUpdated(); }

    void wait();

private:
    void onThisThread(const std::function<void()>& f)
    {
        LOCK_GUARD(plMutex);
        funcs.push_back(f);
    }

    std::vector<std::function<void()>> funcs;

    void cancelStreaming();
    // Progressively streams a finite remote ffmpeg-decodable file (curl -> fifo
    // -> ffmpeg stdin) so it starts after a short prebuffer instead of a full
    // download. Returns false if no streaming player could be created.
    bool streamRemoteFile(const std::string& path);
    bool handlePlaylist(const std::string& fileName);
    void playCurrent();
    bool playFile(utils::path fileName);

    // Fetches one companion file `s` (relative to the song) into `parentDir`,
    // materialising it where the player's loader expects it. Used for both the
    // plugin's direct secondary files and the members of a directory companion.
    void loadSecondaryFile(const std::string& s, const utils::File& parentDir,
                           const std::string& songDirUrl);

    // Handles an LHA-packed song path ("<archive>.lha/<member>", used by the
    // UnExoticA collection): downloads the archive into the cache, extracts its
    // members and queues the requested member for playback.
    void loadLhaSong(const std::string& prefix, const std::string& path);

    void update();
    void updateInfo();

    std::deque<std::string> errors;

    MusicPlayer mp;
    MusicDatabase& musicDatabase;
    RemoteLoader& remoteLoader;

    // Lock when accessing MusicPlayer
    mutable std::mutex plMutex;

    struct PlayQueue
    {
        std::atomic<bool> updated;
        std::deque<SongInfo> songs;
        std::deque<SongInfo> psongs;
        std::string prodScreenshot;
        [[nodiscard]] size_t size() const
        {
            return songs.size() + psongs.size();
        }
        void push_back(const SongInfo& s)
        {
            songs.push_back(s);
            updated = true;
        }
        // void push_font(const SongInfo &s) { songs.push_front(s); }
        void clear()
        {
            psongs.clear();
            songs.clear();
            updated = true;
        }
        void pop_front()
        {
            if (psongs.size() > 0)
                psongs.pop_front();
            else
                songs.pop_front();
            updated = true;
        }
        SongInfo& front()
        {
            if (psongs.size() > 0) return psongs.front();
            return songs.front();
        }

        SongInfo& getSong(size_t i)
        {
            if (i < psongs.size()) return psongs[i];
            return songs[i - psongs.size()];
        }

        const SongInfo& getSong(size_t i) const
        {
            if (i < psongs.size()) return psongs[i];
            return songs[i - psongs.size()];
        }

        void insertAt(int i, const SongInfo& s)
        {
            songs.insert(songs.begin() + i, s);
            updated = true;
        }
        bool wasUpdated()
        {
            bool rc = updated;
            updated = false;
            return rc;
        }
    };

    PlayQueue playList;

    std::atomic<bool> wasAllowed{ true };
    std::atomic<bool> quitThread{ false };

    std::atomic<int> currentTune{ 0 };
    std::atomic<bool> playing{ false };
    std::atomic<bool> paused{ false };
    std::atomic<int> bitRate{ 0 };
    std::atomic<int> playerPosition{ 0 };
    std::atomic<int> playerLength{ 0 };

    std::atomic<int> files{ 0 };
    std::string loadedFile;

    std::atomic<State> state{ Stopped };
    SongInfo currentInfo;
    SongInfo dbInfo;

    std::thread playerThread;

    bool changedSong = false;

    bool detectSilence = true;

    std::shared_ptr<CueSheet> cueSheet;
    std::string subtitle;
    std::atomic<const char*> subtitlePtr{ nullptr };

    // Snapshot of the decoder's "message" metadata, refreshed by the player
    // thread under plMutex so the render thread can read it without touching mp.
    std::string message;

    int multiSongNo = 0;
    std::vector<std::string> multiSongs;
    bool changedMulti = false;
    bool playedNext = false;

    std::vector<utils::File> songFiles;
};

} // namespace chipmachine

