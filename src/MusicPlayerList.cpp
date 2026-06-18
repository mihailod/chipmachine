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
    // Quit the FIFOs first so any thread blocked in sfifo->put() (e.g. the curl
    // streaming thread) is immediately unblocked. cancelStreaming() and the
    // playerThread join both complete quickly after this.
    mp.quit();
    cancelStreaming();
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
        if (!currentInfo.path.empty() && currentInfo.path.back() == '\r') {
            currentInfo.path.pop_back();
        }
        // Tag the codec so playCurrent() streams it (Shoutcast .pls entries
        // resolve to extension-less URLs like ".../stream"; without a format
        // the stream gate fails and we'd try to download an endless stream).
        currentInfo.format =
            toLower(path_extension(currentInfo.path)) == "ogg" ? "OGG" : "MP3";
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
        // Pick the codec from the resolved stream URL so non-mp3 radio streams
        // (e.g. Kohina's .ogg) are tagged correctly; the actual decoder is
        // chosen by extension in playCurrent().
        currentInfo.format =
            toLower(path_extension(currentInfo.path)) == "ogg" ? "OGG" : "MP3";
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
                if (dbInfo.path == infoCopy.path) {
                    currentInfo.metadata = infoCopy.metadata;
                    if (shot != "") {
                        currentInfo.metadata[SongInfo::SCREENSHOT] = shot;
                    }
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

    // Radio streaming: let ffmpeg fetch and decode the resolved stream URL
    // directly. It handles mp3/ogg/aac, redirects and Shoutcast/ICY mounts
    // (including bare "ICY 200 OK" servers that the curl+mpg123 path rejected).
    bool extStreamable = (ext == "mp3" || ext == "ogg" || ext == "aac" ||
                          ext == "m4a" || ext == "mp4");
    if (currentInfo.format != "M3U" &&
        (extStreamable || toLower(currentInfo.format) == "mp3" ||
         toLower(currentInfo.format) == "ogg")) {

        // Resolve "prefix::relpath" to the full URL (source.url + relpath) the
        // way stream()/load() would, so ffmpeg gets a fetchable URL. Passing the
        // raw currentInfo.path would feed ffmpeg the "radio::" prefix ("Protocol
        // not found"), and the bare relpath would be a non-existent local file.
        if (mp.streamUrl(remoteLoader.resolveUrl(currentInfo.path))) {
            SET_STATE(Playstarted);
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
        // The cached file has a URL-encoded name (e.g. "downloads.php%3fmoduleid=1")
        // which may contain bogus extensions like ".php". Use the format field
        // from the database (e.g. "XM") as the real extension.
	LOGD("Detected ext: %s", currentInfo.ext);
        if (!currentInfo.ext.empty()) {
            auto fname = f0.getName();
            auto wantExt = "." + utils::toLower(currentInfo.ext);
            auto curExt = utils::path_extension(fname);
            if (curExt != wantExt) {
                auto newFile = fname + wantExt;
		LOGD("Detected ext from Content-Disposition: %s", curExt.c_str());
		LOGD("New ext from Content-Disposition: %s", wantExt.c_str());
                rename(fname.c_str(), newFile.c_str());
                f0 = File{ newFile };
            }
        }
        auto parentDir = File(path_directory(f0.getName()));
        auto songDirUrl = path_directory(currentInfo.path);

        // Pure-streaming companion-name alignment: the web cache stores the song
        // under a URL-encoded name (e.g. "ftp%3A%2F...%2Faquatic games.sng"), but
        // several UADE multi-file players derive a companion's name from the
        // song's ON-DISK basename -- Richard Joseph swaps .sng->.INS, MusicMaker
        // .sdata->.ip, etc. loadSecondaryFile() materialises those companions
        // under their clean Modland names next to the song, so unless the song
        // ALSO carries its clean basename the derivation can never match and the
        // tune streams silent. Re-materialise the song under its own clean
        // filename beside the companions. In a local mirror the cached name is
        // already the real name, so this is a no-op.
        auto cleanName = path_filename(currentInfo.path);
        if (!cleanName.empty() && path_filename(f0.getName()) != cleanName) {
            File cleanSong = parentDir / cleanName;
            if (!cleanSong.exists()) {
                File::copy(f0.getName(), cleanSong.getName());
            }
            f0 = cleanSong;
        }

        songFiles.push_back(f0);
        loadedFile = f0.getName();
        for (const auto& s : mp.getSecondaryFiles(f0)) {
            if (s == "./") {
                // The song's OWN directory (e.g. a MaxTrax shared-bank set whose
                // instrument file's name can't be predicted from a score part):
                // list the song folder and fetch every sibling next to it. The
                // prefix is empty so members land directly in parentDir. A local
                // mirror yields an empty list (members are read in place).
                files++;
                remoteLoader.listDirectory(
                    songDirUrl, [=](std::vector<std::string> names) {
                        for (const auto& n : names) {
                            loadSecondaryFile(n, parentDir, songDirUrl);
                        }
                        files--;
                    });
            } else if (!s.empty() && s.back() == '/') {
                // A whole-directory companion (e.g. IFF-SMUS "Instruments/"):
                // the member filenames are unpredictable, so list the remote
                // folder and fetch each into the same subdirectory. A local
                // mirror yields an empty list (members are read in place).
                files++;
                auto dirUrl = songDirUrl + "/" + s;
                remoteLoader.listDirectory(
                    dirUrl, [=](std::vector<std::string> names) {
                        for (const auto& n : names) {
                            loadSecondaryFile(s + n, parentDir, songDirUrl);
                        }
                        files--;
                    });
            } else {
                loadSecondaryFile(s, parentDir, songDirUrl);
            }
        }

        files--;

    });

}

void MusicPlayerList::loadSecondaryFile(const std::string& s,
                                        const utils::File& parentDir,
                                        const std::string& songDirUrl)
{
    File target = parentDir / s;
    if (target.exists()) {
        songFiles.push_back(target);
        return;
    }
    files++;
    auto url = songDirUrl + "/" + s;
    remoteLoader.load(url, [=](File f) {
        // Secondary files are companions (sample banks, voicesets), not the song
        // itself. Treat a missing one as non-fatal: the main file already
        // loaded, so let the plugin play whatever it can (e.g. MoonBlaster
        // renders bankless without its .mbk) rather than failing the whole song
        // on an absent companion.
        if (!f) {
            LOGW("Could not load secondary file %s", url);
        } else {
            // The web cache flattens a companion's remote directory into a
            // single encoded folder, so a companion in a SUBdirectory of the
            // song (e.g. IFF-SMUS "Instruments/<name>") is not downloaded to
            // parentDir/s where the player's file loader looks. Materialise a
            // copy there. Same-directory companions already land in place
            // (f == target), so this is a no-op for them.
            if (f.getName() != target.getName() && !target.exists()) {
                utils::makedirs(path_directory(target.getName()));
                File::copy(f.getName(), target.getName());
            }
            songFiles.push_back(target.exists() ? target : f);
        }
        files--;
    });
}

} // namespace chipmachine


