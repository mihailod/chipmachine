#pragma once

#include "MusicDatabase.h"
#include "MusicPlayerList.h"
#include "RemoteLoader.h"

#include <coreutils/utils.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace chipmachine {

// The engine-facing facade. Everything a front end needs to drive playback and
// query the song database lives here, so a UI can be written against this class
// alone and never reach into MusicPlayerList / MusicDatabase / RemoteLoader.
//
// Two front ends exist: runConsole() in textmode.cpp (drives only this class)
// and ChipMachine (the grappix GUI, which still holds MusicPlayerList& directly
// and is being ported over). This is also the boundary a future out-of-process
// protocol would be cut along, so keep the methods here coarse and
// serialisable: pass values, not engine internals.
//
// Everything below is a thin forward to the engine. Do not add behaviour here
// that a front end would then be unable to opt out of -- state lives in the
// engine, not in the facade.
class ChipInterface
{
public:
    // NOTE: this constructor indexes the database SYNCHRONOUSLY (initFromLua),
    // which blocks until the ~776k-song index is ready. That suits textmode,
    // which has nothing to draw until then. The GUI instead calls
    // initFromLuaAsync() itself so it can render its indexing progress screen
    // while the index builds. A front end that must stay responsive during
    // indexing therefore CANNOT adopt this constructor as-is -- decoupling the
    // DB init from the facade is a prerequisite for that (see isReindexing()/
    // dbBusy() below, which exist to drive exactly such a progress screen).
    ChipInterface(const utils::path& wd, RemoteLoader& rl, MusicPlayerList& mpl,
                  MusicDatabase& mdb)
        : remoteLoader(rl), workDir(wd), mdb(mdb), player(mpl)
    {
        mdb.initFromLua(wd);
    }

    // ---------------------------------------------------------------------
    // Transport
    // ---------------------------------------------------------------------

    int play(const SongInfo& song)
    {
        player.playSong(song);
        return 0;
    }

    void addSong(const SongInfo& song) { player.addSong(song); }
    void addSong(const SongInfo& song, bool shuffle)
    {
        player.addSong(song, shuffle);
    }
    void nextSong() { player.nextSong(); }
    void clearSongs() { player.clearSongs(); }
    void stop() { player.stop(); }

    void pause(bool p) { return player.pause(p); }
    bool playing() { return player.isPlaying(); }
    [[nodiscard]] bool isPaused() const { return player.isPaused(); }

    // Select subtune `t` of the current song.
    void setTune(int t) { player.seek(t); }
    // Full seek: subtune + offset in seconds (-1 keeps the current offset).
    void seek(int song, int seconds = -1) { player.seek(song, seconds); }
    [[nodiscard]] int getTune() const { return player.getTune(); }

    // ---------------------------------------------------------------------
    // Playback state / progress
    // ---------------------------------------------------------------------

    [[nodiscard]] int seconds() const { return player.getPosition(); }
    [[nodiscard]] int getPosition() const { return player.getPosition(); }
    [[nodiscard]] int getLength() const { return player.getLength(); }
    [[nodiscard]] int getBitrate() const { return player.getBitrate(); }

    // True once the current song's first samples have reached the DAC -- the
    // "buffering finished" signal for progressive streams.
    [[nodiscard]] bool hasAudioStarted() const
    {
        return player.hasAudioStarted();
    }

    float getVolume() { return player.getVolume(); }
    void setVolume(float volume) { player.setVolume(volume); }

    // DANGER -- pollState() is a CONSUMING read. MusicPlayerList::getState()
    // rewrites Playstarted -> Playing as a side effect of reading it, so the
    // Playstarted edge is delivered to exactly ONE caller, once. Two pollers
    // (or one poller plus update() below) means one of them silently never sees
    // Playstarted, and whatever per-song work it hangs off that edge -- new
    // metadata, screenshot lookup, subtune reset -- just stops happening on
    // some songs. That failure is invisible to cmtest.
    //
    // Rule: exactly one call per front end per frame, and a front end that
    // calls this must NOT also call update().
    MusicPlayerList::State pollState() { return player.getState(); }

    [[nodiscard]] bool hasError() const { return player.hasError(); }
    std::string getError() { return player.getError(); }

    // ---------------------------------------------------------------------
    // Current song / metadata
    // ---------------------------------------------------------------------

    [[nodiscard]] SongInfo getInfo(int index = 0) const
    {
        return player.getInfo(index);
    }
    [[nodiscard]] SongInfo getDBInfo() const { return player.getDBInfo(); }
    std::string getMeta(const std::string& what) { return player.getMeta(what); }

