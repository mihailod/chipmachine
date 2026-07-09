
#include "UADEPlugin.h"

#include "../../chipplayer.h"

#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <coreutils/file.h>

#include <uade/uade.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <set>
#include <thread>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

static std::thread uadeThread;
extern "C" void uade_run_thread(void (*f)(void*), void* data)
{
    LOGD("Starting thread");
    uadeThread = std::thread(f, data);
}

extern "C" void uade_wait_thread()
{
    uadeThread.join();
}

// Returns the lowercased replay filename embedded in a YMST header, or empty.
static std::string ymstReplayName(fs::path const& path)
{
    utils::File f{path.string()};
    if (!f.exists()) { return {}; }
    auto data = f.readAll();
    static const std::string suffix = "_replay";
    for (size_t i = 0; i + 3 < data.size(); i++) {
        if (data[i] == 'Y' && data[i + 1] == 'M' && data[i + 2] == '.') {
            size_t end = i + 3;
            while (end < data.size() && data[end] != '\0') { end++; }
            std::string name(data.begin() + i, data.begin() + end);
            if (name.size() > suffix.size() &&
                name.substr(name.size() - suffix.size()) == suffix) {
                utils::makeLower(name);
                return name;
            }
        }
    }
    return {};
}

// True if the file is an IFF-SMUS (Aegis Sonix) score: "FORM" .... "SMUS".
//
// A SMUS score carries only sequence data; each track's timbre is an external
// instrument referenced by name from a sibling "Instruments/" subdirectory. The
// member names are unpredictable -- a "<name>.instr" descriptor is either a
// self-contained "Synthesis" waveform or a "SampledSound" pointing at a raw
// "<sample>.ss" whose name need not match the instrument's and whose case is
// inconsistent on modland -- so rather than guess them we fetch the whole
// directory (see getSecondaryFiles). The UADE SonixMusicDriver replay then loads
// whatever it needs from the score's own directory at play time.
static bool isIffSmus(fs::path const& path)
{
    utils::File f{path.string()};
    if (!f.exists()) { return false; }
    auto data = f.readAll();
    return data.size() >= 12 && std::memcmp(data.data(), "FORM", 4) == 0 &&
           std::memcmp(data.data() + 8, "SMUS", 4) == 0;
}

namespace musix {

class UADEPlayer : public ChipPlayer
{
public:
    explicit UADEPlayer(const fs::path& _dataDir) : dataDir(_dataDir) {}

    // Called when Amiga wants to load a file
    static struct uade_file* amigaloader(const char* name,
                                         const char* playerdir, void* context,
                                         struct uade_state* state)
    {
        LOGD("Trying to load '%s' from '%s'", name, playerdir);
        auto* player = static_cast<UADEPlayer*>(context);

        fs::path fileName = name;

        auto ext = fileName.extension();

        if (utils::startsWith(name, "Env:")) {
            fileName = fs::path(playerdir) / "ENV" / &name[4];
        } else if (utils::startsWith(name, "smpl.")) {
            fileName = player->loadDir / (player->baseName + ".smpl");
        } else if (utils::endsWith(fileName.string(), "SMPL.set")) {
            // Shared TFMX sample bank. The replay asks for "SMPL.set"; archives
            // name it either "smpl.set" (UnExoticA -- matches the request
            // case-insensitively) or the swapped "set.smpl". Prefer the
            // as-requested file when it exists, else fall back to "set.smpl".
            fs::path shared = player->loadDir / fileName.filename();
            fileName =
                fs::exists(shared) ? shared : player->loadDir / "set.smpl";
        } else if (!player->uadeFile.empty()) {
            fileName = player->loadDir /
                       (player->baseName + "." + utils::path_prefix(fileName));
            LOGD("Translated back to '%s'", fileName.string().c_str());
        } else if (player->currentFileName.string().find(fileName.string()) ==
                   0) {
            LOGD("Restoring filename %s back to '%s'", fileName.string().c_str(),
                 player->currentFileName.string().c_str());
            fileName = player->currentFileName;
        }

        LOGD("Actually loading %s", fileName.string().c_str());
        struct uade_file* f =
            uade_load_amiga_file(fileName.string().c_str(), playerdir, state);

        // IFF-SMUS resilience: many modland SMUS rips are missing a raw ".ss"
        // sample that a "SampledSound" instrument references (e.g. SLL's
        // Never_give_up references orchestra1.ss, which 404s on the server).
        // The Sonix driver renders the WHOLE score silent when one sample fails
        // to load; hand it a short silent placeholder instead so the remaining
        // instruments still play. Gated to SMUS scores + ".ss" requests so it
        // can't mask genuine load failures in other formats.
        if (f == nullptr && player->isSmus &&
            utils::toLower(fileName.extension().string()) == ".ss") {
            static const std::vector<uint8_t> silence(4096, 0);
            LOGD("Missing .ss '%s' -> silent placeholder",
                 fileName.string().c_str());
            f = uade_file(fileName.string().c_str(), silence.data(),
                          silence.size());
        }

