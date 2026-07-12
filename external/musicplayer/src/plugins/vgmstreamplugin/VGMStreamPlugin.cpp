#include "VGMStreamPlugin.h"
#include "../../chipplayer.h"
#include <coreutils/utils.h>
#include <set>
#include <string>
#include <vector>
#include <algorithm>

extern "C" {
#include <libvgmstream.h>
#include <libvgmstream_streamfile.h>
}

namespace musix {

class VGMStreamPlayer : public ChipPlayer
{
public:
    explicit VGMStreamPlayer(const std::string& fileName) : fileName(fileName), lib(nullptr)
    {
        libvgmstream_config_t cfg;
        memset(&cfg, 0, sizeof(cfg));
        cfg.auto_downmix_channels = 2; // downmix to stereo if > 2 channels
        cfg.force_sfmt = LIBVGMSTREAM_SFMT_PCM16;
        cfg.loop_count = 2.0;
        cfg.fade_time = 4.0;

        libstreamfile_t* sf = libstreamfile_open_from_stdio(fileName.c_str());
        if (!sf) {
            throw player_exception("vgmstream: could not open file");
        }

        lib = libvgmstream_create(sf, 0, &cfg);
        libstreamfile_close(sf); // safe to close/free sf after create

        if (!lib) {
            throw player_exception("vgmstream: could not initialize stream");
        }

        int sample_rate = lib->format->sample_rate;
        int length = 0;
        if (sample_rate > 0) {
            length = static_cast<int>(lib->format->play_samples / sample_rate);
        }
        setMeta("length", length,
                "format", lib->format->meta_name,
                "sub_title", lib->format->stream_name,
                "songs", lib->format->subsong_count);
    }

    ~VGMStreamPlayer() override
    {
        if (lib) {
            libvgmstream_free(lib);
            lib = nullptr;
        }
    }

    // The host (GUI) plays our samples straight into a fixed 44100 Hz device and
    // does NOT consult getHZ() -- every plugin must emit 44100. vgmstream has no
    // output-resampling option (it only decodes at the file's native rate), so we
    // decode natively and linearly resample to 44100 here, the same arrangement as
    // the SoundSmith plugin. (getHZ() still returns 44100 for the CLI path, which
    // does honor it -- reporting native there would double-resample.)
    static constexpr int OUTPUT_HZ = 44100;

    int getHZ() override { return OUTPUT_HZ; }

    int getSamples(int16_t* target, int size) override
    {
        int frames = size / 2; // target holds `size` int16 as stereo frames
        int produced = 0;

        // Prime the two native frames we interpolate between.
        if (!primed_) {
            if (!nextNative(s0l_, s0r_)) { srcEnded_ = true; }
            if (!nextNative(s1l_, s1r_)) { s1l_ = s0l_; s1r_ = s0r_; srcEnded_ = true; }
            primed_ = true;
        }

        const double step =
            static_cast<double>(lib->format->sample_rate) / OUTPUT_HZ; // native frames per output frame
        for (int i = 0; i < frames; ++i) {
            if (srcEnded_) break;
            double l = s0l_ + static_cast<double>(s1l_ - s0l_) * phase_;
            double r = s0r_ + static_cast<double>(s1r_ - s0r_) * phase_;
            target[produced++] = clamp16(l);
            target[produced++] = clamp16(r);

            phase_ += step;
            while (phase_ >= 1.0) {
                phase_ -= 1.0;
                s0l_ = s1l_;
                s0r_ = s1r_;
                int16_t nl = 0, nr = 0;
                if (nextNative(nl, nr)) { s1l_ = nl; s1r_ = nr; }
                else { srcEnded_ = true; }
            }
        }

        // Signal SONG_END (so the playlist advances) once fully drained.
        if (produced == 0 && srcEnded_) return -1;
        return produced;
    }

    bool seekTo(int song, int seconds) override
    {
        if (song >= 0) {
            // Reopen the stream with the selected subsong
            libvgmstream_close_stream(lib);
            libstreamfile_t* sf = libstreamfile_open_from_stdio(fileName.c_str());
            if (!sf) return false;
            
            libvgmstream_config_t cfg;
            memset(&cfg, 0, sizeof(cfg));
            cfg.auto_downmix_channels = 2;
            cfg.force_sfmt = LIBVGMSTREAM_SFMT_PCM16;
            cfg.loop_count = 2.0;
            cfg.fade_time = 4.0;
            libvgmstream_setup(lib, &cfg);

            int rc = libvgmstream_open_stream(lib, sf, song + 1);
            libstreamfile_close(sf);
            if (rc < 0) return false;

            int sample_rate = lib->format->sample_rate;
            int length = 0;
            if (sample_rate > 0) {
                length = static_cast<int>(lib->format->play_samples / sample_rate);
            }
            setMeta("length", length,
                    "format", lib->format->meta_name,
                    "sub_title", lib->format->stream_name,
                    "song", static_cast<uint32_t>(song));
            resetResampler();
        }
        if (seconds >= 0) {
            int64_t sample = static_cast<int64_t>(seconds) * lib->format->sample_rate;
            libvgmstream_seek(lib, sample);
            resetResampler();
        }
        return true;
    }

private:
    static int16_t clamp16(double v)
    {
        if (v > 32767.0) return 32767;
        if (v < -32768.0) return -32768;
        return static_cast<int16_t>(v);
    }

    // Clear the native decode buffer and linear-resampler state (after seek/subsong
    // change) so playback restarts cleanly from the new position.
    void resetResampler()
    {
        decFill_ = 0;
        decPos_ = 0;
        decDone_ = false;
        primed_ = false;
        srcEnded_ = false;
        phase_ = 0.0;
    }

