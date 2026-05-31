#include "MusicPlayerList.h"

#include <algorithm>
#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <unordered_map>
#include <fstream>

#include <coreutils/environment.h>
#include <musicplayer/src/chipplayer.h>

using namespace utils;

namespace chipmachine {

MusicPlayerList::~MusicPlayerList()
{
    cancelStreaming();
    mp.quit();
    quitThread = true;
    if (playerThread.joinable())
        playerThread.join();
}

MusicPlayerList::MusicPlayerList(MusicDatabase& mdb, RemoteLoader& rl,
                                 std::shared_ptr<AudioPlayer> ap)
    : mp(std::move(ap)), remoteLoader(rl), musicDatabase(mdb)
{

    playerThread = std::thread([=] {

        while (!quitThread) {
            plMutex.lock();
            if (!funcs.empty()) {
                auto q = funcs;
                funcs.clear();
                plMutex.unlock();
                for (auto& f : q) {
                    f();
                }
            } else {
                plMutex.unlock();
            }
            update();

            // Adaptive sleep: poll tightly while loading/transitioning so curl
            // callbacks and file-count changes are picked up immediately.
            // On Apple Silicon, sleepms(50) was causing 2-3s stalls because the
            // scheduler honours the full sleep on E-cores, stacking 3+ cycles
            // of 50ms latency before files==0 was seen and playFile() was called.
            {
                bool busy = (state == Loading  ||
                             state == Started  ||
                             state == Playnow  ||
                             state == Playmulti);
                sleepms(busy ? 2 : 20);
            }
        }
    });
}

void MusicPlayerList::wait()
{
    if (funcs.empty()) {
        onThisThread([=] {});
    }
    plMutex.lock();
    while (!funcs.empty()) {
        plMutex.unlock();
        sleepms(1);
        plMutex.lock();
    }
    plMutex.unlock();
}

void MusicPlayerList::addSong(const SongInfo& si, bool shuffle)
{
    onThisThread([=] {
        if (shuffle) {
            playList.insertAt(rand() % (playList.size() + 1), si);
        } else {
            playList.push_back(si);
        }
    });
}

void MusicPlayerList::clearSongs()
{
    onThisThread([=] { playList.clear(); });
}

void MusicPlayerList::nextSong()
{
    onThisThread([=] {
        if (playList.size() > 0) {
            SET_STATE(Waiting);
        }
    });
}

void MusicPlayerList::playSong(const SongInfo& si)
{

    onThisThread([=] {
        
        dbInfo = currentInfo = si;
        SET_STATE(Playnow);
        
    });
    
}

void MusicPlayerList::seek(int song, int seconds)
{
    onThisThread([=] {
        if (!multiSongs.empty()) {
            state = Playmulti;
            multiSongNo = song;
            return;
        }
        mp.seek(song, seconds);
        if (song >= 0) changedSong = true;
    });
}

SongInfo MusicPlayerList::getInfo(int index) const
{
    LOCK_GUARD(plMutex);
    if (index == 0) return currentInfo;
    return playList.getSong(index - 1);
}

SongInfo MusicPlayerList::getDBInfo() const
{
    LOCK_GUARD(plMutex);
    return dbInfo;
}

int MusicPlayerList::getLength() const
{
    return playerLength;
}

int MusicPlayerList::getPosition() const
{
    return playerPosition;
}

int MusicPlayerList::listSize() const
{
    LOCK_GUARD(plMutex);
    return playList.size();
}

/// PRIVATE

void MusicPlayerList::updateInfo()
{
    auto si = mp.getPlayingInfo();
    if (si.format != "") currentInfo.format = si.format;
    if (multiSongs.empty()) {
        currentInfo.numtunes = si.numtunes;
        currentInfo.starttune = si.starttune;
    }
}

bool MusicPlayerList::handlePlaylist(const std::string& fileName)
{
    playList.clear();
    File f{ fileName };

    auto lines = f.getLines();

    lines.erase(
        std::remove_if(lines.begin(), lines.end(),
                       [=](const std::string& l) { return l[0] == ';'; }),
        lines.end());
    for (const std::string& s : lines) {
        playList.push_back(SongInfo(s));
    }

    if (playList.size() == 0) return false;

    musicDatabase.lookup(playList.front());
    if (playList.front().path == "") {
        errors.emplace_back("Bad song in playlist");
        SET_STATE(Error);
        return false;
    }
    SET_STATE(Waiting);
    return true;
}

bool MusicPlayerList::playFile(utils::path fileName)
{
    if (fileName == "") return false;
    auto ext = toLower(fileName.extension());
    if (ext == ".pls" || currentInfo.format == "PLS") {
        File f{ fileName };

        auto lines = f.getLines();
        std::vector<std::string> result;
        for (auto& l : lines) {
            if (startsWith(l, "File1=")) result.push_back(l.substr(6));
        }
        currentInfo.path = result[0];
        currentInfo.format = "MP3";
        playCurrent();
        return false;

    } else if (ext == ".m3u" || currentInfo.format == "M3U") {
        File f{ fileName };

        auto lines = f.getLines();

        lines.erase(std::remove_if(lines.begin(), lines.end(),
                                   [=](const std::string& l) {
                                       return l == "" || l[0] == '#';
                                   }),
                    lines.end());
        currentInfo.path = lines[0];
        currentInfo.format = "MP3";
        playCurrent();
        return false;

    } else if (ext == ".plist") {
        handlePlaylist(fileName.string());
        return true;
    } else if (ext == ".jb") {
        auto newName = fileName;
        newName.replace_extension(".jcb");
        if (!exists(newName)) utils::copy(fileName, newName);
        fileName = newName;
    }

    bool success = mp.playFile(fileName.string());
    

    if (success) {
        if (currentInfo.starttune >= 0) mp.seek(currentInfo.starttune);
        changedSong = false;
        if (!changedMulti) {
            updateInfo();
            SET_STATE(Playstarted);
        } else
            SET_STATE(Playing);

        bitRate = 0;
        changedMulti = false;
        return true;
    } else {
        errors.emplace_back("Could not play song");
        SET_STATE(Error);
    }
    return false;
}

void MusicPlayerList::cancelStreaming()
{
    remoteLoader.cancel();
    mp.clearStreamFifo();
}

void MusicPlayerList::update()
{
    LOCK_GUARD(plMutex);

    mp.update();
    remoteLoader.update();

    if (state == Playnow) {
        SET_STATE(Started);
        multiSongs.clear();
        playedNext = false;
        playCurrent();
    }

    if (state == Playmulti) {
        SET_STATE(Started);
        currentInfo.path = multiSongs[multiSongNo];
        changedMulti = true;
        playCurrent();
    }

    if (state == Playing || state == Playstarted) {

        auto pos = mp.getPosition();
        auto length = mp.getLength();

        if (cueSheet) {
            subtitle = cueSheet->getTitle(pos);
            subtitlePtr = subtitle.c_str();
        }

        if (!changedSong && playList.size() > 0) {
            if (!mp.playing()) {
                if (playList.size() == 0)
                    SET_STATE(Stopped);
                else
                    SET_STATE(Waiting);
            } else if ((length > 0 && pos > length) && pos > 7) {
                mp.fadeOut(3.0);
                SET_STATE(Fading);
            } else if (detectSilence && mp.getSilence() > 44100 * 6 &&
                       pos > 7) {
                mp.fadeOut(0.5);
                SET_STATE(Fading);
            }
        }
    }

    if (state == Fading) {
        if (mp.getFadeVolume() <= 0.01) {
            if (playList.size() == 0)
                SET_STATE(Stopped);
            else
                SET_STATE(Waiting);
        }
    }

    if (state == Loading) {
        if (files == 0) {
            cancelStreaming();
            playFile(loadedFile);
        }
    }

    if (state == Waiting && (playList.size() > 0)) {
        SET_STATE(Started);
        playedNext = true;
        dbInfo = currentInfo = playList.front();
        playList.pop_front();

        if (playList.size() > 0) {
            musicDatabase.lookup(playList.front());
        }

        multiSongs.clear();
        playCurrent();
    }

    playerPosition = mp.getPosition();
    playerLength = mp.getLength();

    if (!multiSongs.empty())
        currentTune = multiSongNo;
    else
        currentTune = mp.getTune();

    playing = mp.playing();
    paused = mp.isPaused();
    auto br = mp.getMeta("bitrate");
    if (br != "") {
        bitRate = std::stol(br);
    }

    if (!cueSheet) {
        subtitle = mp.getMeta("sub_title");
        subtitlePtr = subtitle.c_str();
    }
}

void MusicPlayerList::playCurrent()
{
    SET_STATE(Loading);
    songFiles.clear();

    std::string prefix, path;
    auto parts = split(currentInfo.path, "::", 2);
    if (parts.size() == 2) {
        prefix = parts[0];
        path = parts[1];
    } else
        path = currentInfo.path;

    if (prefix == "index") {
        int index = stol(path);
        dbInfo = currentInfo = musicDatabase.getSongInfo(index);
        auto parts = split(currentInfo.path, "::", 2);
        if (parts.size() == 2) {
            prefix = parts[0];
            path = parts[1];
        } else
            path = currentInfo.path;
    }

    if (prefix == "product") {
        auto id = stol(path);
        playList.psongs.clear();
        for (const auto& song : musicDatabase.getProductSongs(id)) {
            playList.psongs.push_back(song);
        }
        if (playList.psongs.empty()) {
            errors.emplace_back("No songs in product");
            SET_STATE(Error);
            return;
        }

        musicDatabase.lookup(playList.psongs.front());
        if (playList.psongs.front().path == "") {
            errors.emplace_back("Bad song in product");
            SET_STATE(Error);
            return;
        }
        SET_STATE(Waiting);
        return;
    } else {
        if (currentInfo.metadata[SongInfo::SCREENSHOT] == "") {
            // Pre-resolve the song info on the worker thread (safe: dbMutex,
            // main db connection) before handing off to the detached thread.
            // getSongScreenshots() calls lookup() internally which hits the
            // main db — running it here avoids a cross-thread db race.
            musicDatabase.lookup(currentInfo);

            // Dispatch the screenshot DB query asynchronously so it doesn't
            // block the network request dispatch. screenshotDb is a dedicated
            // connection so it is safe to use from the detached thread.
            auto& mdb = musicDatabase;
            SongInfo infoCopy = currentInfo;
            std::thread([this, &mdb, infoCopy]() mutable {
                auto shot = mdb.getSongScreenshots(infoCopy);
                // Write back only if still on the same song
                LOCK_GUARD(plMutex);
                if (currentInfo.path == infoCopy.path && shot != "") {
                    currentInfo.metadata[SongInfo::SCREENSHOT] = shot;
                }
            }).detach();
        }
    }

    if (prefix == "playlist") {
        if (!handlePlaylist(path)) SET_STATE(Error);
        return;
    }

    if (startsWith(path, "MULTI:")) {
        multiSongs = split(path.substr(6), "\t");
        if (prefix != "") {
            for (std::string& m : multiSongs) {
                m = prefix + "::" + m;
            }
        }
        multiSongNo = 0;
        currentInfo.path = multiSongs[0];
        currentInfo.numtunes = multiSongs.size();
        playCurrent();
        return;
    }

    auto ext = path_extension(path);
    makeLower(ext);

    detectSilence = true;
    if (ext == "mp3") detectSilence = false;

    cueSheet = nullptr;
    subtitle = "";
    subtitlePtr = subtitle.c_str();

    playerPosition = 0;
    playerLength = 0;
    bitRate = 0;
    currentTune = 0;

    cancelStreaming();

    bool local_exists = utils::exists(currentInfo.path);

    if (local_exists) {
        songFiles = { File(currentInfo.path) };
        loadedFile = currentInfo.path;
        files = 0;
        
        return;
    }

    loadedFile = "";
    files = 0;

    std::string cueName = "";
    if (prefix == "bitjam")
        cueName =
            currentInfo.path.substr(0, currentInfo.path.find_last_of('.')) +
            ".cue";
    else if (prefix == "demovibes")
        cueName = toLower(
            currentInfo.path.substr(0, currentInfo.path.find_last_of('.')) +
            ".cue");

    if (cueName != "") {
        remoteLoader.load(cueName, [=](File cuefile) {
            if (cuefile) cueSheet = std::make_shared<CueSheet>(cuefile);
        });
    }

    if (startsWith(currentInfo.path, "pouet::")) {
        loadedFile = currentInfo.path.substr(7);
        files = 0;
        return;
    }

    if (currentInfo.format != "M3U" &&
        (ext == "mp3" || toLower(currentInfo.format) == "mp3")) {

        if (mp.streamFile("dummy.mp3")) {
            SET_STATE(Playstarted);
            auto sfifo = mp.getStreamFifo();
            auto self = shared_from_this();
            remoteLoader.stream(
                currentInfo.path,
                [self, sfifo](int what, const uint8_t* ptr, int n) -> bool {
                    if (sfifo->isQuitting()) return false;
                    if (what == RemoteLoader::PARAMETER) {
                        self->mp.setParameter((char*)ptr, n);
                    } else if (what == RemoteLoader::DATA) {
                        sfifo->put(ptr, n);
                    } else if (what == RemoteLoader::END) {
                        sfifo->put(nullptr, 0);
                    }
                    return !sfifo->isQuitting();
                });
        }
        return;
    }

    files++;

    remoteLoader.load(currentInfo.path, [=](File f0) {

        if (!f0) {
            errors.emplace_back("Could not load file");
            SET_STATE(Error);
            files--;
            return;
        }
        songFiles.push_back(f0);
        loadedFile = f0.getName();
        auto parentDir = File(path_directory(loadedFile));
        auto fileList = mp.getSecondaryFiles(f0);
        for (const auto& s : fileList) {
            File target = parentDir / s;
            if (!target.exists()) {
                files++;
                auto url = path_directory(currentInfo.path) + "/" + s;
                remoteLoader.load(url, [=](File f) {
                    if (!f) {
                        errors.emplace_back("Could not load file");
                        SET_STATE(Error);
                    } else {
                        songFiles.push_back(f);
                    }
                    files--;
                });
            } else
                songFiles.push_back(target);
        }

        files--;
        
    });
    
}

} // namespace chipmachine