        // IFF-SMUS resilience for a missing instrument *descriptor*: some modland
        // rips drop a whole ".instr" referenced by the score (e.g. SLL's
        // Super_sll_disco lists "Warriors1" first, but Warriors1.instr is absent
        // from the server). The Sonix driver "score died"s on the missing file
        // before it ever reaches the instruments that ARE present. Substitute a
        // valid, self-contained "Synthesis" instrument borrowed from the same
        // tune with its waveform zeroed: that voice plays silent and the rest of
        // the score survives. Gated to SMUS + ".instr" so other formats are
        // untouched.
        if (f == nullptr && player->isSmus &&
            utils::toLower(fileName.extension().string()) == ".instr") {
            auto stub = makeSilentInstr(fileName.parent_path());
            if (!stub.empty()) {
                LOGD("Missing .instr '%s' -> silent Synthesis placeholder",
                     fileName.string().c_str());
                f = uade_file(fileName.string().c_str(), stub.data(),
                              stub.size());
            }
        }
        return f;
    }

    // Build a silent stand-in for a missing ".instr": find a self-contained
    // "Synthesis" instrument already present beside the missing one, zero its
    // 256-byte waveform table (which starts after the 32-byte header + name field
    // + the 128-byte volume table) so the substituted voice produces no sound.
    // Returns empty if no Synthesis donor exists (then the caller leaves
    // f == nullptr).
    static std::vector<uint8_t> makeSilentInstr(fs::path const& dir)
    {
        std::error_code ec;
        for (auto const& de : fs::directory_iterator(dir, ec)) {
            if (ec) break;
            if (!de.is_regular_file()) continue;
            if (utils::toLower(de.path().extension().string()) != ".instr")
                continue;
            utils::File df{de.path().string()};
            auto data = df.readAll();
            // "Synthesis" donors are self-contained (no .ss dependency).
            if (data.size() < 0x1C4 ||
                memcmp(data.data(), "Synthesis", 9) != 0)
                continue;
            // Zero the waveform body; leave header/name/volume-envelope intact.
            std::fill(data.begin() + 0xC4, data.begin() + 0x1C4, uint8_t{0});
            return data;
        }
        return {};
    }

    // Create a fresh uade_state with our standard config + amiga loader. Factored
    // out of load() so the Hippel-CoSo -> ST fallback (see getSamples) can spin up
    // a clean state after the first player crashes.
    void createState()
    {
        struct uade_config* config = uade_new_config();
        uade_config_set_option(config, UC_ONE_SUBSONG, nullptr);
        uade_config_set_option(config, UC_IGNORE_PLAYER_CHECK, nullptr);
        uade_config_set_option(config, UC_NO_EP_END, nullptr);
        // uade_config_set_option(config, UC_VERBOSE, "true");
        uade_config_set_option(config, UC_BASE_DIR,
                               fs::absolute(dataDir).string().c_str());
        state = uade_new_state(config, 1);
        free(config);
        uade_set_amiga_loader(UADEPlayer::amigaloader, this, state);
    }

    // Some Amiga "Hippel COSO" (.hipc) modules crash the JochenHippel-CoSo player
    // ("illegal jump table index") and produce no audio at all, yet the larger
    // Jochen_Hippel_ST player plays the very same COSO data fine. When the default
    // player dies on the first read having emitted nothing, re-open the module
    // through the ST player by handing UADE a temp copy with a ".hst" suffix
    // (eagleplayer.conf routes hst -> Jochen_Hippel_ST). Triggered only on a
    // zero-output crash, so files that already play are never affected.
    bool retryWithStPlayer()
    {
        uade_cleanup_state(state, 1);
        state = nullptr;
        stFallbackFile =
            fs::path(utils::File::getTempDir().getName()) / (baseName + ".hst");
        std::error_code ec;
        if (fs::exists(stFallbackFile)) { fs::remove(stFallbackFile); }
        fs::copy_file(currentFileName, stFallbackFile, ec);
        if (ec) { return false; }
        createState();
        if (uade_play(stFallbackFile.string().c_str(), -1, state) != 1) {
            return false;
        }
        songInfo = uade_get_song_info(state);
        LOGD("Hippel-CoSo crashed; retrying via %s", songInfo->playername);
        setMeta("format", std::string(songInfo->playername) + " (Amiga)");
        return true;
    }

