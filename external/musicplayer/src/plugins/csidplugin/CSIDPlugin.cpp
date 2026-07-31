#include "CSIDPlugin.h"

#include <coreutils/file.h>
#include <coreutils/utils.h>

extern "C" {
#include "csid/csid_engine.h"
}

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace musix {

namespace {

// cSID keeps its C64 memory map, CPU registers and per-voice DSP state in
// file-scope globals, exactly as VICE does -- so, like vicepluginbridge before
// it, the engine is a process-wide singleton. One song plays at a time, but
// MusicPlayerList can hold a freshly-constructed player while the previous one
// is still alive, so guard the engine and let whichever player renders next
// take ownership by reloading its own tune. Reloading restarts that tune from
// the top, which only ever happens in the (already audible) hand-off case.
std::mutex engineMutex;
void const* engineOwner = nullptr;

// cSID's fixed OUTPUT_SCALEDOWN leaves it hotter than VICE/reSID on the same
// tune. Measured over testmus/libvice with both engines rendering 10s at 44.1k,
// cSID/VICE RMS came out at 1.83, 1.84, 1.81, 1.69 and 2.17 -- median ~1.83.
// The catalog's SID tunes have sounded at VICE's level for years and sit in
// shuffle next to ~60 other plugins, so match that level rather than ship a
// step-change in loudness. Applied to the mono sample before it is duplicated
// to stereo.
constexpr int kGainNum = 55; // 0.55 ~= 1/1.83
constexpr int kGainDen = 100;

// How long an RSID that installs its own IRQ handler gets to make a sound
// before we call it unplayable, and how loud "a sound" has to be. See
// CSIDPlayer::getSamples().
//
// The threshold is NOT "any non-zero sample". Sampling real HVSC RSIDs, cSID's
// failures split two ways: most render exact digital silence, but some render a
// tiny DC-ish residue instead -- measured peaks of 9 and 54 against int16 full
// scale, i.e. around -70 dBFS and inaudible. An exact-zero test passes those
// straight through as dead air, which is the exact outcome this probe exists to
// avoid. Tunes that genuinely play are nowhere near: the same sampling put them
// at peaks of 3969 and 9240, so 64 (~-54 dBFS) separates the two cleanly with
// two orders of magnitude of headroom.
constexpr int kSilenceProbeFrames = 44100 * 3;
constexpr int kSilencePeak = 64;

bool hasSidMagic(std::vector<uint8_t> const& d)
{
    return d.size() >= 0x7C &&
           (memcmp(d.data(), "PSID", 4) == 0 || memcmp(d.data(), "RSID", 4) == 0);
}

// An RSID that installs its own IRQ/NMI handler: "RSID" magic with a header play
// address of $0000 (bytes 0x0C-0x0D, big-endian). This is the shape cSID largely
// cannot play -- see the note in getSamples(); measured 2 of 14 audible.
bool isSelfIrqRsid(std::vector<uint8_t> const& d)
{
    return d.size() >= 0x7C && memcmp(d.data(), "RSID", 4) == 0 &&
           (d[0x0C] * 256 + d[0x0D]) == 0;
}

} // namespace

class CSIDPlayer : public ChipPlayer
{
public:
    CSIDPlayer(std::vector<uint8_t> data, std::string const& fileName)
        : fileData(std::move(data))
    {
        std::lock_guard<std::mutex> lk{ engineMutex };
        if (csid_load(fileData.data(), static_cast<int>(fileData.size()), 44100,
                      &info) != 0) {
            throw player_exception("cSID: not a SID file");
        }
        engineOwner = this;
        subtune = info.default_subtune;
        csid_init_tune(subtune);

        // Parity with the VICE bridge this replaces: report the subtune count
        // and default only. Titles and composers come from the catalog, which
        // is curated well beyond what the 32-byte PSID header carries.
        setMeta("songs", static_cast<uint32_t>(info.subtune_count));
        setMeta("startSong", static_cast<uint32_t>(info.default_subtune));

        resetSilenceProbe();
        (void)fileName;
    }

    int getSamples(int16_t* target, int size) override
    {
        int const frames = size / 2;
        if (frames <= 0) { return 0; }
        if (static_cast<int>(mono.size()) < frames) { mono.resize(frames); }

        {
            std::lock_guard<std::mutex> lk{ engineMutex };
            if (engineOwner != this) {
                // Another player took the engine; reclaim it with our tune.
                if (csid_load(fileData.data(),
                              static_cast<int>(fileData.size()), 44100,
                              &info) != 0) {
                    return -1;
                }
                csid_init_tune(subtune);
                engineOwner = this;
            }
            csid_render(mono.data(), frames);
        }

        // An RSID whose header play address is 0 installs its own IRQ/NMI
        // handler and expects a genuine C64 (KERNAL banked in, CIA and raster
        // running). cSID does not emulate enough of that environment, so most
        // such tunes never make a sound.
        //
        // This is NOT a rare edge case, and earlier notes in this project that
        // called it one were wrong -- they were drawn from two files that turned
        // out to be PSID, so no RSID was actually being tested. Measured over 14
        // real RSIDs sampled from HVSC: ALL 14 carry play=$0000, and only 2 of
        // them produce audible output under cSID. Playing dead air until the
        // user notices is worse than moving on, so report the track unplayable
        // once we are confident.
        //
        // Still scoped exactly to the failing shape -- RSID *and* play=$0000 --
        // so an ordinary PSID that opens with several seconds of quiet can never
        // trip it.
        if (silenceProbeActive) {
            for (int i = 0; i < frames; i++) {
                if (std::abs(static_cast<int>(mono[i])) > kSilencePeak) {
                    silenceProbeActive = false;
                    break;
                }
            }
            if (silenceProbeActive) {
                framesRendered += frames;
                if (framesRendered >= kSilenceProbeFrames) { return -1; }
            }
        }

        for (int i = 0; i < frames; i++) {
            auto v = static_cast<int16_t>(mono[i] * kGainNum / kGainDen);
            target[i * 2] = v;
            target[i * 2 + 1] = v;
        }
        return frames * 2;
    }

    bool seekTo(int song, int /*seconds*/) override
    {
        if (song < 0) { return false; }
        std::lock_guard<std::mutex> lk{ engineMutex };
        if (engineOwner != this) {
            if (csid_load(fileData.data(), static_cast<int>(fileData.size()),
                          44100, &info) != 0) {
                return false;
            }
            engineOwner = this;
        }
        subtune = song;
        csid_init_tune(subtune);
        resetSilenceProbe();
        return true;
    }

    ~CSIDPlayer() override
    {
        std::lock_guard<std::mutex> lk{ engineMutex };
        if (engineOwner == this) { engineOwner = nullptr; }
    }

private:
    void resetSilenceProbe()
    {
        framesRendered = 0;
        silenceProbeActive = info.is_rsid != 0 && info.play_address == 0;
    }

    std::vector<uint8_t> fileData;
    std::vector<int16_t> mono;
    csid_info info{};
    int subtune = 0;
    int framesRendered = 0;
    bool silenceProbeActive = false;
};

static const std::set<std::string> supported_ext{ "sid", "rsid" };

bool CSIDPlugin::canHandle(const std::string& name)
{
    if (supported_ext.count(utils::path_extension(utils::toLower(name))) == 0) {
        return false;
    }
    // Content-gate rather than trust the extension: Amiga SidMon tunes are also
    // called ".sid" and are not C64 files at all. Without the magic check cSID
    // would read their first bytes as a PSID header and play noise; declining
    // lets the registration order fall through to whichever plugin owns them.
    utils::File f{ name };
    if (!f.exists()) { return false; }
    auto data = f.readAll();
    if (!hasSidMagic(data)) { return false; }
#ifndef CM_NO_VICE
    // Hand self-IRQ RSIDs back to VICE where VICE exists (the plus build).
    // cSID plays only ~2 in 14 of them; VICE plays them all, and it is still
    // linked here for .mus/.str anyway, so there is no reason for the plus build
    // to lose ~2.6k C64 tunes to an engine swap it made for licensing reasons.
    // VicePlugin::canHandle accepts exactly this shape -- keep the two in step.
    //
    // In the mas build (CM_NO_VICE) there is no fallback, so cSID keeps them:
    // the minority that do play still play, and the rest are reported unplayable
    // by the silence probe rather than sitting there as dead air.
    if (isSelfIrqRsid(data)) { return false; }
#endif
    return true;
}

std::set<std::string> CSIDPlugin::getSupportedExtensions() const
{
    // .rsid is the "real C64" variant of the same container; csid_load accepts
    // its "RSID" magic alongside "PSID".
    return supported_ext;
}

ChipPlayer* CSIDPlugin::fromFile(const std::string& fileName)
{
    auto data = utils::File(fileName).readAll();
    if (!hasSidMagic(data)) {
        throw player_exception("cSID: not a SID file");
    }
    return new CSIDPlayer{ std::move(data), fileName };
}

} // namespace musix

extern "C" void csidplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& /*config*/) {
        return std::make_shared<musix::CSIDPlugin>();
    });
}
