#include "KSSPlugin.h"

#include "FacMus2Kss.h"

#include "../../chipplayer.h"

#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <coreutils/file.h>

// jis2unicode() + utf8_encode() for the Shift-JIS (MSX kanji) title. The
// <coreutils/...> form isn't on this plugin's include path, so reach the
// headers directly, exactly as S98Plugin does.
#include <coreutils/url.h>
#include <coreutils/utf8.h>

// Vendored libkss replayer (ISC). The embedded MGSDRV Z80 driver blob it pulls
// in (modules/drivers/mgsdrv.h) is Ain/GIGAMIX freeware, not ISC.
extern "C" {
#include "kssplay.h"
}

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace musix {

namespace {
constexpr int KSS_HZ = 44100;
constexpr int KSS_NCH = 2;
// Upper bound for the silent length pre-scan, so a non-looping / never-silent
// tune can't spin forever while we measure it.
constexpr int KSS_MAX_SECONDS = 360;
// Length reported for tunes whose driver can't flag loop/stop (MoonBlaster);
// the host uses it to fade and advance since the song never self-terminates.
constexpr int KSS_DEFAULT_SECONDS = 180;

// MoonBlaster header offsets (MBR143 / MoonBlaster 1.4): the 8-byte ADPCM
// sample-bank name lives at 0x140, the title at 0xCF.
constexpr long MBM_BANKNAME_OFF = 0x140;
constexpr int MBM_BANKNAME_LEN = 8;
constexpr long MBM_HEADER_MIN = 0x148;

// Directory part of a path including the trailing separator, "" if none.
std::string dirOf(const std::string& path)
{
    auto pos = path.find_last_of("/\\");
    if (pos == std::string::npos) { return ""; }
    return path.substr(0, pos + 1);
}

bool isMbm(const std::string& name)
{
    return utils::toLower(utils::path_extension(name)) == "mbm";
}

bool isMus(const std::string& name)
{
    return utils::toLower(utils::path_extension(name)) == "mus";
}

// Read an entire file into a byte vector; false if it can't be opened/read.
bool readWhole(const std::string& path, std::vector<uint8_t>& out)
{
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) { return false; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return false; }
    out.resize(static_cast<size_t>(sz));
    size_t got = fread(out.data(), 1, static_cast<size_t>(sz), f);
    fclose(f);
    out.resize(got);
    return got > 0;
}
} // namespace

class KSSPlayer : public ChipPlayer {
public:
    explicit KSSPlayer(const std::string& fileName)
    {
        std::vector<uint8_t> data;
        if (!readWhole(fileName, data)) {
            throw player_exception("KSS: cannot read " + fileName);
        }

        // FAC SoundTracker ".MUS" is a PSG+sampled-drum MSX tune that libkss
        // can't load as-is: convert it (and its SM1/SM2 drumkit companions from
        // the same directory) into a KSS image carrying FAC's own Z80 replay
        // routine, then hand that to the normal KSS path below. See FacMus2Kss.
        if (isMus(fileName)) {
            std::string dir = dirOf(fileName);
            fac::CompanionReader reader =
                [&dir](const std::string& n, std::vector<uint8_t>& out) {
                    return readWhole(dir + n, out);
                };
            std::vector<uint8_t> kssImage;
            if (!fac::FacMusToKss(data.data(), data.size(), reader, kssImage,
                                  facTitle_, facArtist_)) {
                throw player_exception(
                    "KSS: not a playable FAC SoundTracker .mus "
                    "(missing SM1/SM2 drumkit?)");
            }
            data.swap(kssImage); // feed the KSSX image to KSS_bin2kss below
            isFac_ = true;
        }
        // MoonBlaster references an external ADPCM bank (.mbk) by name in its
        // header. KSS_mbm2kss (reached via KSS_bin2kss) loads it through global
        // state seeded by KSS_autoload_mbk, so point that at the song's own
        // basename and directory first. The bank is optional: without it the
        // MBR143 driver patches its bank-load entry to a RET and plays bankless.
        else if (isMbm(fileName)) {
            std::string dir = dirOf(fileName);
            KSS_autoload_mbk(fileName.c_str(), dir.empty() ? "." : dir.c_str(),
                             nullptr);
        }

        // KSS_bin2kss detects the format, runs the matching converter (MGSDRV
        // here, or recognises the KSSX image we built for FAC) and fills in the
        // title metadata.
        kss_ = KSS_bin2kss(data.data(), static_cast<uint32_t>(data.size()),
                           fileName.c_str());
        if (kss_ == nullptr) {
            throw player_exception("KSS: not a recognised MSX music file");
        }

        play_ = KSSPLAY_new(KSS_HZ, KSS_NCH, 16);
        if (play_ == nullptr) {
            cleanup();
            throw player_exception("KSS: KSSPLAY_new failed");
        }
        KSSPLAY_set_data(play_, kss_);

        // High-quality resampling for every chip these formats can drive:
        // PSG/SCC/OPLL for MGS, plus OPL (Y8950 MSX-AUDIO) for MoonBlaster ADPCM.
        KSSPLAY_set_device_quality(play_, KSS_DEVICE_PSG, 1);
        KSSPLAY_set_device_quality(play_, KSS_DEVICE_SCC, 1);
        KSSPLAY_set_device_quality(play_, KSS_DEVICE_OPLL, 1);
        KSSPLAY_set_device_quality(play_, KSS_DEVICE_OPL, 1);

        int lengthSeconds = measureLength();

        // measureLength left the engine at the loop point; rewind for playback.
        KSSPLAY_reset(play_, 0, 0);

        if (isFac_) {
            // FAC text fields are ASCII Latin, not MSX Shift-JIS -- use as-is.
            setMeta("title", facTitle_, "composer", facArtist_, "length",
                    lengthSeconds, "format", "FAC SoundTracker");
        } else {
            std::string title = decodeTitle();
            setMeta("title", title, "length", lengthSeconds, "format",
                    formatName(kss_->type));
        }
    }