    bool load(fs::path const& fileName)
    {
        createState();

        loadDir = fileName.parent_path();
        baseName = fileName.stem().string();
        isSmus = isIffSmus(fileName);
        currentFileName = fileName;

        auto suffix = fileName.extension();
        cosoFamily = (suffix == ".hipc" || suffix == ".soc");

        // Jochen Hippel ST modules (TFMX-derived ST driver, "TFMX" magic) are
        // sometimes filed with a .sog extension, which eagleplayer.conf maps to
        // the *Amiga* JochenHippel player -- that player can't parse the ST data,
        // idles, and dies with no real audio. Hand UADE a temp copy with a ".hst"
        // suffix so it routes to the Jochen_Hippel_ST player (which plays the same
        // data fine). Genuine Amiga JochenHippel .sog files are relocatable 68k
        // player code starting with a BRA ($60xx), never "TFMX", so this can't
        // regress them.
        if (suffix == ".sog") {
            utils::File f{fileName.string()};
            auto data = f.exists() ? f.readAll() : std::vector<uint8_t>{};
            if (data.size() >= 4 && memcmp(data.data(), "TFMX", 4) == 0) {
                uadeFile = fs::path(utils::File::getTempDir().getName()) /
                           (baseName + ".hst");
                LOGD("Routing Hippel-ST .sog %s via %s",
                     fileName.string().c_str(), uadeFile.string().c_str());
                if (fs::exists(uadeFile)) { fs::remove(uadeFile); }
                fs::copy(fileName, uadeFile);
                currentFileName = uadeFile;
            }
        }

        if (suffix == ".mdat") {
            // Transform to prefixed name so UADE can recognize it
            uadeFile = fs::path(utils::File::getTempDir().getName()) / "mdat.music";
            LOGD("Translated %s to %s", fileName.string().c_str(), uadeFile.string().c_str());
            if (fs::exists(uadeFile)) { fs::remove(uadeFile); }
            fs::copy(fileName, uadeFile);
            currentFileName = uadeFile;
        }

        if (suffix == ".puma") {
            // eagleplayer.conf uses prefixes=puma, so UADE expects puma.<stem>
            uadeFile = fs::path(utils::File::getTempDir().getName()) /
                       ("puma." + fileName.stem().string());
            LOGD("Translated %s to %s", fileName.string().c_str(), uadeFile.string().c_str());
            if (fs::exists(uadeFile)) { fs::remove(uadeFile); }
            fs::copy(fileName, uadeFile);
            currentFileName = uadeFile;
        }
        // NOTE: Richard Joseph (.sng) files are intentionally NOT renamed/copied.
        // UADE recognizes them by content (the "RJP1SMOD" magic), and the
        // RichardJoseph Amiga player loads its samples by taking the module's
        // own path and swapping the extension ".sng" -> ".INS" in the SAME
        // directory. getSecondaryFiles() ensures the ".ins" companion is
        // downloaded next to the ".sng" in the cache dir, so playing the file
        // under its real name lets the player find its samples. (Copying it to a
        // temp dir broke this: the ".ins" wasn't there and the extension swap
        // produced the wrong name.)

        if (suffix == ".ymst") {
            auto replay = ymstReplayName(fileName);
            if (!replay.empty() && !fs::exists(loadDir / replay)) {
                LOGD("YMST replay '%s' not found in %s, cannot play",
                     replay.c_str(), loadDir.string().c_str());
                return false;
            }
        }

        LOGD("UADE FILE %s", currentFileName.string().c_str());
        if (uade_play(currentFileName.string().c_str(), -1, state) == 1) {
            songInfo = uade_get_song_info(state);
            std::string modname = songInfo->modulename;
            if (modname == "<no songtitle>") { modname = ""; }
            // if (modname.empty()) {
            //     fs::path p = currentFileName;
            //     auto stem = p.stem().string();
            //     auto file_name = p.filename().string();
            //     if (utils::startsWith(file_name, "mdat")) {
            //         modname = file_name.substr(5);
            //     } else {
            //         modname = stem;
            //     }
            // }
            setMeta("songs",
                    songInfo->subsongs.max - songInfo->subsongs.min + 1,
                    "startsong",
                    songInfo->subsongs.def - songInfo->subsongs.min, "length",
                    static_cast<uint32_t>(songInfo->duration), "title", modname,
                    "format", std::string(songInfo->playername) + " (Amiga)");
            valid = true;
        }
        return valid;
    }
    ~UADEPlayer() override
    {
        uade_cleanup_state(state, 1);
        state = nullptr;
        if (!uadeFile.empty()) { fs::remove(uadeFile); }
        if (!stFallbackFile.empty()) { fs::remove(stFallbackFile); }
    }

    int getSamples(int16_t* target, int noSamples) override
    {
        auto rc = uade_read(target, noSamples * 2, state);
        struct uade_notification nf
        {};
        while (uade_read_notification(&nf, state) != 0) {
            if (nf.type == UADE_NOTIFICATION_SONG_END) {
                LOGD("UADE SONG END: %d %d %d %s", nf.song_end.happy,
                     nf.song_end.stopnow, nf.song_end.subsong,
                     nf.song_end.reason);
                setMeta("song", nf.song_end.subsong + 1);
                bool crashed =
                    nf.song_end.happy == 0 ||
                    (nf.song_end.reason &&
                     std::strstr(nf.song_end.reason, "crash") != nullptr);
                uade_cleanup_notification(&nf);
                // A COSO module that crashed the default player before emitting
                // any audio: retry once through the Jochen_Hippel_ST player, which
                // tolerates COSO data the JochenHippel-CoSo player chokes on.
                if (cosoFamily && crashed && samplesEmitted == 0 &&
                    !stFallbackTried) {
                    stFallbackTried = true;
                    if (retryWithStPlayer()) {
                        auto rc2 = uade_read(target, noSamples * 2, state);
                        if (rc2 > 0) {
                            samplesEmitted += rc2 / 2;
                            return rc2 / 2;
                        }
                        return rc2 < 0 ? -1 : 0;
                    }
                }
                // Signal a hard end-of-stream (negative) rather than 0. The host
                // only treats a NEGATIVE return as "song finished"; returning 0
                // makes it retry next frame, so a crashed/looping score (e.g. a
                // Hippel-CoSo "illegal jump table index" that emits SONG_END every
                // read) would spin forever emitting "No more subsongs left".
                return -1;
            }
            if (nf.type == UADE_NOTIFICATION_MESSAGE) {
                LOGD("Amiga message: %s\n", nf.msg);
            } else {
                LOGD("Unknown notification: %d\n", nf.type);
            }
            uade_cleanup_notification(&nf);
        }
        if (rc > 0) {
            samplesEmitted += rc / 2;
            return rc / 2;
        }
        return rc;
    }

