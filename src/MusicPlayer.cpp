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
            // Timestamp BEFORE advancing the position: a reader that catches
            // the two stores half-done then sees the old position against the
            // new time, i.e. interpolates ~nothing, rather than the old time
            // against the new position and running a whole buffer ahead.
            play_chunk = size / 2;
            play_pos_ms = utils::getms();
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

            uint64_t chunk_start = gen_pos;
            int samples_generated =
                player->getSamples(&temp_buf[0], space_left - 1024);

            if (samples_generated <= 0) {
                play_ended = samples_generated < 0;
                break;
            }
            // Must happen before anything else can call getSamples again: the
            // offsets the player recorded are relative to the call above.
            collectTrackerRows(chunk_start);
            gen_pos += static_cast<uint64_t>(samples_generated) / 2;
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

void MusicPlayer::collectTrackerRows(uint64_t chunkStart)
{
    if (!tracker_view) { return; }
    player->takeTrackerRows(tracker_scratch);
    if (tracker_scratch.empty()) { return; }
    std::lock_guard<std::mutex> lock(tracker_mutex);
    for (auto& r : tracker_scratch) {
        // Bounded: nothing drains this in the headless/console builds, and the
        // render thread can also stall. Dropping the oldest costs at worst a
        // few rows scrolled past unseen.
        if (tracker_rows.size() >= 4096) { tracker_rows.pop_front(); }
        tracker_rows.push_back(
            { chunkStart + static_cast<uint64_t>(r.frameOffset), r });
    }
}

void MusicPlayer::resetTrackerRows(uint64_t position)
{
    // Every caller has just jumped play_pos (new song, seek), so the
    // interpolation baseline belongs to the old position. Drop it here rather
    // than in four call sites; interpolatedPos() falls back to the raw value
    // until the next audio callback re-establishes it.
    play_pos_ms = 0;
    play_chunk = 0;

    std::lock_guard<std::mutex> lock(tracker_mutex);
    tracker_rows.clear();
    gen_pos = position;
    tracker_current_pos = position;
    tracker_has_current = false;
}

uint64_t MusicPlayer::interpolatedPos() const
{
    auto pos = static_cast<uint64_t>(play_pos.load());
    auto chunk = static_cast<uint64_t>(play_chunk.load());
    auto since = play_pos_ms.load();
    if (chunk == 0 || since == 0 || paused) { return pos; }
    auto now = utils::getms();
    if (now <= since) { return pos; }
    uint64_t ahead = (now - since) * 44100 / 1000;
    // Never more than the buffer that is currently playing out, so this stays
    // an interpolation and never becomes a guess.
    return pos + std::min(ahead, chunk);
}

bool MusicPlayer::takeTrackerRows(std::vector<musix::TrackerRow>& out,
                                  std::vector<musix::TrackerRow>& upcoming,
                                  float& fraction)
{
    out.clear();
    upcoming.clear();
    fraction = 0.0F;
    if (!tracker_view) { return false; }

    auto now = interpolatedPos();
    std::lock_guard<std::mutex> lock(tracker_mutex);
    while (!tracker_rows.empty() && tracker_rows.front().pos <= now) {
        out.push_back(tracker_rows.front().row);
        tracker_current_pos = tracker_rows.front().pos;
        tracker_has_current = true;
        tracker_rows.pop_front();
    }
    if (tracker_has_current && !tracker_rows.empty()) {
        auto next = tracker_rows.front().pos;
        if (next > tracker_current_pos) {
            fraction = static_cast<float>(now - tracker_current_pos) /
                       static_cast<float>(next - tracker_current_pos);
            if (fraction < 0.0F) { fraction = 0.0F; }
            if (fraction > 1.0F) { fraction = 1.0F; }
        }
    }
    // Everything still queued is, by definition, decoded but not yet heard.
    // A screen's worth is plenty; the queue itself holds far more than that.
    size_t n = std::min<size_t>(tracker_rows.size(), kTrackerLookahead);
    upcoming.reserve(n);
    for (size_t i = 0; i < n; i++) {
        upcoming.push_back(tracker_rows[i].row);
    }
    return true;
}

