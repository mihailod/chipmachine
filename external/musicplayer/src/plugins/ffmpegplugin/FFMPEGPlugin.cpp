#include "FFMPEGPlugin.h"
#include "../../chipplayer.h"

#include <coreutils/exec.h>
#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <coreutils/format.h>

#include <coreutils/fifo.h>

#include <set>
#include <unordered_map>
#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <memory>
#include <thread>
#include <chrono>

namespace musix {

class FFMPEGPlayer : public ChipPlayer
{
public:
    FFMPEGPlayer(const std::string& fileName, const std::string& ffmpeg)
    {
        // For remote URLs (YouTube's googlevideo CDN, radio, etc.) let ffmpeg
        // reconnect after a mid-stream disconnect instead of treating it as a
        // fatal EOF. googlevideo routinely drops long-lived HTTPS connections
        // without a TLS close_notify (SecureTransport "IO Error: -9806"), which
        // otherwise surfaces as "partial file" + "Error during demuxing" and
        // ends the song after a minute or two of fine playback -- random across
        // videos, since it depends on the CDN edge dropping the socket. These
        // are http-protocol options and MUST come before -i; ffmpeg resumes by
        // re-issuing a Range request from the current byte offset. Only apply
        // them to URL inputs so local files don't get unused-option warnings.
        const char* reconnect =
            utils::startsWith(fileName, "http")
                ? "-reconnect 1 -reconnect_streamed 1 "
                  "-reconnect_on_network_error 1 -reconnect_delay_max 5 "
                : "";
        // googlevideo binds a stream URL to the User-Agent of the yt-dlp player
        // client that minted it. We pin android_vr (see lua/init.lua), so the URL
        // carries c=ANDROID_VR and its edge nodes will intermittently return
        // "403 Forbidden" when ffmpeg fetches with its default "Lavf/.." UA --
        // enforcement is per-edge/probabilistic (no pot token), which is why the
        // same track fails on open then plays on a fresh re-resolve. Present the
        // matching UA so the request is accepted the first time. Keep this string
        // in sync with android_vr's userAgent in yt-dlp's youtube/_base.py.
        std::string userAgent;
        if (fileName.find("googlevideo.com") != std::string::npos) {
            userAgent =
                "-user_agent \"com.google.android.apps.youtube.vr.oculus/1.65.10 "
                "(Linux; U; Android 12L; eureka-user Build/SQ3A.220605.009.A1) gzip\" ";
        }
        auto command = fmt::format("{} {}{}-i \"{}\" -v error -ac 2 -ar 44100 -f s16le -",
                        ffmpeg, reconnect, userAgent, fileName);
        LOGD("FFMPEG RUNNING COMMAND: %s", command.c_str());
        pipe = utils::execPipe(command);
        // Poll, never block. getSamples() runs on the MusicPlayerList player
        // thread *while it holds plMutex*. If the read blocks -- or, worse, spins
        // in a multi-second retry sleep -- the lock is held the whole time and the
        // UI/render thread, which needs plMutex for getInfo()/getMeta(), freezes.
        // That is the "plays for a few seconds, then the whole app locks up and
        // goes silent" hang on finite ffmpeg files (e.g. IFF-8SVX one-shot
        // samples): at end-of-stream the old loop slept out its entire retry
        // budget under the lock. Non-blocking + a prompt return keeps the hold
        // short so the render thread can interleave.
        pipe.setReadNonBlocking();
    }

    // Streaming constructor: feed the incoming byte fifo into ffmpeg's stdin
    // (pipe:0) on a dedicated thread while getSamples() reads decoded PCM from
    // its stdout. ffmpeg probes the container itself, so this handles ogg/aac/
    // etc. radio streams. A separate feeder thread is required to avoid a pipe
    // deadlock (writing stdin while ffmpeg blocks on a full stdout).
    FFMPEGPlayer(std::shared_ptr<utils::Fifo<uint8_t>> fifo,
                 const std::string& ffmpeg)
        : streaming(true), fifo(std::move(fifo))
    {
        auto command =
            fmt::format("{} -i pipe:0 -v error -ac 2 -ar 44100 -f s16le -", ffmpeg);
        LOGD("FFMPEG RUNNING STREAM COMMAND: %s", command.c_str());
        pipe = utils::execPipe(command);
        // Poll, don't block: while the download is still buffering, getSamples()
        // must return promptly (as "buffering") so the player thread stays
        // responsive instead of blocking inside read().
        pipe.setReadNonBlocking();
        feeder = std::thread([this] { feedLoop(); });
    }

    // Called (from the download/web thread) when the source is fully fetched: the
    // feeder drains whatever is left then closes ffmpeg's stdin so it flushes the
    // tail and exits, which getSamples() sees as SONG_END.
    void endStream() override { producer_done = true; }

    ~FFMPEGPlayer() override
    {
        stop = true;
        // The feeder may be parked in a blocking write() to ffmpeg's stdin (its
        // stdout filled once we stopped reading). Closing stdin makes that write
        // fail so the feeder exits and join() can't hang. Then it no longer
        // touches the pipe, so Kill() is race-free.
        if (streaming) pipe.closeWrite();
        if (feeder.joinable()) feeder.join();
        pipe.Kill();
    }