    bool seekTo(int song, int /*seconds*/) override
    {
        if (song < 0) { return false; }
        uade_seek(UADE_SEEK_SUBSONG_RELATIVE, 0, song + songInfo->subsongs.min,
                  state);
        setMeta("song", song);
        return true;
    }

private:
    fs::path uadeFile; // Copy of main song but with different name
    fs::path dataDir;
    bool valid{false};
    struct uade_state* state{};
    const struct uade_song_info* songInfo{};
    std::string baseName;
    fs::path currentFileName;
    fs::path loadDir;
    // Hippel-CoSo -> Jochen_Hippel_ST crash fallback (see retryWithStPlayer).
    bool cosoFamily{false};
    bool isSmus{false}; // IFF-SMUS score: tolerate missing .ss samples
    bool stFallbackTried{false};
    int64_t samplesEmitted{0};
    fs::path stFallbackFile;
};

static const std::set<std::string> supported_ext{
    "smod",      "lion",         "okta",        "sid",          "ymst",
    // NOTE: "sps"/"spm" (StoneTracker) intentionally omitted -- UADE ships no
    // Stonetracker eagleplayer, so claiming them produced a hard "playback
    // FAILED" instead of a clean Skip. No open replayer exists anywhere to
    // vendor (not UADE/rePlayer/libxmp/OpenMPT); only Marty's x86 DOS/GUS
    // hardware-mixing player. See data/misc/not_supported_extensions.txt.
    "jb",        "ast",          "ahx",
    "thx",       "adpcm",        "amc",         "nt",           "abk",
    "aam",       "alp",          "aon",         "aon4",         "aon8",
    "adsc",      "mod_adsc4",    "bss",         "bd",           "bds",
    "uds",       "kris",         "cin",         "core",         "cus",
    "cust",      "custom",       "cm",          "rk",           "rkb",
    "dz",        "mkiio",        "dl",          "dl_deli",      "dln",
    "dh",        "dw",           "dwold",       "dlm2",         "dm2",
    "dlm1",      "dm1",          "dsr",         "db",           "digi",
    "dsc",       "dss",          "dns",         "ems",          "emsv6",
    "ex",        "fc13",         "fc3",         "fc",           "fc14",
    "fc4",       "fred",         "gray",        "bfc",          "bsi",
    "fc-bsi",    "fp",           "fw",          "glue",         "gm",
    "ea",        "mg",           "hd",          "hipc",         "soc",
    "emod",      "qc",           "ims",         "dum",          "is",
    "is20",      "jam",          "jc",          "jmf",          "jcb",
    "jcbo",      "jpn",          "jpnd",        "jp",           "jt",
    "mon_old",   "jo",           "hip",         "mcmd",         "sog",
    "hip7",      "s7g",          "hst",         "kh",           "powt",
    "pt",        "lme",          "mon",         "mfp",          "hn",
    "mtp2",      "thn",          "mc",          "mcr",          "mco",
    "mk2",       "mkii",         "avp",         "mw",           "max",
    "mcmd_org",  "med",          "mmd0",        "mmd1",         "mmd2",
    "mso",       "midi",         "md",          "mmdc",         "dmu",
    "mug",       "dmu2",         "mug2",        "ma",           "mm4",
    "mm8",       "mms",          "ntp",         "two",          "octamed",
    "okt",       "one",          "dat",         "ps",           "snk",
    "pvp",       "pap",          "psa",         "mod_doc",      "mod15",
    "mod15_mst", "mod_ntk",      "mod_ntk1",    "mod_ntk2",     "mod_ntkamp",
    "mod_flt4",  "mod",          "mod_comp",    "!pm!",         "40a",
    "40b",       "41a",          "50a",         "60a",          "61a",
    "ac1",       "ac1d",         "aval",        "chan",         "cp",
    "cplx",      "crb",          "di",          "eu",           "fc-m",
    "fcm",       "ft",           "fuz",         "fuzz",         "gmc",
    "gv",        "hmc",          "hrt",         "hrt!",         "ice",
    "it1",       "kef",          "kef7",        "krs",          "ksm",
    "lax",       "mexxmp",       "mpro",        "np",           "np1",
    "np2",       "noisepacker2", "np3",         "noisepacker3", "nr",
    "nru",       "ntpk",         "p10",         "p21",          "p30",
    "p40a",      "p40b",         "p41a",        "p4x",          "p50a",
    "p5a",       "p5x",          "p60",         "p60a",         "p61",
    "p61a",      "p6x",          "pha",         "pin",          "pm",
    "pm0",       "pm01",         "pm1",         "pm10c",        "pm18a",
    "pm2",       "pm20",         "pm4",         "pm40",         "pmz",
    "polk",      "pp10",         "pp20",        "pp21",         "pp30",
    "ppk",       "pr1",          "pr2",         "prom",         "pru",
    "pru1",      "pru2",         "prun",        "prun1",        "prun2",
    "pwr",       "pyg",          "pygm",        "pygmy",        "skt",
    "skyt",      "snt",          "snt!",        "st2",          "st26",
    "st30",      "star",         "stpk",        "tp",           "tp1",
    "tp2",       "tp3",          "un2",         "unic",         "unic2",
    "wn",        "xan",          "xann",        "zen",          "puma",
    "rjp",       "sng",              "riff",        "rh",           "rho",
    "sa-p",      "scumm",        "s-c",         "scn",          "scr",
    "sid1",      "smn",          "sid2",        "mok",          "sa",
    "sonic",     "sa_old",       "smus",        "snx",          "tiny",
    "spl",       "sc",           "sct",         "psf",          "sfx",
    "sfx13",     "tw",           "sm",          "sm1",          "sm2",
    "sm3",       "smpro",        "bp",          "sndmon",       "bp3",
    "sjs",       "jd",           "doda",        "sas",          "ss",
    "sb",        "jpo",          "jpold",       "sun",          "syn",
    "sdr",       "osp",          "st",          "synmod",       "tfmx1.5",
    "tfhd1.5",   "tfmx7v",       "tfhd7v",      "mdat",         "tfmxpro",
    "tfhdpro",   "tfmx",         "mdst",        "thm",          "tf",
    "tme",       "sg",           "dp",          "trc",          "tro",
    "tronic",    "ufo",          "mod15_ust",   "vss",          "wb",
    "ym",        "ml",           "mod15_st-iv", "agi",          "tpu",
    "qpa",       "sqt",          "qts",         "ftm",          "sdata",
    // NOTE: ".dux" intentionally removed -- it was listed here but UADE has NO
    // player for it (no eagleplayer.conf prefix, no amifilemagic detection, no
    // player binary), so every .dux module just FAILED ("score died"). No open
    // replayer exists; declining the extension lets it Skip cleanly. See
    // data/misc/not_supported_extensions.txt. Same pattern as .spm/.sps.
    // Added in UADE 3.05 re-vendor (new eagleplayers). Note: PokeyNoise (.pn)
    // is intentionally omitted -- its eagleplayer crashes the vendored 68k
    // engine ("score died"); see data/misc/not_supported_extensions.txt.
    "aps",       "arp",          "ash",         "bye",          "dm",
    "hot",       "js",           "kim",         "mod3",         "mosh",
    "mus",       "npp",          "pat",         "prt",          "ptm",
    "rj",        "sfx20",        "tcb",         "tits",         "tmk"};

// Detects an MSX BSAVE binary header: marker 0xFE followed by little-endian
// start/end/exec addresses, with start <= end. FAC SoundTracker .MUS tunes are
// raw MSX binaries shaped this way (start 0x8000, end 0xBFFF).
static bool isMsxBsave(const std::string& name)
{
    utils::File f{name};
    if (!f.exists()) { return false; }
    auto data = f.readAll();
    if (data.size() < 7 || data[0] != 0xFE) { return false; }
    uint16_t start = data[1] | (data[2] << 8);
    uint16_t end = data[3] | (data[4] << 8);
    return start <= end;
}

// ".sng" is shared (case-insensitively): UADE owns the Amiga Richard Joseph
// variant, but the modland "SCC-Musixx" corpus also uses .SNG -- MSX music for
// Konami's SCC chip, played by sccmusixxplugin. We must dispatch by content, not
// by extension casing. SCC-Musixx images are raw MSX dumps with no magic of
// their own, so reject the other ".sng" magics (incl. Richard Joseph's own
// RJP1SMOD) and confirm the SCC-Musixx structure: 256-byte-aligned size, the
// first waveform's 8-char name at 0x20, a sane sequence length at 0x780, and
// small pattern indices in the order list at 0x781. Decline those here so they
// route to sccmusixxplugin. (Kept in sync with looksLikeSccMusixx there.)
static bool isSccMusixxSng(const std::string& name)
{
    utils::File f{name};
    if (!f.exists()) { return false; }
    auto data = f.readAll();
    const auto len = data.size();
    const uint8_t* d = data.data();
    if (len < 0x800 || (len % 256) != 0) { return false; }
    if (memcmp(d, "GTS", 3) == 0) { return false; }  // GoatTracker
    if (memcmp(d, "ObsM", 4) == 0) { return false; } // AdLib SNGPlay
    if (memcmp(d, "FMC!", 4) == 0) { return false; } // AdLib Faust Music Creator
    if (memcmp(d, "RJP1", 4) == 0) { return false; } // (this is UADE's own RJ)
    for (int i = 0; i < 8; i++) {
        if (d[0x20 + i] < 0x20 || d[0x20 + i] > 0x7E) { return false; }
    }
    unsigned seqlen = d[0x780];
    if (seqlen < 1 || seqlen > 200 || 0x781 + seqlen > len) { return false; }
    for (unsigned i = 0; i < seqlen; i++) {
        if (d[0x781 + i] >= 64) { return false; }
    }
    return true;
}

// ".sng" is one of the most overloaded extensions in the corpus, spanning
// several unrelated chips. UADE (a 68k Amiga emulator) can only actually play
// the Amiga "Richard Joseph" variant, whose files start with the magic
// "RJP1SMOD" and are content-detected by amifilemagic. But UADE claims .sng by
// extension, and for anything the content detector doesn't recognise the
// runtime eagleplayer.conf prefix rule (sng -> ZoundMonitor) dumps the file
// into the ZoundMonitor 68k player. That player then misreads foreign data as
// sample names and dies ("score died") -- a hard playback FAILURE instead of a
// clean Skip. This hits the C64/SID formats (GoatTracker "GTS5", Synder), the
// SAM Coupe SAA1099 "Sam Coupe SNG" corpus, and even genuine ZoundMonitor
// tunes (whose external Samples/ files we can't fetch). So keep only Richard
// Joseph and decline every other readable .sng: the AdLib variants (FMC!/ObsM)
// then route to AdPlug, and the rest Skip cleanly. If the header is unreadable
// (a virtual path during a dry canHandle), return false so we fall through to
// the old extension match and nothing regresses.
static bool isUnplayableUadeSng(const std::string& name)
{
    utils::File f{name};
    if (!f.exists()) { return false; }
    auto data = f.readAll();
    if (data.size() < 8) { return false; }
    // Richard Joseph is the only .sng family UADE actually plays.
    // Richard Joseph is one .sng family UADE plays outright; ZoundMonitor is the
    // other, but it is claimed separately (isZoundMonitor) because we can fetch
    // its samples, so treat only RJ as "playable" here.
    return memcmp(data.data(), "RJP1SMOD", 8) != 0;
}

// ZoundMonitor (Amiga, Marco Swagerman) .sng: UADE genuinely has this player
// (eagleplayer.conf prefix sng -> ZoundMonitor), but the modland tunes reference
// external samples from a shared "Samples/" dir. We CAN fetch those (see
// getSecondaryFiles), so -- unlike the other non-RJ .sng chips -- claim it.
// ZoundMonitor carries no magic, so it must be told apart from the Synder
// ("SYND"/"SYNC") and Sam Coupe .sng (and from SCC-Musixx) by structure: the
// 9-byte header has byte[0]==byte[3] (both nonzero), byte[2]==0 and byte[5]==0,
// immediately followed by the first sample's printable, NUL-terminated name.
// Validated against all 9 modland ZoundMonitor tunes and rejects every Synder /
// Sam Coupe / SCC-Musixx / GoatTracker / Richard Joseph header.
static bool isZoundMonitor(const std::string& name)
{
    utils::File f{name};
    if (!f.exists()) { return false; }
    auto data = f.readAll();
    if (data.size() < 32) { return false; }
    const uint8_t* d = data.data();
    if (d[0] == 0 || d[0] != d[3] || d[2] != 0 || d[5] != 0) { return false; }
    // First sample name at offset 9: printable and NUL-terminated within 22 bytes.
    if (d[9] < 0x20 || d[9] > 0x7E) { return false; }
    bool terminated = false;
    for (int i = 9; i < 31; i++) {
        if (d[i] == 0) { terminated = true; break; }
        if (d[i] < 0x20 || d[i] > 0x7E) { return false; }
    }
    return terminated;
}

// ".ast" is shared. UADE's "ActionAmics" eagleplayer (prefixes=ast) is the
// "Action Amics SoundTools V0.1 player module V1.1 (05 April 93)" testversion --
// it only drives the V0.1 replay-ready binary dump (a pointer table + the
// "ACTIONAMICS SOUND TOOL V0.1" string, e.g. testmus/uade/dynablaster.ast). The
// modland "All Sound Tracker" corpus uses the same .ast extension but is the
// tracker's native versioned save format, a different layout with a Pascal-string
// magic \x08"AST 00xx" (versions "AST 0001", "AST 0032", ...). The V0.1 player
// cannot parse it: selected by extension it runs but never produces audio, so
// UADE reports success while the speakers stay silent. No open replayer exists
// for the native format, so decline it here -- it Skips cleanly instead of
// false-positiving as a silent "ok". The genuine V0.1 dumps carry no "AST" magic,
// so this can't regress them.
static bool isAllSoundTrackerNative(const std::string& name)
{
    utils::File f{name};
    if (!f.exists()) { return false; }
    auto data = f.readAll();
    return data.size() >= 9 && memcmp(data.data(), "\x08""AST 00", 7) == 0;
}

// ".mon" is shared: UADE owns the Amiga "Maniacs of Noise" player, but the
// modland "Monotone" corpus also uses .mon -- a PC-speaker tracker (Trixter/
// Hornet) that has nothing to do with the Amiga. Fed to the 68k Maniacs of
// Noise player it crashes ("Illegal instruction" jumping into the module data),
// so decline it here -- monotoneplugin plays it instead. Real Maniacs of Noise
// modules never carry this magic, so this can't regress.
static bool isMonotone(const std::string& name)
{
    utils::File f{name};
    if (!f.exists()) { return false; }
    auto data = f.readAll();
    return data.size() >= 9 && memcmp(data.data(), "\x08MONOTONE", 9) == 0;
}

// Forgotten Worlds game music (.fw, magic "FWMP", Mark Cooksey / Psygore
// eagleplayer). The ForgottenWorlds_Game player loads and runs but emits pure
// silence -- verified against upstream uade123 3.05, which renders all five
// modland tunes as silence while a TFMX module renders loud, so this is a player
// bug in UADE itself, not our integration, and is unfixable without RE-patching
// the player binary. Decline the format so it Skips cleanly instead of reporting
// a silent "ok"/NO SOUND. Re-enable if a working ForgottenWorlds player lands.
static bool isForgottenWorlds(const std::string& name)
{
    utils::File f{name};
    if (!f.exists()) { return false; }
    auto data = f.readAll();
    return data.size() >= 4 && memcmp(data.data(), "FWMP", 4) == 0;
}

// Old MED / Amiga "Music Editor" (magic 'M','E','D' + version byte 0x02..0x04,
// MED 2.00 .. 3.20). This is the pre-OctaMED format, distinct from the
// MMD0..MMD3 OctaMED containers ('M','M','D',...) that UADE's MED eagleplayer
// legitimately drives. Fed an old-MED file the 68k player's score dies
// ("UADE SONG END: 0 ... score crashed") -- verified on modland's "Music
// Editor" corpus (e.g. orage.med, fresnel.med). libxmp decodes these and
// medplugin claims them, but medplugin registers after UADE, so we MUST decline
// here (UADE otherwise returns a player that crashes at getSamples time, and the
// host never falls through). MMD containers keep byte[1]=='M' and are unaffected.
static bool isOldMed(const std::string& name)
{
    utils::File f{name};
    if (!f.exists()) { return false; }
    auto data = f.readAll();
    return data.size() >= 4 && data[0] == 'M' && data[1] == 'E' &&
           data[2] == 'D' && (uint8_t)data[3] >= 2 && (uint8_t)data[3] <= 4;
}

bool UADEPlugin::canHandle(const std::string& name)
{
    auto lowerName = utils::toLower(name);
    auto ext = utils::path_extension(lowerName);
    if (ext == "mon" && isMonotone(name)) { return false; } // PC-speaker Monotone
    if (ext == "fw" && isForgottenWorlds(name)) { return false; } // silent in UADE
    if (ext == "med" && isOldMed(name)) { return false; } // old MED -> medplugin (libxmp)
    // .mus is an overloaded modland extension: UADE's UFO eagleplayer owns the
    // Amiga variant, but it is also used by FAC SoundTracker, an MSX FM format
    // (FM-PAC/YM2413 + MSX-AUDIO/Y8950)
    // the vendored 68k engine cannot run (it feeds Z80 code to a 68k player and
    // the score dies). Decline MSX BSAVE .mus files so they don't false-positive
    // here. If the header is unreadable (e.g. a virtual path during a dry
    // canHandle), fall through to the old extension match so nothing regresses.
    if (ext == "mus" && isMsxBsave(name)) { return false; }
    if (ext == "sng" && isSccMusixxSng(name)) { return false; } // MSX SCC-Musixx
    // ZoundMonitor is a genuine UADE .sng player and we can fetch its shared
    // Samples/ (getSecondaryFiles), so claim it explicitly before the blanket
    // decline below.
    if (ext == "sng" && isZoundMonitor(name)) { return true; }
    // Beyond SCC-Musixx and ZoundMonitor, .sng covers many chips UADE can't
    // touch (GoatTracker/Synder SID, Sam Coupe SAA1099). Keep only the Amiga
    // Richard Joseph variant; decline the rest so they Skip (or route to AdPlug
    // for the AdLib FMC!/ObsM variants) instead of crashing the 68k engine with
    // "score died".
    if (ext == "sng" && isUnplayableUadeSng(name)) { return false; }
    // .ast: decline the "All Sound Tracker" native save format (magic \x08"AST 00xx").
    // The V0.1 ActionAmics player only handles the binary V0.1 dump; the native
    // saves load silently. Skip them cleanly rather than report a silent "ok".
    if (ext == "ast" && isAllSoundTrackerNative(name)) { return false; }
    // The Amiga "SoundMaster" eagleplayer (Michiel Soede, .sm/.sm1/.sm2/.sm3/
    // .smpro) genuinely works -- real modules are relocatable 68k code starting
    // with a BRA ($60xx). But the modland .SM1/.SM2 corpus also contains
    // unrelated MSX BSAVE dumps (0xFE header, load 0x8000-0xBFFF) that just
    // happen to share the extension. Fed to the 68k SoundMaster player they are
    // unrecognised: the score idles, UADE force-starts audio after 3s
    // ("involuntary audio output start") and then fails before the playloop.
    // Decline the MSX blobs so they Skip cleanly instead of hanging UADE; real
    // SoundMaster modules never carry the 0xFE marker, so this can't regress.
    static const std::set<std::string> soundMasterExt{"sm", "sm1", "sm2", "sm3",
                                                      "smpro"};
    if (soundMasterExt.count(ext) > 0 && isMsxBsave(name)) { return false; }
    if (supported_ext.count(ext) > 0) {
        return true;
    }
    return (supported_ext.count(utils::path_prefix(lowerName)) > 0);
}

std::set<std::string> UADEPlugin::getSupportedExtensions() const
{
    return supported_ext;
}

std::vector<std::string> UADEPlugin::getSecondaryFiles(const std::string& file)
{
    bool isStarTrekker = (file.find("Startrekker") != std::string::npos);

    // Known music formats with 2 files
    static const std::unordered_map<std::string, std::string> fmt_2files = {
        {"mdat", "smpl"},    // TFMX
        {"sng", "ins"},      // Richard Joseph
        {"ash", "smp"},      // Ashley Hogg (Codemasters Amiga games)
        {"jpn", "smp"},      // Jason Page PREFIX
        {"sjs", "smp"},      // SoundPlayer (Scott Johnston)
        {"dum", "ins"},      // Rob Hubbard 2
        {"adsc", "adsc.as"}, // Audio Sculpture
        {"sdata", "ip"},     // Audio Sculpture
        {"dns", "smp"},      // Dynamic Synthesizer
        {"mcr", "mcs"},      // Mark Cooksey
        {"uds", "smp"},      // Unique Development (BladePacker)
        {"tpu", "smp"},      // Dirk Bialluch
        {"mfp", "smp"},      // Magnetic Fields Packer
        {"thm", "smp"},      // Thomas Hermann
        {"MIDI", "SMPL"}     // MIDI-Loriciel (prefix/companion are upper-case)
    };

    std::string fileName = file;
    std::string prefix;
    size_t dot = 0;

    std::string ext = utils::path_extension(file);
    std::string base = utils::path_basename(file);

    auto slash = file.find_last_of("/\\");
    if (slash != std::string::npos) {
        fileName = file.substr(slash + 1);
        dot = fileName.find_first_of('.');
        if (dot != std::string::npos) { prefix = fileName.substr(0, dot); }
    }

    std::vector<std::string> result;

    LOGD("FILENAME '%s', PREFIX '%s', EXT '%s', BASE '%s'", fileName.c_str(), prefix.c_str(),
         ext.c_str(), base.c_str());

    // Synth Dream (.sdr prefix): some tunes carry a per-song "smp.<name>"
    // sample file, but a whole set (e.g. Laurens Tummers' "monsterbusiness"/
    // "nobuddiesland" tunes) instead shares a single "smp.set" bank in the same
    // directory. Surface both candidates; the host treats a missing companion
    // as non-fatal, so whichever exists is fetched next to the song.
    if (prefix == "sdr") {
        base = fileName.substr(dot + 1);
        return { "smp." + base, "smp.set" };
    }

    // ZoundMonitor (.sng): the tune loads its instruments by name from a
    // "Samples/" directory. On modland that directory is shared at the
    // collection root ("Zoundmonitor/Samples/"), one level above each song, so
    // surface it as a whole-directory companion -- MusicPlayerList lists it
    // (falling back to the parent dir) and fetches every sample into the song's
    // own "Samples/" where the ZoundMonitor player looks. Content-checked so it
    // never fires for the Richard Joseph ".sng" handled below.
    if (ext == "sng" && isZoundMonitor(file)) { return { "Samples/" }; }

    // Quartet ST (.qts, "qts.<name>"): the tunes in a folder share a single
    // fixed-name "SMP.set" sample bank (the per-song "set.<name>" convention
    // does not apply -- only "SMP.set" actually drives the Quartet_ST player).
    if (prefix == "qts") { return { "SMP.set" }; }

    // Kris Hatlelid (.kh): the tunes in a folder share a fixed-name "songplay"
    // replay executable that the eagleplayer loads from the same directory.
    if (ext == "kh") { return { "songplay" }; }

    if (fmt_2files.count(prefix) > 0) {
        base = fileName.substr(dot + 1);
        LOGD("Found prefix, base now %s", base.c_str());
        result.push_back(fmt_2files.at(prefix) + "." + base);
        return result;
    }

    if (ext == "ymst") {
        auto replay = ymstReplayName(file);
        if (!replay.empty()) { result.push_back(replay); }
        return result;
    }

    // IFF-SMUS (Aegis Sonix) scores reference their instruments by name from a
    // sibling "Instruments/" subdirectory whose member names can't be predicted
    // from the score (see isIffSmus). Surface the directory itself (trailing
    // slash) so MusicPlayerList fetches the whole folder next to the score.
    // Content-checked, since the "smus"/"snx"/"tiny" prefixes overlap other
    // Sonix variants.
    if ((ext == "smus" || prefix == "smus" || prefix == "snx" ||
         prefix == "tiny") &&
        isIffSmus(file)) {
        return { "Instruments/" };
    }

    // Kris Hatlelid (.kh): the song data pairs with a fixed-name "songplay"
    // replay/driver file in the same directory (one per game dir on modland);
    // without it the KrisHatlelid player loads but renders silent. The name is
    // constant, not derived from the song, so surface it verbatim.
    if (ext == "kh") { return { "songplay" }; }

    // MusicMaker V8 (.sdata): the song references a 3-part instrument pack in
    // the same directory -- "<name>.ip" (samples) plus "<name>.ip.l" and
    // "<name>.ip.n" (metadata). All three must land next to the .sdata or the
    // tune renders silent, so surface every part.
    if (ext == "sdata") {
        return { base + ".ip", base + ".ip.l", base + ".ip.n" };
    }

    std::string ext2;
    if (fmt_2files.count(ext) > 0) {
        ext2 = fmt_2files.at(ext);
    } else if (fmt_2files.count(base) > 0) {
        ext2 = base;
        base = fmt_2files.at(base);
    } else if (isStarTrekker) {
        ext2 = "mod.nt";
    }
    if (!ext2.empty()) { result.push_back(base + "." + ext2); }
    return result;
}

ChipPlayer* UADEPlugin::fromFile(const std::string& fileName)
{
    auto realName = fs::absolute(fileName);
    auto* player = new UADEPlayer(dataDir / "uade");
    LOGD("UADE data %s", dataDir.string().c_str());
    if (!player->load(realName)) {
        delete player;
        player = nullptr;
    }
    return player;
}

} // namespace musix
//
extern "C" void uadeplugin_register()
{
    musix::ChipPlugin::addPluginConstructor([](std::string const& config) {
        return std::make_shared<musix::UADEPlugin>(config);
    });
}