MusicPlayer::~MusicPlayer()
{
    // 1. Null out the callback under the mutex so fill_audio() becomes a no-op.
    audio_player->play(nullptr);

    // 2. Stop the AudioQueue for good, synchronously.
    //
    //    This used to be pause(). Do NOT go back to that: AudioQueuePause() only
    //    halts callback dispatch, so a buffer callback already handed to the
    //    client queue stayed pending forever. The AudioQueueDispose() that runs
    //    later (when the main()-scoped audio_player shared_ptr finally drops)
    //    then blocked the main thread in AwaitAllPendingCallbacks until
    //    CoreAudio's ~10s timeout — the freeze-on-quit, after which macOS killed
    //    the app as unresponsive. It reproduced with nothing playing at all,
    //    since the queue is fed silence either way.
    //
    //    close() stops with immediate=true, which flushes pending callbacks and
    //    guarantees the audio thread is not in fill_audio() when it returns — a
    //    strictly stronger guarantee than pause() ever gave, so the FIFO teardown
    //    below is still race-free.
    audio_player->close();

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
        resetTrackerRows(static_cast<uint64_t>(play_pos.load()));
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

void MusicPlayer::endStream()
{
    if (player) player->endStream();
}

void MusicPlayer::abortStream()
{
    if (stream_fifo) stream_fifo->quit();
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
    LOGD("TEARDOWN: streamFile resetting old player");
    player = nullptr; // destroys previous FFMPEGPlayer (joins feeder, closes pipe)
    LOGD("TEARDOWN: old player destroyed");

    utils::makeLower(name);
    check_silence = true;

    // Fresh fifo per streaming session. A previous, cancelled stream may still
    // have a producer (the curl/web thread) mid-put; giving each session its own
    // fifo means stale bytes land in the orphaned old fifo, never in this song's.
    stream_fifo = std::make_shared<utils::Fifo<uint8_t>>(32768 * 8);

    // Always stream through ffmpeg: it probes the container itself and its player
    // implements endStream()/EOF so playback ends cleanly when the download does.
    for (auto& plugin : musix::ChipPlugin::getPlugins()) {
        if (plugin->name() == "ffmpeg" && plugin->canHandle(name)) {
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

        fifo.clear();
        fadeout_pos = 0;
        pause(false);
        play_pos = 0;
        // Streamed audio (ffmpeg) is never a tracker.
        tracker_view = false;
        resetTrackerRows(0);
        message = "";
        length = 0;
        sub_title = "";
        currentTune = playing_info.starttune;
        return true;
    }
    return false;
}

bool MusicPlayer::streamUrl(const std::string& url)
{
    dont_play = true;
    silent_frames = 0;
    playing_info = SongInfo();
    player = nullptr;
    check_silence = false; // live streams take a moment to buffer; don't cut

    // Hand the URL straight to ffmpeg (fromFile spawns `ffmpeg -i <url>`), which
    // does its own HTTP and container/codec handling.
    for (auto& plugin : musix::ChipPlugin::getPlugins()) {
        if (plugin->name() == "ffmpeg") {
            auto newPlayer =
                std::shared_ptr<musix::ChipPlayer>(plugin->fromFile(url));
            if (newPlayer) player = newPlayer;
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
        tracker_view = false;
        resetTrackerRows(0);
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
        tracker_view = player->hasTrackerRows();
        resetTrackerRows(0);
        updatePlayingInfo();
        currentTune = playing_info.starttune;
        return true;
    }
    tracker_view = false;
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
        // Several plugins can claim the same extension -- ".mus" alone is FAC
        // SoundTracker (MSX, needs .SM1/.SM2 drumkits), C64 Sidplayer and UFO
        // Amiga. fromFile() tries the claimers in order until one actually
        // LOADS, so the plugin that ends up playing is not necessarily the first
        // canHandle match. If we returned the first match's companions we would
        // fetch the wrong (usually empty) list and the real player would then
        // fail for want of its side files. Return the first NON-EMPTY companion
        // list among the claimers instead, so e.g. KSSPlugin's FAC drumkits are
        // fetched even though OpenMPT/libvice claim ".mus" first and need none.
        for (auto& plugin : musix::ChipPlugin::getPlugins()) {
            if (plugin->canHandle(name)) {
                auto secondary = plugin->getSecondaryFiles(name);
                if (!secondary.empty()) { return secondary; }
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