    // The files backing the current song (module + its companion samples).
    [[nodiscard]] const std::vector<utils::File>& getSongFiles() const
    {
        return player.getSongFiles();
    }

    // ---------------------------------------------------------------------
    // Play queue
    // ---------------------------------------------------------------------

    [[nodiscard]] int listSize() const { return player.listSize(); }
    // True if the current song came off the queue rather than a direct play.
    [[nodiscard]] bool wasFromQueue() const { return player.wasFromQueue(); }
    // Consuming read: clears the "updated" flag. One caller per front end.
    bool playlistUpdated() { return player.playlistUpdated(); }

    // Whether `info` will play by progressive streaming (short prebuffer) vs a
    // whole-file download -- the same predicate the engine routes on, so a UI
    // showing "BUFFERING..." vs "LOADING..." always matches the real path.
    static bool willStream(const SongInfo& info)
    {
        return MusicPlayerList::willStream(info);
    }

    // ---------------------------------------------------------------------
    // Audio tap (spectrum analyser)
    // ---------------------------------------------------------------------

    // Raw PCM tap, called on the audio thread. In-process this is a direct
    // forward -- same thread, same timing, no buffering added.
    //
    // NOTE for any future out-of-process split: do NOT ship this PCM over a
    // transport (44.1kHz stereo ~= 172 KB/s, and the GUI deliberately delays
    // the spectrum to line it up with audible output). Run the FFT engine-side
    // and ship the reduced levels instead.
    void setAudioCallback(const std::function<void(int16_t*, int)>& cb)
    {
        player.setAudioCallback(cb);
    }

    // ---------------------------------------------------------------------
    // Song database
    // ---------------------------------------------------------------------

    std::shared_ptr<IncrementalQuery> createQuery()
    {
        std::lock_guard<std::mutex> lg(m);
        return mdb.createQuery();
    }

    SongInfo getSongInfo(int i) { return mdb.getSongInfo(i); }

    [[nodiscard]] bool dbBusy() const { return mdb.busy(); }
    [[nodiscard]] bool isReindexing() const { return mdb.isReindexing(); }

    // ---------------------------------------------------------------------
    // Asset availability (drives the UI's cached "*" / local "+" marks and the
    // download progress bar)
    // ---------------------------------------------------------------------

    [[nodiscard]] bool inCache(const std::string& path) const
    {
        return remoteLoader.inCache(path);
    }
    bool isOffline(const std::string& p) { return remoteLoader.isOffline(p); }
    [[nodiscard]] bool isLocalFile(const std::string& p) const
    {
        return remoteLoader.isLocalFile(p);
    }
    [[nodiscard]] bool downloadProgress(int64_t& downloaded,
                                        int64_t& total) const
    {
        return remoteLoader.downloadProgress(downloaded, total);
    }

    // Escape hatch. Every current use is covered by the four calls above --
    // prefer those. This exists so the GUI can be ported incrementally; it
    // cannot survive an out-of-process split and should be deleted once the
    // last caller is gone.
    RemoteLoader& getRemoteLoader() { return remoteLoader; }

    // ---------------------------------------------------------------------
    // Pump
    // ---------------------------------------------------------------------

    // Drives the meta callbacks registered via onMeta(). Consumes the
    // Playstarted edge -- see the DANGER note on pollState(). A front end uses
    // EITHER update()+onMeta() (textmode) OR pollState() (the GUI, which does
    // its own per-song work inline), never both.
    void update()
    {
        playerState = player.getState();
        if (playerState == MusicPlayerList::Playstarted) {
            info = player.getInfo();
            for (auto& cb : meta_callbacks)
                (*cb)(info);
        }
    }

    using MetaCallback = std::function<void(const SongInfo&)>;
    using MetaHolder = std::shared_ptr<std::function<void(std::nullptr_t)>>;

    MetaHolder onMeta(const MetaCallback& callback)
    {
        std::lock_guard<std::mutex> lg(m);
        meta_callbacks.push_back(std::make_shared<MetaCallback>(callback));
        auto mc = meta_callbacks.back();
        (*mc)(info);
        return MetaHolder(nullptr, [=](std::nullptr_t) {
            std::lock_guard<std::mutex> lg(m);
            meta_callbacks.erase(
                std::remove(meta_callbacks.begin(), meta_callbacks.end(), mc),
                meta_callbacks.end());
        });
    }

private:
    RemoteLoader& remoteLoader;
    utils::path workDir;
    std::mutex m;
    MusicDatabase& mdb;
    SongInfo info;
    MusicPlayerList& player;
    MusicPlayerList::State playerState{ MusicPlayerList::State::Stopped };
    std::vector<std::shared_ptr<MetaCallback>> meta_callbacks;
};

} // namespace chipmachine