    int getSamples(int16_t* target, int noSamples) override
    {
        if (streaming) {
            // Non-blocking read on the player thread. ffmpeg only emits PCM once
            // it has enough input to decode, so this naturally "prebuffers": we
            // report buffering (0 samples) until the first frames are ready, then
            // play as soon as they arrive.
            int rc = pipe.read(reinterpret_cast<uint8_t*>(target), noSamples * 2);
            if (rc > 0) { return rc / 2; }
            if (rc == -1) {
                // EAGAIN: no decoded audio right now. Normally that means we are
                // still buffering -> report 0 ("not ended, retry"). But once the
                // whole input has been fed AND ffmpeg has exited, there will never
                // be more audio: end the song. (We can't rely on a read() EOF
                // alone: if a stray copy of the output pipe's write end leaks into
                // another process/fd, the read never reports EOF.)
                if (producer_done && pipe.hasEnded()) { return -1; }
                return 0;
            }
            // rc == 0 (stdin closed -> ffmpeg flushed and exited) or rc == -2
            // (pipe gone): the stream is finished. -1 = SONG_END.
            return -1;
        }
        // Local files and finite remote URLs (pipe is non-blocking, see ctor).
        // ffmpeg keeps the pipe full whenever it has decoded audio, so a normal
        // read returns data immediately. Critically, do NOT block or sleep here:
        // this runs under the player-list lock and any stall freezes the UI (see
        // the ctor comment). Do a single non-blocking read and return promptly.
        int rc = pipe.read(reinterpret_cast<uint8_t*>(target), noSamples * 2);
        if (rc > 0) { return rc / 2; }
        // EOF (ffmpeg closed stdout) or pipe gone: SONG_END. Returning 0 here
        // would make the host retry forever and hang.
        if (rc == 0 || rc == -2) { return -1; }
        // rc == -1 (EAGAIN): no decoded audio right now. If ffmpeg has already
        // exited there will never be any more -> end the song. Otherwise it is
        // still decoding/buffering (e.g. a network URL prebuffering): report
        // "no data yet" (0) so the caller drops the lock and polls again instead
        // of us spinning here with the lock held.
        if (pipe.hasEnded()) { return -1; }
        return 0;
    }

    bool seekTo(int /*song*/, int /*seconds*/) override { return false; }

private:
    void feedLoop()
    {
        std::array<uint8_t, 8192> buf{};
        while (!stop) {
            int sz = fifo->filled();
            if (sz <= 0) {
                // Drained everything and the download is complete: close stdin so
                // ffmpeg flushes its last frames and exits. Done once.
                if (producer_done) {
                    pipe.closeWrite();
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            if (sz > static_cast<int>(buf.size())) sz = buf.size();
            int got = fifo->get(buf.data(), sz);
            if (got <= 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            int off = 0;
            while (off < got && !stop) {
                int w = pipe.write(buf.data() + off, got - off);
                if (w <= 0) { stop = true; break; } // ffmpeg gone (EPIPE)
                off += w;
            }
        }
    }

    bool streaming{false};
    utils::ExecPipe pipe;
    std::shared_ptr<utils::Fifo<uint8_t>> fifo;
    std::thread feeder;
    std::atomic<bool> stop{false};
    std::atomic<bool> producer_done{false};
};

FFMPEGPlugin::FFMPEGPlugin()
{
#ifdef _WIN32
    ffmpeg = "bin\\ffmpeg.exe";
#else
    ffmpeg = "ffmpeg";
#endif
    LOGD("FFMPEG PLUGIN INITIALIZED WITH PATH '%s'", ffmpeg.c_str());
}

// Container/codec extensions the bundled ffmpeg decodes into PCM. Kept as the
// single source of truth for both canHandle() and getSupportedExtensions() so
// the two never drift. The compressed-lossy set (mp3/aac/m4a/mp4/ogg/opus/mp2/
// mpeg/ac3) plus the lossless PCM containers (wav/flac/aiff/aif) that show up
// across the demoscene collections (demozoo has ~685 wav, ~343 flac, ~60 mp2,
// ~6 aif/aiff, ~4 opus; scene.org compos add raw .mpeg audio and Dolby .ac3) --
// all handled by ffmpeg's demuxers, previously dropped because the gate omitted
// them.
static const std::set<std::string>& ffmpegExtensions()
{
    static const std::set<std::string> exts = {
        "m4a", "aac", "mp3", "mp4", "ogg", "opus", "mp2", "mpeg", "ac3",
        "wav", "flac", "aiff", "aif", "8svx", "wma"};
    return exts;
}

bool FFMPEGPlugin::canHandle(const std::string& name)
{
    LOGD("FFMPEGPlugin::canHandle checking '%s'", name.c_str());
    // Since MusicPlayer already called makeLower(), 'name' is guaranteed to be lowercase.
    // Match on the trailing extension against the supported set.
    auto ext = utils::path_extension(name);
    if (!ext.empty() && ffmpegExtensions().count(ext) > 0) return true;
    // IFF-8SVX Amiga samples (modland/UnExoticA "8svx.<name>" prefix form). UADE
    // ships no 8SVX eagleplayer, but ffmpeg's IFF demuxer decodes them (incl.
    // Fibonacci-delta), so route them here. Content is FORM..8SVX; the prefix is
    // a reliable selector for the modland naming convention.
    if (utils::path_prefix(name) == "8svx") return true;
    return false;
}

std::set<std::string> FFMPEGPlugin::getSupportedExtensions() const
{
    return ffmpegExtensions();
}


ChipPlayer* FFMPEGPlugin::fromFile(const std::string& fileName)
{
    LOGD("FFMPEGPlugin::fromFile loading '%s'", fileName.c_str());
    return new FFMPEGPlayer{fileName, ffmpeg};
};

ChipPlayer* FFMPEGPlugin::fromStream(std::shared_ptr<utils::Fifo<uint8_t>> fifo)
{
    return new FFMPEGPlayer(std::move(fifo), ffmpeg);
}
} // namespace musix

extern "C" void ffmpegplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& config) {
        return std::make_shared<musix::FFMPEGPlugin>();
    });
}
