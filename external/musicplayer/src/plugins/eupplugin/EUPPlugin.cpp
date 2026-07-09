#include "EUPPlugin.h"

#include "../../chipplayer.h"
#include <coreutils/utils.h>
#include <coreutils/file.h>
#include <coreutils/log.h>

// Vendored eupmini replayer (GPLv2).
#include "eupplayer.hpp"
#include "eupplayer_towns.hpp"
#include "eupplayer_townsEmulator.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace musix {

// The eupmini emulator hands audio off through this single global ring buffer,
// which is normally drained by an SDL callback thread.  The CLI (eupplay.cpp)
// owns the definition; since we don't compile that translation unit, we must
// provide it here.  Global state means one .eup plays at a time — exactly the
// ChipMachine model.
} // namespace musix

struct pcm_struct pcm;

namespace musix {

static const int EUP_RATE = streamAudioRate; // 44100, fixed by eupmini

// 2 KB Euphony header, byte-for-byte (offsets verified against eupplay.cpp).
// The event stream begins at 0x806, right after this struct.
namespace {
struct EUPHeaderRaw {
    char title[32];             // 0x000
    char artist[8];             // 0x020
    char dummy[44];             // 0x028
    char trk_name[32][16];      // 0x054
    char short_trk_name[32][8]; // 0x254
    char trk_mute[32];          // 0x354
    char trk_port[32];          // 0x374
    char trk_midi_ch[32];       // 0x394
    char trk_key_bias[32];      // 0x3B4
    char trk_transpose[32];     // 0x3D4
    char trk_play_filter[32][7];// 0x3F4
    char instruments_name[128][4]; // 0x4D4
    char fm_midi_ch[6];         // 0x6D4
    char pcm_midi_ch[8];        // 0x6DA
    char fm_file_name[8];       // 0x6E2
    char pcm_file_name[8];      // 0x6EA
    char reserved[260];         // 0x6F2
    char appli_name[8];         // 0x7F6
    char appli_version[2];      // 0x7FE
    int32_t size;               // 0x800
    char signature;             // 0x804
    char first_tempo;           // 0x805
};
// The struct rounds up to 2056 bytes (trailing int32 alignment padding); what
// matters is that each field lands at its verified header offset.
static_assert(offsetof(EUPHeaderRaw, trk_midi_ch) == 0x394, "trk_midi_ch offset");
static_assert(offsetof(EUPHeaderRaw, fm_midi_ch) == 0x6D4, "fm_midi_ch offset");
static_assert(offsetof(EUPHeaderRaw, fm_file_name) == 0x6E2, "fm_file_name offset");
static_assert(offsetof(EUPHeaderRaw, pcm_file_name) == 0x6EA, "pcm_file_name offset");
static_assert(offsetof(EUPHeaderRaw, first_tempo) == 0x805, "first_tempo offset");

constexpr int EUP_HEADER_SIZE = 0x806; // 2048 + 6

// Look up the directory part of a path (including trailing separator), "" if none.
std::string dirOf(const std::string& path)
{
    auto pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return "";
    return path.substr(0, pos + 1);
}

// Read an entire file into a byte vector; returns false if it can't be opened.
bool readWhole(const std::string& path, std::vector<uint8_t>& out)
{
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return false; }
    out.resize((size_t)sz);
    size_t got = fread(out.data(), 1, (size_t)sz, f);
    fclose(f);
    out.resize(got);
    return got > 0;
}

// Instrument banks come from varied sources (Modland is lowercase; the eupmini
// sample set is UPPERCASE), so try the header-cased name, lowercased, and both
// extension cases before giving up.
bool readBankFile(const std::string& dir, const std::string& base,
                  const std::string& ext, std::vector<uint8_t>& out)
{
    std::string lo = utils::toLower(base);
    std::string EXT = ext; for (auto& c : EXT) c = (char)toupper((unsigned char)c);
    for (const std::string& nm : {base + "." + ext, lo + "." + ext,
                                  base + "." + EXT, lo + "." + EXT}) {
        if (readWhole(dir + nm, out)) return true;
    }
    return false;
}

// Trim a fixed 8-byte header name field to a C string.
std::string trim8(const char name8[8])
{
    return std::string(name8, strnlen(name8, 8));
}
} // namespace

class EUPPlayer_ : public ChipPlayer {
public:
    explicit EUPPlayer_(const std::string& fileName)
    {
        if (!readWhole(fileName, eup_) || eup_.size() < EUP_HEADER_SIZE + 6) {
            throw player_exception("EUP: file too short");
        }
        const auto* hdr = reinterpret_cast<const EUPHeaderRaw*>(eup_.data());

        dev_ = new EUP_TownsEmulator();
        dev_->outputSampleUnsigned(false);
        dev_->outputSampleSize(2);
        dev_->outputSampleChannels(2);
        dev_->outputSampleLSBFirst(hostIsLittleEndian());
        dev_->rate(EUP_RATE);

        player_ = new EUPPlayer();
        player_->outputDevice(dev_);

        // Track -> MIDI channel map, and FM/PCM device channel assignment.
        for (int t = 0; t < 32; t++)
            player_->mapTrack_toChannel(t, hdr->trk_midi_ch[t]);
        for (int t = 0; t < 6; t++)
            dev_->assignFmDeviceToChannel(hdr->fm_midi_ch[t]);
        for (int t = 0; t < 8; t++)
            dev_->assignPcmDeviceToChannel(hdr->pcm_midi_ch[t]);

        loadFmBank(dirOf(fileName), hdr->fm_file_name);
        loadPcmBank(dirOf(fileName), hdr->pcm_file_name);

        player_->tempo(hdr->first_tempo + 30);

        std::memset(&pcm, 0, sizeof(pcm));
        pcm.on = true;
        player_->startPlaying(eup_.data() + EUP_HEADER_SIZE);

        // Title is CP932 (Shift-JIS) in the header; pass the trimmed raw bytes.
        std::string title(hdr->title, strnlen(hdr->title, sizeof(hdr->title)));
        setMeta("title", title, "format", "Euphony");
    }