    // Refill decBuf_ with one chunk of native-rate stereo frames. Mono sources are
    // duplicated to stereo, >2ch are already downmixed to stereo by auto_downmix.
    // NB: libvgmstream_fill() fully fills the requested buffer except at EOF, and
    // returns a result code (<0 = error); the real frame count is decoder->buf_samples.
    bool refillNative()
    {
        if (decDone_) return false;
        constexpr int WANT = 4096;

        if (lib->format->channels == 1) {
            if (static_cast<int>(monoBuf_.size()) < WANT) monoBuf_.resize(WANT);
            if (static_cast<int>(decBuf_.size()) < WANT * 2) decBuf_.resize(WANT * 2);
            int rc = libvgmstream_fill(lib, monoBuf_.data(), WANT);
            if (rc < 0) { decDone_ = true; return false; }
            int n = lib->decoder->buf_samples;
            if (lib->decoder->done) decDone_ = true;
            if (n <= 0) return false;
            for (int i = 0; i < n; ++i) {
                decBuf_[i * 2] = monoBuf_[i];
                decBuf_[i * 2 + 1] = monoBuf_[i];
            }
            decFill_ = n;
            decPos_ = 0;
            return true;
        }

        if (static_cast<int>(decBuf_.size()) < WANT * 2) decBuf_.resize(WANT * 2);
        int rc = libvgmstream_fill(lib, decBuf_.data(), WANT);
        if (rc < 0) { decDone_ = true; return false; }
        int n = lib->decoder->buf_samples;
        if (lib->decoder->done) decDone_ = true;
        if (n <= 0) return false;
        decFill_ = n;
        decPos_ = 0;
        return true;
    }

    // Pull one native-rate stereo frame; false at end of stream.
    bool nextNative(int16_t& l, int16_t& r)
    {
        while (decPos_ >= decFill_) {
            if (!refillNative()) return false;
        }
        l = decBuf_[decPos_ * 2];
        r = decBuf_[decPos_ * 2 + 1];
        ++decPos_;
        return true;
    }

    std::string fileName;
    libvgmstream_t* lib;

    // native-rate decode buffer (interleaved stereo) + cursor
    std::vector<int16_t> decBuf_;
    std::vector<int16_t> monoBuf_;
    int decFill_ = 0;
    int decPos_ = 0;
    bool decDone_ = false;

    // linear resampler state (native rate -> 44100)
    double phase_ = 0.0;
    int16_t s0l_ = 0, s0r_ = 0, s1l_ = 0, s1r_ = 0;
    bool primed_ = false;
    bool srcEnded_ = false;
};

// Declining extensions claimed by other plugins:
static const std::set<std::string> excluded_extensions = {
    "vgm", "vgz", "kss", "ay", "gbs", "gym", "hes", "nsf", "nsfe", "sap", "sgc", "spc",
    "2sf", "mini2sf", "gsf", "minigsf", "usf", "miniusf",
    "mod", "xm", "it", "s3m", "stm", "med", "mtm", "669", "dsm", "far", "amf", "okt", "ptm", "umx",
    "mp3", "wav", "ogg", "opus", "flac", "aac", "m4a", "mp4", "aiff", "aif",
    "sid", "eup", "s98", "sunvox", "org", "dmf", "cop", "sks", "ned", "mon", "uni", "ftm", "sng",
    "bbsong", "soundsmith", "ixs", "musx", "coco", "mgt", "pac", "mxtx", "mad", "jxs",
    "pt3", "pt2", "stc", "stp", "sqt", "psc", "psm", "pt1", "ftc", "rsn"
};

bool VGMStreamPlugin::canHandle(const std::string& name)
{
    auto ext = utils::path_extension(name);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (excluded_extensions.count(ext) > 0) {
        return false;
    }

    // Fast extension reject first: libvgmstream_is_valid() only matches the
    // extension against vgmstream's list -- it never reads the file -- so this
    // cheaply rules out anything vgmstream doesn't claim at all.
    libvgmstream_valid_t vcfg;
    memset(&vcfg, 0, sizeof(vcfg));
    vcfg.accept_common = false;
    vcfg.reject_extensionless = true;
    if (!libvgmstream_is_valid(name.c_str(), &vcfg)) {
        return false;
    }

    // Many of vgmstream's ~700 extensions are shared with unrelated formats (e.g.
    // a CRI ADX vs. some game's proprietary sequenced ".adx"). Since is_valid only
    // checks the extension, actually open the file with vgmstream to confirm the
    // content is decodable -- otherwise fromFile() would throw on a look-alike and
    // the host would report a playback error instead of skipping it gracefully.
    libstreamfile_t* sf = libstreamfile_open_from_stdio(name.c_str());
    if (!sf) {
        return false;
    }
    libvgmstream_t* probe = libvgmstream_create(sf, 0, nullptr);
    libstreamfile_close(sf);
    if (!probe) {
        return false;
    }
    libvgmstream_free(probe);
    return true;
}

std::set<std::string> VGMStreamPlugin::getSupportedExtensions() const
{
    std::set<std::string> extensions;
    int size = 0;
    const char** exts = libvgmstream_get_extensions(&size);
    if (exts) {
        for (int i = 0; i < size; ++i) {
            std::string ext = exts[i];
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (excluded_extensions.count(ext) == 0) {
                extensions.insert(ext);
            }
        }
    }
    return extensions;
}

ChipPlayer* VGMStreamPlugin::fromFile(const std::string& fileName)
{
    return new VGMStreamPlayer{fileName};
}

} // namespace musix

extern "C" void vgmstreamplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::VGMStreamPlugin>();
    });
}
