#include "MusicPlayerList.h"
#include "LhaArchive.h"
#include <archive/archive.h>
#include <set>

#include <algorithm>
#include <cctype>
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
        if (lines.empty()) {
            errors.emplace_back("Empty playlist");
            SET_STATE(Error);
            return false;
        }
        std::string first = lines[0];
        if (!first.empty() && first.back() == '\r') first.pop_back();

        // Modland's console collections (SGC/KSS/NSF/GBS/HES/...) expose each
        // subtune as a tiny GME-style .m3u that points at a SHARED module file:
        //   "Alien Syndrome.sgc::KSS,0,<title>,<len>,..."
        // i.e. "<filename>[::<type>],<track>,...". These are NOT radio
        // playlists -- resolve to the sibling module in the same remote
        // directory and start the named subtune, rather than streaming the
        // playlist text to ffmpeg (which yields "No such file"). Radio .m3u
        // entries are always http(s) URLs, so they skip this branch.
        bool isUrl = startsWith(toLower(first), "http://") ||
                     startsWith(toLower(first), "https://");
        auto sep = first.find("::");
        std::string fileField, rest;
        if (sep != std::string::npos) {
            fileField = first.substr(0, sep);
            rest = first.substr(sep + 2); // "<type>,<track>,..."
            auto c = rest.find(',');      // drop the "::type" token
            rest = (c == std::string::npos) ? "" : rest.substr(c + 1);
        } else {
            auto c = first.find(',');
            fileField = (c == std::string::npos) ? first : first.substr(0, c);
            rest = (c == std::string::npos) ? "" : first.substr(c + 1);
        }
        std::string fext = toLower(path_extension(fileField));
        bool subtuneM3u = !isUrl && !fileField.empty() && !fext.empty() &&
                          fext != ".m3u" && fext != ".mp3" && fext != ".ogg";
        if (subtuneM3u) {
            // Leading integer of the remaining fields = the 0-based subtune.
            int track = 0;
            size_t i = 0;
            while (i < rest.size() && rest[i] == ' ') i++;
            size_t j = i;
            while (j < rest.size() && std::isdigit((unsigned char)rest[j])) j++;
            if (j > i) track = std::stoi(rest.substr(i, j - i));

            // Resolve the sibling module's URL, preserving the source prefix so
            // remoteLoader.load() fetches it from the same collection/host.
            std::string prefix, rel;
            auto pp = split(currentInfo.path, std::string("::"), size_t(2));
            if (pp.size() == 2) {
                prefix = pp[0];
                rel = pp[1];
            } else
                rel = currentInfo.path;
            std::string dir = path_directory(rel);
            std::string modRel = dir.empty() ? fileField : dir + "/" + fileField;
            currentInfo.path = prefix.empty() ? modRel : prefix + "::" + modRel;
            currentInfo.ext = fext.substr(1);    // "sgc"
            currentInfo.format = fext.substr(1); // routed/described by extension
            currentInfo.starttune = track;
            currentInfo.numtunes = 0;
            playCurrent();
            return false;
        }

        currentInfo.path = first;
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
    //LOGD("TEARDOWN: cancelStreaming begin");
    remoteLoader.cancel();
    //LOGD("TEARDOWN: remoteLoader.cancel done");
    // quit() (not clear()) the fifo: if the web thread is blocked in put()
    // feeding a stream we're abandoning, only quitting unblocks it -- otherwise
    // it would wedge curl_multi_perform and stall every transfer. streamFile()
    // allocates a fresh fifo for the next song.
    mp.abortStream();
    //LOGD("TEARDOWN: abortStream done");
}