    ~KSSPlayer() override { cleanup(); }

    int getHZ() override { return KSS_HZ; }

    int getSamples(int16_t* target, int noSamples) override
    {
        if (play_ == nullptr) { return -1; }
        // The host counts interleaved int16 values; KSSPLAY_calc counts frames.
        uint32_t frames = static_cast<uint32_t>(noSamples / KSS_NCH);
        KSSPLAY_calc(play_, target, frames);
        // Songs without a loop point eventually raise the stop flag; end there.
        if (KSSPLAY_get_stop_flag(play_)) { return -1; }
        return static_cast<int>(frames) * KSS_NCH;
    }

    bool seekTo(int /*song*/, int /*seconds*/) override { return false; }

private:
    // Render silently (no audio mixing) until the tune reaches its loop point or
    // stops, and return that duration in seconds. This becomes the "length"
    // metadata so the host fades and advances at a musically correct boundary.
    int measureLength()
    {
        // MoonBlaster (MBR143) marks neither loop nor stop as detectable, so a
        // silent scan would never terminate early -- it would always burn the
        // full KSS_MAX_SECONDS of emulation and still report a meaningless
        // length. Skip it and hand the host a sensible default instead.
        if (kss_->loop_detectable == 0 && kss_->stop_detectable == 0) {
            return KSS_DEFAULT_SECONDS;
        }

        KSSPLAY_reset(play_, 0, 0);
        int seconds = 0;
        for (; seconds < KSS_MAX_SECONDS; ++seconds) {
            KSSPLAY_calc_silent(play_, KSS_HZ); // one second
            if (KSSPLAY_get_loop_count(play_) >= 1 ||
                KSSPLAY_get_stop_flag(play_)) {
                ++seconds; // count the second we just rendered
                break;
            }
        }
        return seconds > 0 ? seconds : KSS_MAX_SECONDS;
    }

    static const char* formatName(uint32_t type)
    {
        switch (type) {
        case MGSDATA: return "MGSDRV";
        case BGMDATA: return "MuSICA";
        case OPXDATA: return "OPLLDriver";
        case MPK103DATA:
        case MPK106DATA: return "MPK";
        case MBMDATA: return "MoonBlaster";
        default: return "MSX";
        }
    }

    std::string decodeTitle()
    {
        // kss_->title is MSX text (Shift-JIS-ish, already kanji-fixed by libkss).
        const char* raw = KSS_get_title(kss_);
        if (raw == nullptr || raw[0] == '\0') { return {}; }
        std::string sjis(raw, strnlen(raw, KSS_TITLE_MAX));
        std::string utf8 = utils::utf8_encode(
            utils::jis2unicode(reinterpret_cast<uint8_t*>(sjis.data())));
        return utf8.empty() ? sjis : utf8;
    }

    void cleanup()
    {
        if (play_ != nullptr) { KSSPLAY_delete(play_); play_ = nullptr; }
        if (kss_ != nullptr) { KSS_delete(kss_); kss_ = nullptr; }
    }