    ~EUPPlayer_() override
    {
        if (player_) { player_->stopPlaying(); delete player_; }
        delete dev_;
    }

    int getHZ() override { return EUP_RATE; }

    int getSamples(int16_t* target, int noSamples) override
    {
        const int framesWanted = noSamples / 2;
        int framesGot = 0;

        while (framesGot < framesWanted) {
            // Serve any frames left over from the previous tick first.
            if (residPos_ < resid_.size()) {
                int avail = (int)(resid_.size() - residPos_) / 2;
                int take = framesWanted - framesGot;
                if (take > avail) take = avail;
                std::memcpy(target + framesGot * 2,
                            resid_.data() + residPos_,
                            (size_t)take * 2 * sizeof(int16_t));
                residPos_ += (size_t)take * 2;
                framesGot += take;
                continue;
            }
            resid_.clear();
            residPos_ = 0;

            if (!player_ || !player_->isPlaying()) break;

            // Render exactly one sequencer tick.  Reset the ring so the producer
            // sees a full empty buffer (it never blocks) and writes from frame 0
            // without wrapping; pcm.write_pos then equals the frames produced.
            pcm.write_pos = 0;
            pcm.read_pos = streamAudioSamplesBuffer - 1;
            pcm.on = true;
            player_->nextTick();

            int produced = pcm.write_pos; // frames written this tick
            if (produced <= 0) continue;
            if (produced > streamAudioSamplesBuffer) produced = streamAudioSamplesBuffer;

            resid_.assign(pcm.buffer, pcm.buffer + (size_t)produced * 2);
            residPos_ = 0;
        }

        if (framesGot == 0) return -1;
        return framesGot * 2;
    }

    bool seekTo(int, int) override { return false; }

private:
    static bool hostIsLittleEndian()
    {
        uint16_t x = 1;
        return *reinterpret_cast<uint8_t*>(&x) == 1;
    }

    void loadFmBank(const std::string& dir, const char name8[8])
    {
        std::string base(name8, strnlen(name8, 8));
        if (base.empty()) return;
        std::vector<uint8_t> bank;
        if (!readBankFile(dir, base, "fmb", bank)) {
            LOGD("EUP: FM bank {}.fmb not found", base);
            return;
        }
        // 8-byte header, then 48 bytes per FM instrument.
        if (bank.size() <= 8) return;
        int count = (int)((bank.size() - 8) / 48);
        for (int n = 0; n < count; n++)
            dev_->setFmInstrumentParameter(n, bank.data() + 8 + 48 * n);
    }

    void loadPcmBank(const std::string& dir, const char name8[8])
    {
        std::string base(name8, strnlen(name8, 8));
        if (base.empty()) return;
        if (!readBankFile(dir, base, "pmb", pmb_)) {
            LOGD("EUP: PCM bank {}.pmb not found", base);
            return;
        }
        dev_->setPcmInstrumentParameters(pmb_.data(), pmb_.size());
    }

    EUPPlayer* player_ = nullptr;
    EUP_TownsEmulator* dev_ = nullptr;
    std::vector<uint8_t> eup_; // kept alive: player reads events in place
    std::vector<uint8_t> pmb_; // kept alive: emulator references sample data
    std::vector<int16_t> resid_;
    size_t residPos_ = 0;
};

bool EUPPlugin::canHandle(const std::string& name)
{
    return utils::toLower(utils::path_extension(name)) == "eup";
}

std::set<std::string> EUPPlugin::getSupportedExtensions() const
{
    return {"eup"};
}

std::vector<std::string> EUPPlugin::getSecondaryFiles(const std::string& name)
{
    // Read just the header region and return the .fmb/.pmb bank filenames so the
    // host can fetch them next to the .eup (e.g. from Modland). Names are emitted
    // lowercase to match Modland's filenames; the loader also tries other cases.
    FILE* f = fopen(name.c_str(), "rb");
    if (!f) return {};
    std::vector<uint8_t> hdr(EUP_HEADER_SIZE);
    size_t got = fread(hdr.data(), 1, hdr.size(), f);
    fclose(f);
    if (got < EUP_HEADER_SIZE) return {};

    const auto* h = reinterpret_cast<const EUPHeaderRaw*>(hdr.data());
    std::vector<std::string> banks;
    std::string fm = utils::toLower(trim8(h->fm_file_name));
    std::string pcm = utils::toLower(trim8(h->pcm_file_name));
    if (!fm.empty()) banks.push_back(fm + ".fmb");
    if (!pcm.empty()) banks.push_back(pcm + ".pmb");
    return banks;
}

ChipPlayer* EUPPlugin::fromFile(const std::string& fileName)
{
    return new EUPPlayer_{fileName};
}

} // namespace musix

extern "C" void eupplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::EUPPlugin>();
    });
}