// Stream a remote, finite, ffmpeg-decodable file: curl fetches it into a fifo
// that feeds ffmpeg's stdin, so playback starts after a short prebuffer instead
// of waiting for the whole file to download. Returns false if no streaming
// player could be created (caller falls back to a direct ffmpeg URL).
bool MusicPlayerList::streamRemoteFile(const std::string& path)
{
    LOGD("TEARDOWN: streamRemoteFile begin %s", path.c_str());
    if (!mp.streamFile(path)) return false;
    LOGD("TEARDOWN: streamFile created player");

    auto weakPlayer = mp.getPlayer();
    auto fifo = mp.getStreamFifo();
    remoteLoader.stream(
        path,
        [weakPlayer, fifo](int what, const uint8_t* data, int size) -> bool {
            // The player is gone (song switched): returning false aborts the
            // transfer. abortStream() has already quit this fifo, so any put()
            // below returns immediately rather than blocking.
            auto player = weakPlayer.lock();
            if (!player) return false;
            if (what == RemoteLoader::DATA && data != nullptr && size > 0) {
                fifo->put(data, size); // blocks for backpressure (web thread)
            } else if (what == RemoteLoader::END) {
                player->endStream();
            }
            return true;
        });
    return true;
}

void MusicPlayerList::update()
{
    // Refill the audio FIFO WITHOUT holding plMutex. The decode loop in
    // mp.update() can take several milliseconds for emulated chips; holding
    // plMutex across it stalls the render thread, which acquires the same lock
    // every frame to read metadata (getInfo/getMeta/getVolume). That stall is
    // what made the scroller stutter while music was playing. mp owns its own
    // synchronization (the FIFO is thread-safe and its flags are atomic), and
    // it is only ever decoded from this thread, so it is safe to run unlocked.
    mp.update();

    LOCK_GUARD(plMutex);

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

    // Snapshot the decoder message under the lock (we are on the same thread
    // that wrote mp.message in mp.update() above, so this read is safe) so the
    // render thread can serve getMeta("message") from this cache.
    message = mp.getMeta("message");
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
    // Strip any URL query (".mp3?p=f" -> "mp3") so the streamable test and the
    // decoder see the real codec. Some podcast feeds (AmigaVibes via podCloud)
    // append a query to the enclosure URL, which otherwise left ext empty and
    // dropped the episode onto the no-extension full-download path (no decoder).
    auto qpos = ext.find('?');
    if (qpos != std::string::npos) ext = ext.substr(0, qpos);
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

    // UnExoticA (and any LHA-packed source): the song path is
    // "<archive>.lha/<member>". Fetch the .lha into the cache, extract its
    // members, then play the requested member.
    if (toLower(path).find(".lha/") != std::string::npos) {
        loadedFile = "";
        files = 0;
        loadLhaSong(prefix, path);
        return;
    }

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

    bool extStreamable = (ext == "mp3" || ext == "ogg" || ext == "aac" ||
                          ext == "m4a" || ext == "mp4");
    // A bare "MP3"/"OGG" codec tag is set only by .pls/.m3u resolution, i.e. a
    // radio/Shoutcast stream (often an extension-less ICY mount). Those stay on
    // the direct-ffmpeg-URL path, which handles ICY/redirects/endless streams.
    bool isRadioStream = (toLower(currentInfo.format) == "mp3" ||
                          toLower(currentInfo.format) == "ogg");
    // Podcasts are always streamed (their enclosure URLs sometimes lack a
    // detectable extension), via the switch-safe streamRemoteFile path below.
    bool isPodcast = MusicDatabase::classifyFormat(currentInfo.format,
                                                   currentInfo.path) == PODCAST;
    if (currentInfo.format != "M3U" &&
        (extStreamable || isRadioStream || isPodcast)) {

        // Resolve "prefix::relpath" to the full URL (source.url + relpath) the
        // way stream()/load() would, so ffmpeg gets a fetchable URL. Passing the
        // raw currentInfo.path would feed ffmpeg the "radio::" prefix ("Protocol
        // not found"), and the bare relpath would be a non-existent local file.
        if (isRadioStream) {
            // Endless radio: let ffmpeg fetch and decode the resolved URL
            // directly (handles bare "ICY 200 OK" mounts the curl path rejects).
            if (mp.streamUrl(remoteLoader.resolveUrl(currentInfo.path))) {
                SET_STATE(Playstarted);
            }
            return;
        }
        // Any finite remote file -- a real .mp3/.ogg/... in a collection, OR a
        // podcast episode: stream it progressively through curl->fifo->ffmpeg so
        // it plays after a short prebuffer instead of a full download. This path
        // is now switch-safe: aborting an in-flight transfer (RemoteLoader::cancel
        // + MusicPlayer::abortStream) detaches and frees the curl handle under the
        // web mutex, so a 100MB+ download can be cancelled mid-flight without
        // racing the next track's transfer. Fall back to a direct ffmpeg URL if
        // the progressive stream can't start.
        if (streamRemoteFile(currentInfo.path) ||
            mp.streamUrl(remoteLoader.resolveUrl(currentInfo.path))) {
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

        // --- ZIP-by-magic: the downloaded file is a .zip of playable members --
        // Zophar's Domain console gamerips (01.vgm, 02.vgm, ...) AND Demozoo
        // scene.org compo entries (a lone .mod/.xm/.it/.sid/... or a streamed
        // .mp3/.ogg, plus a readme). Detect the ZIP by magic (PK\x03\x04),
        // extract every member next to the cache file (so a module's companion
        // samples / a shared usflib/gsflib land alongside), and present the
        // playable tracks as local subsongs (multiSongs). Switching tracks then
        // plays an already-extracted local file -- no re-download.
        {
            bool isZip = false;
            if (FILE* fp = fopen(f0.getName().c_str(), "rb")) {
                unsigned char m[4] = { 0 };
                isZip = fread(m, 1, 4, fp) == 4 && m[0] == 'P' && m[1] == 'K' &&
                        m[2] == 3 && m[3] == 4;
                fclose(fp);
            }
            if (isZip) {
                // "Song" members (chip/console rips + Amiga/PC tracker modules)
                // we prefer; "audio" members (streamed renderings) are the
                // fallback so a compo zip shipping a module + its .mp3 preview
                // plays the module, while a pure-.mp3/.ogg zip still plays.
                // Anything else (nfo/diz/txt/exe/sample.wav) is ignored.
                static const std::set<std::string> songExt = {
                    "vgm", "vgz", "nsf", "nsfe", "spc", "gbs", "hes", "kss",
                    "sgc", "ay", "gym", "usf", "miniusf", "gsf", "minigsf",
                    "psf", "minipsf", "2sf", "mini2sf", "ssf", "dsf", "sid",
                    "psid", "sndh", "sap", "ym", "sc68", "pt3", "pt2", "pt1",
                    "stc", "stp", "sqt", "asc", "vtx", "psc", "mod", "xm", "it",
                    "s3m", "mtm", "669", "far", "okt", "med", "mmd0", "mmd1",
                    "mmd2", "mmd3", "dbm", "digi", "ahx", "hvl", "thx", "dmf",
                    "ptm", "stm", "ult", "amf", "psm", "mt2", "gt2", "dtm", "fc",
                    "fc13", "fc14", "aon", "smod", "dw", "cust", "mptm" };
                static const std::set<std::string> audioExt = {
                    "mp3", "ogg", "flac", "wav", "mp2", "m4a", "aac", "opus" };
                std::string dir = f0.getName() + "_x";
                utils::makedirs(dir);
                std::vector<std::string> songs, audio;
                try {
                    auto* a = utils::Archive::open(f0.getName(), dir);
                    // open() returns null for an unrecognised container. We got
                    // here by ZIP magic, so it should be a ZipFile -- but never
                    // dereference null (a SIGSEGV here is NOT caught by catch).
                    if (a == nullptr) { throw std::runtime_error("archive open failed"); }
                    for (auto const& m : *a) {
                        // Skip macOS resource forks and dotfiles -- their ext
                        // would otherwise spoof a bogus track (e.g. ._x.mp3).
                        if (m.rfind("__MACOSX/", 0) == 0) continue;
                        if (path_filename(m).rfind(".", 0) == 0) continue;
                        auto ef = a->extract(m);
                        auto ext = toLower(path_extension(m));
                        if (songExt.count(ext) > 0)
                            songs.push_back(ef.getName());
                        else if (audioExt.count(ext) > 0)
                            audio.push_back(ef.getName());
                    }
                } catch (...) {}
                std::vector<std::string>& tracks = songs.empty() ? audio : songs;
                std::sort(tracks.begin(), tracks.end());
                if (tracks.empty()) {
                    errors.emplace_back("No playable tracks in archive");
                    SET_STATE(Error);
                    files--;
                    return;
                }
                multiSongs = tracks; // local extracted paths (no source prefix)
                multiSongNo = 0;
                currentInfo.numtunes = (int)tracks.size();
                currentInfo.path = tracks[0];
                loadedFile = tracks[0];
                files--;
                return;
            }
        }

        // --- LHA-by-magic for StSound (.ym/.mix) ------------------------------
        // CPC-Power's Amstrad CPC .ym game rips are LHA archives wrapping a
        // single inner YM stream. StSound's built-in LZH depacker only handles
        // level-0 LHA headers, so the level-1 archives some rips use (e.g.
        // "BeTiled!") make its load fail outright -- it can't depack them and the
        // tune won't play (and used to crash on the resulting NULL song name).
        // Extract the inner member with lhasa (extractLha handles every LHA
        // level), which yields the bare YM stream StSound then plays directly.
        // Level-0 archives pass through here too and play identically, so the
        // route is uniform. The Content-Disposition ext-rename block below
        // re-tags the extracted member (".ym5") as ".ym" so it routes to StSound.
        if (ext == "ym" || ext == "mix") {
            unsigned char m[7] = { 0 };
            bool isLha = false;
            if (FILE* fp = fopen(f0.getName().c_str(), "rb")) {
                isLha = fread(m, 1, 7, fp) == 7 && m[2] == '-' &&
                        m[3] == 'l' && m[4] == 'h' && m[6] == '-';
                fclose(fp);
            }
            if (isLha) {
                std::string dir = f0.getName() + "_lha";
                for (auto const& rel : extractLha(f0.getName(), dir)) {
                    File member = File(dir + "/" + rel);
                    if (member.exists()) {
                        // One YM stream per rip -- play the first member.
                        f0 = member;
                        break;
                    }
                }
            }
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
                // Copy, don't rename: the original URL-named file IS the cache
                // entry that getFile()/inCache() look up next time. Renaming it
                // away makes every replay miss the cache and re-download (e.g.
                // modarchive "downloads.php?moduleid=N" -> N.mod). The extension
                // copy is what the player routes on; the bare original stays put.
                if (!File::exists(newFile)) File::copy(fname, newFile);
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
        // Only re-materialise when the song's own path is a real filename WITH
        // an extension (the companion-alignment formats: RJP .sng, MusicMaker
        // .sdata, ...). Moduleid-based collections (e.g. modarchive, whose path
        // is just "1") have no extension here -- re-copying would strip the
        // ".xm"/".umx" the Content-Disposition rename just added and leave the
        // player unable to route the file by extension.
        auto cleanName = path_filename(currentInfo.path);
        if (!cleanName.empty() && !path_extension(cleanName).empty() &&
            path_filename(f0.getName()) != cleanName) {
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
                //
                // The folder's case differs by source -- the Sonix score
                // references "Instruments/" but modland stores "instruments/" --
                // and FTP listing is case-sensitive. So if the requested case
                // lists empty, retry with the first letter's case flipped before
                // giving up. Members are fetched/placed under whichever case
                // actually exists remotely; the local FS is case-insensitive, so
                // the player finds them regardless of the case it asks for.
                files++;
                std::string alt = s;
                if (!alt.empty() && std::isalpha((unsigned char)alt[0])) {
                    alt[0] = std::islower((unsigned char)alt[0])
                                 ? (char)std::toupper((unsigned char)alt[0])
                                 : (char)std::tolower((unsigned char)alt[0]);
                }
                auto fetchInto = [=](const std::string& dir,
                                     const std::vector<std::string>& names) {
                    for (const auto& n : names) {
                        loadSecondaryFile(dir + n, parentDir, songDirUrl);
                    }
                };
                remoteLoader.listDirectory(
                    songDirUrl + "/" + s,
                    [=](std::vector<std::string> names) {
                        if (!names.empty() || alt == s) {
                            fetchInto(s, names);
                            files--;
                            return;
                        }
                        // Requested case was empty -- try the flipped case.
                        remoteLoader.listDirectory(
                            songDirUrl + "/" + alt,
                            [=](std::vector<std::string> names2) {
                                fetchInto(alt, names2);
                                files--;
                            });
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

void MusicPlayerList::loadLhaSong(const std::string& prefix,
                                  const std::string& path)
{
    // Split "<archive>.lha/<member>" into the archive path and the member name.
    // The music type lives at the FRONT of the member ("mod.mix0" -> a ".mod"
    // tune), exactly like Modland's prefix-form names, so the player/UADE layer
    // handles the member as-is.
    auto lpos = toLower(path).find(".lha/");
    std::string archiveRel = path.substr(0, lpos + 4); // ".../X.lha"
    std::string member = path.substr(lpos + 5);        // member after ".lha/"

    // Fetch the archive through the same source as the song so RemoteLoader
    // resolves it against the collection's base URL (or local mirror).
    std::string archivePath =
        prefix.empty() ? archiveRel : (prefix + "::" + archiveRel);

    // Stable per-archive extraction dir under the cache: re-selecting any tune
    // from the same archive reuses the already-extracted members, and a song's
    // in-archive companions are extracted alongside it (preserving subdirs like
    // "instruments/" that some replayers need -- see extractLha).
    //
    // The "_lha" -> "_lha2" bump invalidates caches written by the older,
    // flattening extractLha: those dropped subdirectories, so reusing them left
    // e.g. Sonix tunes without their instruments ("score died"). A fresh dir
    // forces re-extraction with the current layout instead of needing a manual
    // cache wipe.
    auto safeName = prefix + archiveRel;
    std::replace(safeName.begin(), safeName.end(), '/', '_');
    auto destDir = (Environment::getCacheDir() / "_lha2" / safeName).string();
    std::string memberFile = destDir + "/" + member;

    // Already extracted in a previous selection -- play it straight away.
    if (utils::exists(memberFile)) {
        songFiles.push_back(File(memberFile));
        loadedFile = memberFile;
        files = 0;
        return;
    }

    files++;
    remoteLoader.load(archivePath, [=](File f) {
        // A subsong's archive can be missing: UnExoticA's song list references
        // stale per-version .lha paths (e.g. Hybris/Custom_Version.lha) that no
        // longer exist on the server (FTP 550). When this song is one tune of a
        // collapsed game (MULTI), don't fail the whole game -- skip to the next
        // subsong, which may live in an archive that does exist. Only the last
        // remaining subsong surfaces a real error.
        auto failOrSkip = [=](const char* msg) {
            if (!multiSongs.empty() &&
                multiSongNo + 1 < (int)multiSongs.size()) {
                multiSongNo++;
                SET_STATE(Playmulti);
            } else {
                errors.emplace_back(msg);
                SET_STATE(Error);
            }
            files--;
        };

        if (!f) {
            failOrSkip("Could not load archive");
            return;
        }
        auto extracted = extractLha(f.getName(), destDir);
        if (!utils::exists(memberFile)) {
            failOrSkip("Could not extract song from archive");
            return;
        }
        songFiles.push_back(File(memberFile));
        loadedFile = memberFile;
        files--;
    });
}

} // namespace chipmachine