    KSS* kss_ = nullptr;
    KSSPLAY* play_ = nullptr;
    bool isFac_ = false;       // converted from a FAC SoundTracker .mus
    std::string facTitle_;     // FAC song title (ASCII), set during conversion
    std::string facArtist_;    // FAC composer (ASCII), set during conversion
};

bool KSSPlugin::canHandle(const std::string& name)
{
    std::string ext = utils::toLower(utils::path_extension(name));
    if (ext != "mgs" && ext != "bgm" && ext != "opx" && ext != "mpk" &&
        ext != "mbm" && ext != "mus") {
        return false;
    }
    // Validate the actual content with libkss's own detectors so we don't grab
    // unrelated files that merely share one of these extensions. A 64 KB header
    // covers every MSX driver's signature (these files are all small anyway).
    FILE* fp = fopen(name.c_str(), "rb");
    if (!fp) { return false; }
    std::vector<uint8_t> buf(65536);
    size_t n = fread(buf.data(), 1, buf.size(), fp);
    fclose(fp);
    if (n < 8) { return false; }
    uint8_t* d = buf.data();
    auto sz = static_cast<uint32_t>(n);
    // ".mus" is heavily overloaded (AdLib, UFO Amiga, Doom, ...). Claim only FAC
    // SoundTracker (MSX BSAVE, 16391 bytes). Gate before any libkss detector so
    // a stray 0xFE BSAVE can't be mistaken for MuSICA/.bgm.
    if (ext == "mus") { return fac::IsFacMus(d, n); }
    if (ext == "mgs") { return KSS_isMGSdata(d, sz) != 0; }
    if (ext == "bgm") { return KSS_isBGMdata(d, sz) != 0; }
    if (ext == "opx") { return KSS_isOPXdata(d, sz) != 0; }
    if (ext == "mpk") {
        return KSS_isMPK103data(d, sz) != 0 || KSS_isMPK106data(d, sz) != 0;
    }
    // MoonBlaster carries no content signature (KSS_isMBMdata is always false),
    // so libkss identifies it by the .mbm extension once nothing else matches.
    // KSS_check_type applies exactly that rule, and also rejects a misnamed file
    // that is really one of the signatured formats.
    return KSS_check_type(d, sz, name.c_str()) == MBMDATA; // mbm
}

std::set<std::string> KSSPlugin::getSupportedExtensions() const
{
    return {"mgs", "bgm", "opx", "mpk", "mbm", "mus"};
}

std::vector<std::string> KSSPlugin::getSecondaryFiles(const std::string& name)
{
    // A drummed FAC SoundTracker .mus needs its two drumkit sample banks,
    // <DRUMKIT>.SM1 and .SM2, named (8 chars) in the song header. Modland stores
    // them UPPERCASE next to the song; the host fetches by exact name. A
    // "NO DRUMS" song needs none (base name comes back empty).
    if (isMus(name)) {
        std::vector<uint8_t> data;
        if (!readWhole(name, data) || !fac::IsFacMus(data.data(), data.size())) {
            return {};
        }
        std::string base = fac::FacDrumkitBaseName(data.data(), data.size());
        if (base.empty()) { return {}; }
        return {base + ".SM1", base + ".SM2"};
    }

    // MoonBlaster's only companion is its .mbk ADPCM sample bank, named (8 chars,
    // space-padded) in the header at 0x140. Emit "<NAME>.MBK" preserving the
    // header's case: Modland stores every MoonBlaster bank UPPERCASE, and the
    // host fetches secondary files by exact name -- a case mismatch 404s. The
    // bank is optional (the driver plays bankless without it), so we deliberately
    // do NOT emit a <song-basename>.mbk guess: it almost never exists on Modland
    // and a missing companion needlessly degrades the load.
    if (!isMbm(name)) { return {}; }

    FILE* f = fopen(name.c_str(), "rb");
    if (f == nullptr) { return {}; }
    std::vector<uint8_t> hdr(MBM_HEADER_MIN);
    size_t got = fread(hdr.data(), 1, hdr.size(), f);
    fclose(f);
    if (got < MBM_HEADER_MIN) { return {}; }

    std::string bankName(reinterpret_cast<const char*>(&hdr[MBM_BANKNAME_OFF]),
                         MBM_BANKNAME_LEN);
    auto endp = bankName.find_first_of(" \0", 0, 2);
    if (endp != std::string::npos) { bankName.resize(endp); }
    if (bankName.empty()) { return {}; }
    return {bankName + ".MBK"};
}

ChipPlayer* KSSPlugin::fromFile(const std::string& fileName)
{
    return new KSSPlayer{fileName};
}

} // namespace musix

extern "C" void kssplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::KSSPlugin>();
    });
}
