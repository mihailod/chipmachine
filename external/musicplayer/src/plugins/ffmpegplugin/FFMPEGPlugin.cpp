#include "FFMPEGPlugin.h"
#include "../../chipplayer.h"

#include <coreutils/log.h>
#include <coreutils/utils.h>

#include <coreutils/fifo.h>

// libav* is linked directly (see CMakeLists) and used IN-PROCESS -- we no longer
// spawn the ffmpeg command-line binary. Decoding a bundled/spawned executable is
// what made this app ineligible for sandboxed distribution (App Store guideline
// 2.5.2); linking the LGPL libraries instead is the compliant shape and also
// drops the ~51MB CLI and the fragile stdin/stdout pipe plumbing. Behaviour is
// preserved exactly: same output format (s16le / stereo / 44100), same
// getSamples() contract, same reconnect + User-Agent handling for remote URLs,
// same progressive-streaming (feed-bytes-as-they-arrive) path.
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace musix {

namespace {

constexpr int kOutRate = 44100;
constexpr int kOutChannels = 2;
// Decoded-PCM ring between the decode thread (producer) and getSamples()
// (consumer). Interleaved int16. ~1.5s of stereo audio -- large enough that a
// single decoded+resampled frame always fits, small enough to bound latency.
constexpr int kPcmRing = 1 << 17; // 131072 int16
// Per-put chunk cap: never hand Fifo::put() a block larger than this so `count`
// is always << ring size (Fifo::put() blocks until the WHOLE count fits).
constexpr int kPutChunk = 1 << 13; // 8192 int16
// Buffer libav reads through for the custom (streaming) IO path.
constexpr int kAvioBuf = 32768;

} // namespace

// One decode session. A dedicated worker thread runs the libav demux/decode/
// resample loop and pushes interleaved s16 stereo into `pcm`; getSamples() (on
// the player thread, under plMutex) only ever does a NON-BLOCKING drain of that
// ring. This mirrors the old process boundary -- the player thread never blocks
// on network or decode work, which is essential because a stall under plMutex
// freezes the UI/render thread (see the historical FFMPEG hang notes).
class FFMPEGPlayer : public ChipPlayer
{
public:
    // Local file or remote URL (radio, resolved YouTube googlevideo CDN, ...).
    explicit FFMPEGPlayer(std::string url)
        : pcm(kPcmRing), inputUrl(std::move(url))
    {
        worker = std::thread([this] { decodeLoop(); });
    }

    // Progressive streaming: bytes are fed into `f` by the download thread while
    // we decode them. libav pulls input through our readPacket() callback, so it
    // probes the container itself (ogg/aac/wav/flac/... radio + finite streams).
    explicit FFMPEGPlayer(std::shared_ptr<utils::Fifo<uint8_t>> f)
        : pcm(kPcmRing), streaming(true), fifo(std::move(f))
    {
        worker = std::thread([this] { decodeLoop(); });
    }

    // Streaming path: the source is fully fetched; no more bytes will arrive.
    // readPacket() returns EOF once the fifo drains, so the decoder flushes its
    // tail and exits -> getSamples() reports SONG_END.
    void endStream() override { producerDone = true; }

    ~FFMPEGPlayer() override
    {
        // Order matters. Set stop first so the interrupt callback aborts any
        // blocking libav network read and readPacket() stops polling; quit the
        // PCM ring so a decode thread parked in pcm.put() (ring full because
        // getSamples() is no longer draining) wakes and returns. Then join.
        stop = true;
        pcm.quit();
        if (worker.joinable()) worker.join();
    }

    int getSamples(int16_t* target, int size) override
    {
        // NON-BLOCKING drain. Fifo::get() returns min(size, filled) immediately
        // (or -1 while quitting). Never sleeps/blocks: runs under plMutex.
        int got = pcm.get(target, size);
        if (got > 0) { return got; }
        if (got < 0) { return -1; } // ring quitting -> treat as end
        // Ring momentarily empty. If the decoder has finished (EOF/error) and
        // there is nothing left to hand out, end the song; otherwise we are
        // still buffering -> 0 tells the host to drop the lock and poll again.
        if (finished && pcm.filled() == 0) { return -1; }
        return 0;
    }

    bool seekTo(int /*song*/, int /*seconds*/) override { return false; }

private:
    // libav interrupt callback: abort blocking I/O (avformat_open_input /
    // av_read_frame on a stalled network socket) when we are tearing down.
    static int interruptCb(void* opaque)
    {
        return static_cast<FFMPEGPlayer*>(opaque)->stop ? 1 : 0;
    }

    // Custom AVIO read for the streaming path: pull the next bytes out of the
    // download fifo. Runs on the worker thread, so blocking here is fine -- it
    // never touches plMutex. Polls (2ms) while the download is still catching
    // up; returns EOF when drained-and-done, or EXIT on teardown.
    static int readPacket(void* opaque, uint8_t* buf, int bufSize)
    {
        auto* self = static_cast<FFMPEGPlayer*>(opaque);
        for (;;) {
            if (self->stop) { return AVERROR_EXIT; }
            int avail = self->fifo->filled();
            if (avail > 0) {
                if (avail > bufSize) { avail = bufSize; }
                int got = self->fifo->get(buf, avail);
                if (got > 0) { return got; }
                if (got < 0) { return AVERROR_EXIT; } // fifo quitting
            }
            // Nothing buffered right now.
            if (self->producerDone) { return AVERROR_EOF; }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }

    // Push one decoded frame (resampled to s16 stereo 44100) into the PCM ring,
    // chunked so each Fifo::put() count comfortably fits. put() blocks on a full
    // ring (backpressure, exactly like the old ffmpeg-blocks-on-full-stdout) and
    // returns immediately once pcm.quit() is called on teardown.
    void pushSamples(const int16_t* data, int nInt16)
    {
        int off = 0;
        while (off < nInt16 && !stop && !pcm.isQuitting()) {
            int n = std::min(nInt16 - off, kPutChunk);
            pcm.put(data + off, n);
            off += n;
        }
    }

    void decodeLoop()
    {
        AVFormatContext* fmt = avformat_alloc_context();
        if (fmt == nullptr) { finished = true; return; }
        fmt->interrupt_callback.callback = &FFMPEGPlayer::interruptCb;
        fmt->interrupt_callback.opaque = this;

        unsigned char* avioBuf = nullptr;
        AVIOContext* avio = nullptr;
        AVDictionary* opts = nullptr;

        if (streaming) {
            avioBuf = static_cast<unsigned char*>(av_malloc(kAvioBuf));
            if (avioBuf == nullptr) {
                avformat_free_context(fmt);
                finished = true;
                return;
            }
            avio = avio_alloc_context(avioBuf, kAvioBuf, /*write*/ 0, this,
                                      &FFMPEGPlayer::readPacket, nullptr, nullptr);
            if (avio == nullptr) {
                av_free(avioBuf);
                avformat_free_context(fmt);
                finished = true;
                return;
            }
            fmt->pb = avio;
            fmt->flags |= AVFMT_FLAG_CUSTOM_IO;
        } else {
            // Remote-URL knobs (identical to the old CLI flags). googlevideo
            // routinely drops long-lived HTTPS sockets without a TLS close_notify
            // ("partial file" / "Error during demuxing"); reconnect makes libav
            // re-issue a Range request from the current offset instead of ending
            // the song. Only meaningful for network protocols; harmless
            // otherwise, but scope to http* to avoid noise on local files.
            if (utils::startsWith(inputUrl, "http")) {
                av_dict_set(&opts, "reconnect", "1", 0);
                av_dict_set(&opts, "reconnect_streamed", "1", 0);
                av_dict_set(&opts, "reconnect_on_network_error", "1", 0);
                av_dict_set(&opts, "reconnect_delay_max", "5", 0);
            }
            // googlevideo binds a stream URL to the User-Agent of the yt-dlp
            // client that minted it (we pin android_vr in lua/init.lua -> the URL
            // carries c=ANDROID_VR); fetching with libav's default "Lavf/.." UA
            // intermittently gets 403. Present the matching UA. Keep in sync with
            // android_vr's userAgent in yt-dlp's youtube/_base.py.
            if (inputUrl.find("googlevideo.com") != std::string::npos) {
                av_dict_set(&opts, "user_agent",
                            "com.google.android.apps.youtube.vr.oculus/1.65.10 "
                            "(Linux; U; Android 12L; eureka-user "
                            "Build/SQ3A.220605.009.A1) gzip",
                            0);
            }
        }

        const char* url = streaming ? nullptr : inputUrl.c_str();
        int rc = avformat_open_input(&fmt, url, nullptr, &opts);
        if (opts != nullptr) { av_dict_free(&opts); }
        if (rc < 0) {
            // avformat_open_input frees fmt on failure and (with custom IO) the
            // AVIOContext buffer+context too. Nothing else allocated yet.
            LOGD("FFMPEG open failed for '%s' (%d)", url ? url : "<stream>", rc);
            finished = true;
            return;
        }

        if (avformat_find_stream_info(fmt, nullptr) < 0) {
            closeInput(fmt);
            finished = true;
            return;
        }

        int audioIdx =
            av_find_best_stream(fmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (audioIdx < 0) {
            closeInput(fmt);
            finished = true;
            return;
        }

        AVStream* st = fmt->streams[audioIdx];
        const AVCodec* dec = avcodec_find_decoder(st->codecpar->codec_id);
        AVCodecContext* cc = dec ? avcodec_alloc_context3(dec) : nullptr;
        if (cc == nullptr ||
            avcodec_parameters_to_context(cc, st->codecpar) < 0 ||
            avcodec_open2(cc, dec, nullptr) < 0) {
            if (cc != nullptr) { avcodec_free_context(&cc); }
            closeInput(fmt);
            finished = true;
            return;
        }

        // Some decoders leave the channel layout unspecified; give it a sane
        // default from the channel count so the resampler is happy.
        if (cc->ch_layout.nb_channels == 0) {
            av_channel_layout_default(&cc->ch_layout, st->codecpar->ch_layout.nb_channels);
        }

        SwrContext* swr = nullptr;
        AVChannelLayout outLayout = AV_CHANNEL_LAYOUT_STEREO;
        if (swr_alloc_set_opts2(&swr, &outLayout, AV_SAMPLE_FMT_S16, kOutRate,
                                &cc->ch_layout, cc->sample_fmt, cc->sample_rate,
                                0, nullptr) < 0 ||
            swr_init(swr) < 0) {
            if (swr != nullptr) { swr_free(&swr); }
            avcodec_free_context(&cc);
            closeInput(fmt);
            finished = true;
            return;
        }

        AVPacket* pkt = av_packet_alloc();
        AVFrame* frame = av_frame_alloc();
        std::vector<int16_t> outBuf;

        bool draining = false;
        while (!stop) {
            if (!draining) {
                rc = av_read_frame(fmt, pkt);
                if (rc < 0) {
                    // EOF or a read error (incl. the interrupt on teardown).
                    // Flush the decoder to emit any buffered frames, then stop.
                    avcodec_send_packet(cc, nullptr);
                    draining = true;
                } else if (pkt->stream_index != audioIdx) {
                    av_packet_unref(pkt);
                    continue;
                } else {
                    avcodec_send_packet(cc, pkt);
                    av_packet_unref(pkt);
                }
            }

            // Drain all frames the decoder can currently produce.
            bool decoderDrained = false;
            while (!stop) {
                rc = avcodec_receive_frame(cc, frame);
                if (rc == AVERROR(EAGAIN)) { break; } // needs more input
                if (rc == AVERROR_EOF) { decoderDrained = true; break; }
                if (rc < 0) { decoderDrained = true; break; } // decode error
                emitFrame(swr, cc, frame, outBuf);
                av_frame_unref(frame);
            }
            if (draining && decoderDrained) { break; }
        }

        // Flush the resampler's internal buffer (a few tail samples).
        if (!stop) { emitFrame(swr, cc, nullptr, outBuf); }

        av_frame_free(&frame);
        av_packet_free(&pkt);
        swr_free(&swr);
        avcodec_free_context(&cc);
        closeInput(fmt);
        finished = true;
    }

    // Resample one frame (or, with frame==nullptr, flush the resampler tail) to
    // s16 stereo 44100 and push it into the PCM ring.
    void emitFrame(SwrContext* swr, AVCodecContext* /*cc*/, AVFrame* frame,
                   std::vector<int16_t>& outBuf)
    {
        int inSamples = frame ? frame->nb_samples : 0;
        int maxOut = swr_get_out_samples(swr, inSamples);
        if (maxOut <= 0) { return; }

        outBuf.resize(static_cast<size_t>(maxOut) * kOutChannels);
        auto* outPtr = reinterpret_cast<uint8_t*>(outBuf.data());
        const uint8_t** inData =
            frame ? const_cast<const uint8_t**>(frame->extended_data) : nullptr;

        int outSamples = swr_convert(swr, &outPtr, maxOut, inData, inSamples);
        if (outSamples <= 0) { return; }
        pushSamples(outBuf.data(), outSamples * kOutChannels);
    }

    // avformat_close_input(&fmt) also frees the custom AVIOContext buffer+context
    // (they hang off fmt->pb with AVFMT_FLAG_CUSTOM_IO), so nothing else to do.
    static void closeInput(AVFormatContext* fmt)
    {
        if (fmt->pb != nullptr) {
            av_freep(&fmt->pb->buffer);
            avio_context_free(&fmt->pb);
        }
        avformat_close_input(&fmt);
    }

    utils::Fifo<int16_t> pcm;
    std::string inputUrl;
    bool streaming{false};
    std::shared_ptr<utils::Fifo<uint8_t>> fifo;
    std::thread worker;
    std::atomic<bool> stop{false};
    std::atomic<bool> producerDone{false};
    std::atomic<bool> finished{false};
};

FFMPEGPlugin::FFMPEGPlugin()
{
    // One-time global init for the network protocols (http/https/tcp/tls used by
    // radio + resolved YouTube URLs). Cheap and idempotent-guarded.
    static std::once_flag once;
    std::call_once(once, [] {
        avformat_network_init();
        // Match the old CLI's "-v error": suppress libav's info/warning chatter
        // (per-frame "overread skip", "Estimating duration from bitrate", opus
        // "Could not update timestamps") so only real errors reach stderr.
        av_log_set_level(AV_LOG_ERROR);
    });
    LOGD("FFMPEG PLUGIN INITIALIZED (in-process libav)");
}

// Container/codec extensions the linked libav decodes into PCM. Kept as the
// single source of truth for both canHandle() and getSupportedExtensions() so
// the two never drift. The compressed-lossy set (mp3/aac/m4a/mp4/ogg/opus/mp2/
// mpeg/ac3) plus the lossless PCM containers (wav/flac/aiff/aif) that show up
// across the demoscene collections (demozoo has ~685 wav, ~343 flac, ~60 mp2,
// ~6 aif/aiff, ~4 opus; scene.org compos add raw .mpeg audio and Dolby .ac3) --
// all handled by libav's demuxers.
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
    // ships no 8SVX eagleplayer, but libav's IFF demuxer decodes them (incl.
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
    return new FFMPEGPlayer{fileName};
};

ChipPlayer* FFMPEGPlugin::fromStream(std::shared_ptr<utils::Fifo<uint8_t>> fifo)
{
    return new FFMPEGPlayer(std::move(fifo));
}
} // namespace musix

extern "C" void ffmpegplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& config) {
        return std::make_shared<musix::FFMPEGPlugin>();
    });
}
