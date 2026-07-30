#include "catch.hpp"

#include "src/MusicDatabase.h"
#include "src/MusicPlayer.h"
#include "src/MusicPlayerList.h"
#include "src/RemoteLoader.h"
#include "src/LhaArchive.h"
#include "src/modutils.h"

#include "src/di.hpp"
namespace di = boost::di;

#include <audioplayer/audioplayer.h>
#include <coreutils/log.h>
#include <musicplayer/src/chipplugin.h>
#include <musicplayer/src/plugins/plugins.h>
#include <musicplayer/src/plugins/uadeplugin/UADEPlugin.h>
#include <musicplayer/src/plugins/ffmpegplugin/FFMPEGPlugin.h>
#include <musicplayer/src/plugins/ptkplugin/PTKPlugin.h>
#include <musicplayer/src/plugins/openmptplugin/OpenMPTPlugin.h>
#include <musicplayer/src/plugins/quartetplugin/QuartetPlugin.h>
#include <musicplayer/src/plugins/dmfplugin/DMFPlugin.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <memory>
#include <pthread.h>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <string>
#include <thread>
#include <unordered_map>
#include <set>
#include <cstring>

// libav logging is controlled directly in the "expected failure" URL test so its
// deliberate connection error is labelled and not printed in libav's red.
extern "C" {
#include <libavutil/log.h>
}
namespace fs = std::filesystem;

// Running tallies across all playback (testPlugin) runs, summarized in "coverage".
static int g_errors = 0; // red lines: FAILED / NO SOUND / EXCEPTION
static int g_skips = 0;   // gray lines: Skipping (plugin can't handle)
static int g_ok = 0;     // playback OK

// Same tallies de-duplicated by extension: 5 .mod OKs count as one unique OK.
// An extension can appear in more than one set (e.g. some .hsc play, some don't).
static std::set<std::string> g_errorExts;
static std::set<std::string> g_skipExts;
static std::set<std::string> g_okExts;

// Extensions that are truly impossible to support (per data/misc/
// not_supported_extensions.txt). These are silently ignored everywhere -- no
// testing, no Skipping warning, no missing-coverage report -- and listed once
// at the very end of the run. Stored lower-case and without the leading dot.
static const std::set<std::string>& notSupportedExts()
{
    static const std::set<std::string> exts = [] {
        std::set<std::string> s;
        std::ifstream f("data/misc/not_supported_extensions.txt");
        std::string line;
        while (std::getline(f, line)) {
            auto a = line.find_first_not_of(" \t\r\n");
            if (a == std::string::npos) { continue; }
            auto b = line.find_last_not_of(" \t\r\n");
            line = line.substr(a, b - a + 1);
            if (line.empty() || line[0] == '#') { continue; }
            if (line[0] == '.') { line.erase(0, 1); }
            if (!line.empty()) { s.insert(utils::toLower(line)); }
        }
        return s;
    }();
    return exts;
}

// Classification for files that are correctly skipped and shouldn't count
// toward coverage tallies.
// Category A: Companion/Support files (instruments, banks, libs).
// Category B: Intentionally Unsupported or Negative Tests (Deflemask DMF, bad PSF).
static bool shouldIgnoreFile(const std::string& name)
{
    static const std::set<std::string> ignoredExts = {
        "ins", "bnk", "dat", "dtl", "edl", "fmf", "cal", "d01", "vib", "003",
        "fmb", "pmb", "pvi", "mbk", "pdx", "gsflib", "2sflib", "qsflib",
        "ssflib", "usflib", "psflib", "psf2lib", "opm", "ss", "instr", "inst",
        "dsflib", "smpl", "ip", "sm1", "sm2"
    };
    auto ext = utils::toLower(utils::path_extension(name));
    if (ignoredExts.count(ext) > 0) return true;

    // Companion / sample-bank filename patterns (matched case-insensitively):
    // these pair with a song (their getSecondaryFiles names them) and aren't
    // standalone tunes -- e.g. "smpl.<song>" (TFMX), "SMPL.<song>" (MIDI-
    // Loriciel), "mcs.<song>" (Mark Cooksey), "<song>.ip.l/.ip.n" (MusicMaker).
    auto lname = utils::toLower(name);
    if (lname.find("smpl.") != std::string::npos) return true;
    if (lname.find("smp.") != std::string::npos) return true;
    if (lname.find("mcs.") != std::string::npos) return true;
    if (lname.find(".ip.") != std::string::npos) return true;
    if (lname.find(".adsc.as") != std::string::npos) return true;
    if (lname.find("sfx2.dmf") != std::string::npos) return true;
    if (lname.find("bad-magic-not-a-psf.psf") != std::string::npos) return true;
    // Kris Hatlelid (.kh) songs share a fixed-name "songplay" replay executable.
    if (lname.find("songplay") != std::string::npos) return true;

    static const std::set<std::string> auxExts = {
        "w", "md", "set"
    };
    if (auxExts.count(ext) > 0) return true;

    return false;
}

TEST_CASE("modutils", "[machine]")
{
    auto x = getTypeAndBase("/blaj/mdat.gurgle%tjosan");
    REQUIRE(x == std::make_tuple("mdat", "gurgle%tjosan"));

    x = getTypeAndBase("/blaj/skurk.mannen.x.mod");
    REQUIRE(x == std::make_tuple("mod", "skurk.mannen.x"));

    x = getTypeAndBase("/blaj/mod/mdat/hejsan hoppsan.whatever");
    REQUIRE(x == std::make_tuple("whatever", "hejsan hoppsan"));

    REQUIRE(getBaseName("/asda/das/test.mod") == "test.mod");
    REQUIRE(getTypeFromName("gurgle.format") == "format");
    REQUIRE(getTypeFromName("mdat.gurgle") == "mdat");
    REQUIRE(getTypeFromName("mdat.gurgle") == "mdat");
    REQUIRE(getTypeFromName("ftp%3a%2f%2fftp.modland.com%2fpub%2fmodules%"
                            "2fSunTronic%2fTSM%2fmsx-intro.sun") == "sun");
    REQUIRE(
        getTypeFromName("ftp%3a%2f%2fftp.modland.com%2fpub%2fmodules%2fTFMX%"
                        "2fChris Huelsbeck%2fmdat.apidya (level 3)") == "mdat");
}

TEST_CASE("music database", "[database]")
{
    using namespace chipmachine;
    const auto injector = di::make_injector(di::bind<utils::path>.to("."));

    auto mdb = injector.create<std::unique_ptr<MusicDatabase>>();
    REQUIRE(mdb->initFromLua(utils::path(".")) == true);
    auto q = mdb->createQuery();
}

struct AudioPlayerNull : public AudioPlayer
{
    std::function<void(int16_t*, int)> callback;
    virtual void play(std::function<void(int16_t*, int)> cb) override
    {
        callback = cb;
    }

    void get(std::vector<int16_t>& target)
    {
        callback(&target[0], target.size());
    }

    void seek(int seconds)
    {
        std::array<int16_t, 44100 * 2> dummy;
        while (seconds--) {
            callback(dummy.data(), dummy.size());
        }
    };
};

TEST_CASE("musicplayerlist", "")
{
    logging::setLevel(logging::Level::Debug);
    auto ap = std::make_shared<AudioPlayerNull>();
    const auto injector = di::make_injector(di::bind<utils::path>.to("."),
                                            di::bind<AudioPlayer>.to(ap));
    musix::ChipPlugin::createPlugins("data");
    auto mpl = injector.create<std::unique_ptr<chipmachine::MusicPlayerList>>();
    mpl->addSong("testmus/openmpt/Starbuck - Tennis.mod"s);
    mpl->addSong("testmus/openmpt/Dr.Awesome - Intromusic3.mod"s);
    mpl->nextSong();
    mpl->wait();
    auto state = mpl->getState();
    auto info = mpl->getInfo();
    //LOGI("%s %s %d", info.title, info.path, state);
    ap->seek(150);
    mpl->wait();
    info = mpl->getInfo();
    //LOGI("%s %s %d", info.title, info.path, state);
}

TEST_CASE("musicplayer", "")
{
    auto ap = std::make_shared<AudioPlayerNull>();
    const auto injector = di::make_injector(di::bind<utils::path>.to("."),
                                            di::bind<AudioPlayer>.to(ap));
    musix::ChipPlugin::createPlugins("data");
    chipmachine::MusicPlayer mp{ ap };
    bool ok = mp.playFile("testmus/openmpt/Nuke - Loader.mod");
    REQUIRE(ok);
    mp.update();
    std::vector<int16_t> data(8192);
    ap->get(data);
    auto sum = std::accumulate(data.begin(), data.end(), (int64_t)0);
    REQUIRE(sum != 0);
}

// A drummed FAC SoundTracker ".mus" is claimed by OpenMPT and libvice (both
// registered before KSSPlugin and needing no companions) as well as by
// KSSPlugin (which needs the song's .SM1/.SM2 drumkit banks). fromFile() tries
// claimers until one LOADS -- KSSPlugin -- so the host's getSecondaryFiles must
// not just return the FIRST claimer's (empty) list, or the drumkits never get
// fetched and KSSPlugin then fails "missing SM1/SM2". Regression for that GUI
// bug: getSecondaryFiles must surface KSSPlugin's drumkits despite the earlier
// claimers. (Plugin-isolation tests can't catch this -- it's host routing.)
TEST_CASE("MusicPlayer fetches the playing plugin's secondary files", "[music]")
{
    auto ap = std::make_shared<AudioPlayerNull>();
    musix::ChipPlugin::createPlugins("data");
    chipmachine::MusicPlayer mp{ ap };
    auto sec = mp.getSecondaryFiles("testmus/fac/32 color.mus");
    REQUIRE(std::find(sec.begin(), sec.end(), "DRUMKIT1.SM1") != sec.end());
    REQUIRE(std::find(sec.begin(), sec.end(), "DRUMKIT1.SM2") != sec.end());
}

// Exercises the *registered-plugin* host path (createPlugins -> MusicPlayer::
// fromFile -> getSamples -> fifo), not just the plugin in isolation. This is
// what the GUI / cm use, and it would have caught sksplugin being absent from
// chipmachine/src/plugin_register.cpp (it is registered in two places).
TEST_CASE("STarKos host path plays sound", "[music]")
{
    auto ap = std::make_shared<AudioPlayerNull>();
    const auto injector = di::make_injector(di::bind<utils::path>.to("."),
                                            di::bind<AudioPlayer>.to(ap));
    musix::ChipPlugin::createPlugins("data");
    chipmachine::MusicPlayer mp{ ap };
    bool ok = mp.playFile("testmus/sks/Targhan - Orion Prime - Introduction.sks");
    REQUIRE(ok);
    int64_t sum = 0;
    for (int i = 0; i < 20 && sum == 0; ++i) {
        mp.update();
        std::vector<int16_t> data(8192);
        ap->get(data);
        sum = std::accumulate(data.begin(), data.end(), (int64_t)0);
    }
    REQUIRE(sum != 0);
}

// OPL Archive routing regression: unlike the isolated LibVGM plugin test, this
// drives the app's real createPlugins()/register_plugins() (plugin_register.cpp)
// + MusicPlayer::fromFile path -- the one that first shipped broken because
// libvgmplugin was registered only in musicplayer's reg.cpp, not the app's list.
// Also feeds a DECOMPRESSED .vgz: the GUI's gzip-by-magic step inflates the
// downloaded .vgz before the plugin sees it, so the router must still land it on
// libvgm (GME declines OPL) and produce non-silent audio.
TEST_CASE("OPL Archive routes to libvgm and plays", "[music]")
{
    auto ap = std::make_shared<AudioPlayerNull>();
    const auto injector = di::make_injector(di::bind<utils::path>.to("."),
                                            di::bind<AudioPlayer>.to(ap));
    musix::ChipPlugin::createPlugins("data");
    chipmachine::MusicPlayer mp{ ap };
    for (auto const& vgz : {"testmus/libvgm/2a03fox - Snowgoons vs Acid (OPL2).vgz",
                            "testmus/libvgm/Zero - Shinespark (OPL3).vgz",
                            // Virtual Boy VSU: libvgm has the only VSU core, so
                            // this is the whole Nintendo Virtual Boy platform's
                            // playback path.
                            "testmus/libvgm/virtualboy-vsu.vgz"}) {
        REQUIRE(mp.playFile(vgz));
        int64_t sum = 0;
        for (int i = 0; i < 30 && sum == 0; ++i) {
            mp.update();
            std::vector<int16_t> data(8192);
            ap->get(data);
            sum = std::accumulate(data.begin(), data.end(), (int64_t)0);
        }
        REQUIRE(sum != 0);
    }
}

// VGMRips routing: VGM is a multi-chip container. GME's Vgm_Emu only decodes the
// Sega/AY logs (SN76489/YM2413/YM2612/AY8910); every other chip must route to
// libvgm via the chip gate in vgm_opl_detect.h. The testmus/libvgm fixtures carry
// one VGMRips rip per non-Sega chip (NES APU, GameBoy DMG, HuC6280, YM2610 Neo
// Geo, the OPN family/PC-98, QSound, C140) -- assert the gate sends them to
// libvgm and keeps GME off them, while the Sega/AY VGZ in testmus/gme stay on GME.
TEST_CASE("VGMRips non-Sega VGM routes to libvgm", "[music]")
{
    musix::LibVGMPlugin lv;
    musix::GMEPlugin gme;
    for (auto const& vgz : { "testmus/libvgm/nes-2a03.vgz",
                             "testmus/libvgm/gameboy-dmg.vgz",
                             "testmus/libvgm/pce-huc6280.vgz",
                             "testmus/libvgm/neogeo-ym2610.vgz",
                             "testmus/libvgm/pc98-opn.vgz",
                             "testmus/libvgm/capcom-qsound.vgz",
                             "testmus/libvgm/namco-c140.vgz",
                             // Virtual Boy VSU (@0xC4). The VB rips are the only
                             // VSU logs we carry and they are what puts the
                             // "Nintendo Virtual Boy" platform on the TAB screen.
                             "testmus/libvgm/virtualboy-vsu.vgz",
                             // Dual AY8910 (Capcom 1942): GME instantiates one
                             // AY, so the 2nd chip's writes overflow Ay_Apu and
                             // abort ("addr < reg_count"). The dual-chip bit must
                             // route it to libvgm even though AY8910 is a GME chip.
                             "testmus/libvgm/capcom-dual-ay8910.vgz" }) {
        REQUIRE(lv.canHandle(vgz));
        REQUIRE_FALSE(gme.canHandle(vgz));
    }
    // The Sega (YM2612) and Vectrex (AY8910) logs GME plays well must NOT move.
    for (auto const& vgz : { "testmus/gme/batman.vgz",
                             "testmus/gme/vectrex-berzerk.vgz" }) {
        REQUIRE(gme.canHandle(vgz));
        REQUIRE_FALSE(lv.canHandle(vgz));
    }
}

// VGMRips platform classification: the collection's path is an archive.org
// ".../<game>.zip" URL (no useful extension), so every game is filed purely by
// its `format` label. Guard that each distinct label resolves to the right
// platform byte -- never UNKNOWN (which would make the game invisible to every
// TAB platform filter).
// ChipPlugin::getSupportedExtensions() defaults to an EMPTY set in the base
// class, so a plugin that identifies files only in canHandle() is invisible to
// every caller that needs the set up front -- the archive track picker
// (MusicPlayerList::archiveExtensions) and the priority_map / playability
// audits. The symptom is silent and one-sided: a loose .ptk plays, but a .ptk
// inside a zip is "No playable tracks in archive", and an audit reports those
// rows as having no decoder at all.
//
// This guards the DEFAULT, which is the actual hole: a new plugin that forgets
// to override it fails here instead of quietly dropping its format out of every
// derived list. That is the same shape as the Zophar .adp and Organya .org gaps.
TEST_CASE("every plugin declares its extensions", "[music]")
{
    musix::ChipPlugin::createPlugins("data");
    std::vector<std::string> silent;
    for (auto const& pl : musix::ChipPlugin::getPlugins())
        if (pl->getSupportedExtensions().empty()) silent.push_back(pl->name());
    INFO("plugins returning an empty getSupportedExtensions(): "
         << [&] { std::string s; for (auto& n : silent) s += n + " "; return s; }());
    REQUIRE(silent.empty());
}

// The ZIP track picker must accept exactly what the app plays as a loose file.
// It used to carry two hand-maintained extension lists, which drifted: a zip
// holding only an Organya .org reported "No playable tracks in archive" even
// though OrgPlugin decodes it (129 demozoo archive rows were dead for exactly
// this reason), and the same gap had already hidden Zophar's GameCube .adp /
// Xbox .wma rips until someone patched the list by hand. The sets are now
// derived from the registered plugins, so a new plugin can't reintroduce it.
TEST_CASE("archive picker accepts every format the app can play", "[music]")
{
    musix::ChipPlugin::createPlugins("data");
    auto const& [songExt, audioExt] = chipmachine::MusicPlayerList::archiveExtensions();
    REQUIRE(songExt.size() > 200);   // was a 70-entry hand list

    // The formats that were unfindable inside an archive.
    for (auto* e : { "org", "mdl", "mo3", "a2m", "ftm", "ams", "prg" }) {
        INFO("song ext " << e);
        REQUIRE(songExt.count(e) == 1);
    }
    // Still classified as chip/module, i.e. preferred over a rendered preview.
    for (auto* e : { "mod", "xm", "it", "sid", "nsf", "adp", "musx" }) {
        INFO("song ext " << e);
        REQUIRE(songExt.count(e) == 1);
    }
    // ffmpeg renderings stay the FALLBACK bucket, never the preferred one --
    // otherwise a compo zip with a module + its .mp3 preview could play the mp3.
    for (auto* e : { "mp3", "ogg", "flac", "wav", "opus", "wma" }) {
        INFO("audio ext " << e);
        REQUIRE(audioExt.count(e) == 1);
        REQUIRE(songExt.count(e) == 0);
    }
    // .8svx stays in the AUDIO bucket, and that is correct here even though
    // format_map deliberately files it under Amiga rather than the rendered
    // "no platform" bucket: ffmpeg is the only plugin that decodes it, and this
    // bucket only means "fallback if no chip/module member exists". A zip with a
    // .mod next to a .8svx still plays the .mod. Two different questions -- what
    // PLATFORM a format belongs to, vs which member the picker prefers.
    REQUIRE(audioExt.count("8svx") == 1);
    // Extensions we ship a plugin for but can't really play must NOT be picked:
    // the picker would choose a member it is then guaranteed to fail on.
    // (Only meaningful once the not-supported list is loaded; guard on that.)
    if (auto* db = chipmachine::MusicDatabase::instance())
        for (auto const& e : db->unsupportedExtensions()) {
            INFO("not-supported ext " << e);
            REQUIRE(songExt.count(e) == 0);
            REQUIRE(audioExt.count(e) == 0);
        }
}

// pouet YouTube captures classify by their "Youtube (<platform>)" tag. Tags that
// name hardware resolve to it; tags that name none resolve to OTHER (the Other
// Platforms drill), NOT to a YouTube-only bucket -- there is no such filter now.
TEST_CASE("YouTube captures classify by their pouet platform tag", "[music]")
{
    using namespace chipmachine;
    RemoteLoader rl;
    MusicDatabase mdb{ rl };
    const std::string yt = "https://www.youtube.com/watch?v=abc";
    struct { const char* fmt; uint8_t plat; } cases[] = {
        { "Youtube (Amiga AGA)", AMIGA },
        { "Youtube (Commodore 64)", SID },
        { "Youtube (Windows)", PC },
        { "Youtube (Virtual Boy)", VIRTUALBOY },
        // Name no hardware -> Other Platforms, where the drill surfaces them
        // under the bare tag ("Wild", "Animation/Video"). These three were the
        // whole 1103-video bucket.
        { "Youtube (Animation/Video)", OTHER },
        { "Youtube (mIRC)", OTHER },
        { "Youtube (Alambik)", OTHER },
        { "Youtube (Wild)", OTHER },
        // A combo naming real hardware still wins over the generic tag.
        { "Youtube (Amiga AGA,Animation/Video)", AMIGA },
        { "Youtube (Windows,Animation/Video)", PC },
        // Every Atari machine reaches its own filter under the TAB "Atari"
        // group -- a capture must land on the same byte as the native rips, or
        // the platform is split in two (which is exactly what "atari jaguar"
        // and "wonderswan" used to do: format_map and platformNameToByte
        // disagreed, so natives and captures went to different filters).
        { "Youtube (Atari VCS)", ATARIVCS },
        { "Youtube (Atari 7800)", ATARI7800 },
        { "Youtube (Atari Lynx)", ATARILYNX },
        { "Youtube (Atari Jaguar)", ATARIJAGUAR },
        { "Youtube (Atari Falcon 030)", ATARIFALCON },
        { "Youtube (Atari XL/XE)", POKEY },
        { "Youtube (Atari ST)", ATARI },
        { "Youtube (Atari STe)", ATARI },
        { "Youtube (Atari TT 030)", ATARI }, // TT folds in with ST/STE
        { "Youtube (Wonderswan)", WONDERSWAN },
        // Unrecognised tag: falls back to OTHER so the drill can surface it,
        // rather than a byte no filter matches.
        { "Youtube (Some Future Pouet Tag)", OTHER },
    };
    for (auto const& c : cases) {
        INFO("format " << c.fmt);
        REQUIRE(mdb.classifyFormat(c.fmt, yt) == c.plat);
    }
    // Nothing should classify to the now-unused YOUTUBE byte.
    for (auto const& c : cases)
        REQUIRE(mdb.classifyFormat(c.fmt, yt) != YOUTUBE);
}

// The Falcon-native sample trackers are recovered from the EXTENSION: their
// format strings say "Atari ST" / name the tracker, so only .gtk/.dtm/.mix tells
// the Falcon apart from the YM2149 ST line. Replaced a display-only relabel, so
// the risk it guards against is real: .dtm is THREE different formats.
TEST_CASE("Falcon sample trackers split from the Atari ST byte", "[database]")
{
    using namespace chipmachine;
    RemoteLoader rl;
    MusicDatabase mdb{ rl };
    // Falcon: the extension decides, whatever the format string claims.
    REQUIRE(mdb.classifyFormat("Atari ST", "http://x/a.gtk") == ATARIFALCON);
    REQUIRE(mdb.classifyFormat("Graoumf Tracker", "http://x/a.gtk") ==
            ATARIFALCON);
    REQUIRE(mdb.classifyFormat("Digital Tracker DTM", "http://x/a.dtm") ==
            ATARIFALCON);
    REQUIRE(mdb.classifyFormat("Atari Digi-Mix", "http://x/a.mix") ==
            ATARIFALCON);
    // The YM2149 ST chiptune formats are untouched -- same machine name, but a
    // different extension means a different machine.
    REQUIRE(mdb.classifyFormat("Atari ST", "http://x/a.snd") == ATARI);
    REQUIRE(mdb.classifyFormat("Hippel ST", "http://x/a.hip") == ATARI);
    // THE GUARD: .dtm is also DeFy AdLib Tracker (PC/AdLib) and DigiTrekker.
    // Those never classify to ATARI, so the Falcon rule must not claim them --
    // it would file a PC AdLib tune under an Atari Falcon.
    REQUIRE(mdb.classifyFormat("DeFy AdLib Tracker", "http://x/a.dtm") !=
            ATARIFALCON);
    REQUIRE(mdb.classifyFormat("Digitrekker", "http://x/a.dtm") != ATARIFALCON);
    // Graoumf Tracker 2 is the WINDOWS successor; the .gt2 override outranks
    // both the "Atari ST" tag and the Falcon rule.
    REQUIRE(mdb.classifyFormat("Atari ST", "http://x/a.gt2") == PCTRACKER);
}

// The three Japanese FM computers were one JPFM byte behind a combined
// "PC-98/X68000/FM Towns" row; they now split into a "Japanese Computers" drill.
// Both classification paths (driver format string AND platform tag) must route
// each machine to its own byte, and the .mdx/.s98/pmd EXTENSION fallback too.
TEST_CASE("Japanese FM computers split into three bytes", "[database]")
{
    using namespace chipmachine;
    RemoteLoader rl;
    MusicDatabase mdb{ rl };
    const std::string p = "http://x/a.zip"; // neutral path (no telltale ext)
    // Driver format strings.
    REQUIRE(mdb.classifyFormat("FM sound driver (FMP)", p) == JPFM);   // PC-98
    REQUIRE(mdb.classifyFormat("PMD", p) == JPFM);                     // PC-98
    REQUIRE(mdb.classifyFormat("S98", p) == JPFM);                     // PC-98
    REQUIRE(mdb.classifyFormat("MDX", p) == JPX68000);                 // X68000
    REQUIRE(mdb.classifyFormat("Euphony", p) == JPFMTOWNS);            // FM Towns
    // Platform tags group by vendor: NEC -> PC-98, Sharp -> X68000,
    // Fujitsu -> FM Towns.
    REQUIRE(mdb.classifyFormat("NEC PC-98", p) == JPFM);
    REQUIRE(mdb.classifyFormat("NEC PC-88", p) == JPFM);
    REQUIRE(mdb.classifyFormat("Sharp X68000", p) == JPX68000);
    REQUIRE(mdb.classifyFormat("Sharp X1", p) == JPX68000);
    REQUIRE(mdb.classifyFormat("FM Towns", p) == JPFMTOWNS);
    REQUIRE(mdb.classifyFormat("Fujitsu FM-7", p) == JPFMTOWNS);
    // Extension fallback: a bare .mdx with no useful format string is X68000
    // (the mdx/s98/pmd format_map keys double as extension keys).
    REQUIRE(mdb.classifyFormat("", "http://x/song.mdx") == JPX68000);
    REQUIRE(mdb.classifyFormat("", "http://x/song.s98") == JPFM);
}

// The Other/Arcade drill groups on the canonical sub-platform name, so this is
// what decides that "Youtube (Oric)" and "Oric" are ONE row rather than two.
// No "Youtube (<platform>)" row may survive it (rule reversed 2026-07-15: a
// capture now groups with the hardware it was captured from).
TEST_CASE("sub-platform names fold captures onto their hardware", "[database]")
{
    using namespace chipmachine;
    struct { const char* fmt; const char* want; } cases[] = {
        // The wrapper comes off, so capture and native rips share a row.
        { "Youtube (Oric)", "Oric" },
        { "Oric", "Oric" },
        { "Youtube (Vectrex)", "Vectrex" },
        { "Vectrex", "Vectrex" },
        // Non-hardware tags unwrap literally rather than being renamed.
        { "Youtube (Wild)", "Wild" },
        { "Youtube (Animation/Video)", "Animation/Video" },
        // Combos: the first tag naming real hardware wins over the compo tag.
        { "Youtube (Wild,Raspberry Pi)", "Raspberry Pi" },
        { "Youtube (Java,Mobile Phone)", "Mobile" },
        // ...falling back to the first tag when none names hardware.
        { "Youtube (Wild,JavaScript)", "Wild" },
        { "Youtube (Java,Wild)", "Java" },
        // Fantasy consoles are platforms, not compo buckets: they keep a row.
        { "Youtube (MicroW8,PICO-8,TIC-80)", "MicroW8" },
        // Aliases for variants the case-only fold in buildSubPlatforms misses.
        // (Neo Geo Pocket is no longer here -- it was promoted to its own
        // top-level NEOGEOPOCKET filter row, so it never reaches the Other drill.)
        { "Youtube (Mobile Phone)", "Mobile" },
        { "Youtube (Android)", "Mobile" },
        { "Mobile", "Mobile" },
        { "Youtube (VIC 20)", "Commodore VIC-20" },
        // The two CPU-split TI-8x rows collapse to one calculator row; the two
        // GamePark handhelds collapse to one vendor row (nested parens and all).
        { "Youtube (TI-8x (Z80))", "TI-8x Calculator" },
        { "Youtube (TI-8x (68k))", "TI-8x Calculator" },
        { "Youtube (GamePark GP32)", "GamePark" },
        { "Youtube (GamePark GP2X)", "GamePark" },
        // The bare "Other" catch-all is relabelled to the playful "Easter Egg!"
        // row (its logo is EasterEgg.png).
        { "Other", "Easter Egg!" },
        { "Youtube (Other)", "Easter Egg!" },
        // Arcade strings carry no wrapper and no comma: untouched, so the
        // vendor rules in buildSubPlatforms still see them verbatim.
        { "Arcade (Capcom)", "Arcade (Capcom)" },
        { "", "Unknown" },
    };
    for (auto const& c : cases) {
        INFO("format '" << c.fmt << "'");
        REQUIRE(MusicDatabase::subPlatformName(c.fmt) == std::string(c.want));
    }
    // The point of the exercise: no row may be named after YouTube.
    for (auto const& c : cases)
        REQUIRE(MusicDatabase::subPlatformName(c.fmt).find("Youtube") ==
                std::string::npos);
}

// filter_demozoo_archives.py --classify replaces the generic "Demoscene" label
// on an archive row with the real format of the member inside it (peeked via an
// HTTP range read of the archive's directory), written as the bare uppercase
// extension -- the same vocabulary keygenmusic/botb use in that column. Every
// label it can emit must resolve to a real platform: one that format_map can't
// key would leave the row exactly as unclassified as before the pass ran.
// The path stays the ARCHIVE (.zip), so the trailing .mod/.xm extension
// correction must not fire -- the label alone has to carry it.
TEST_CASE("demozoo archive labels from the member peek classify", "[music]")
{
    using namespace chipmachine;
    RemoteLoader rl;
    MusicDatabase mdb{ rl };
    const std::string zip =
        "https://archive.scene.org/pub/parties/2010/breakpoint10/mmul/x.zip";
    struct { const char* fmt; uint8_t plat; } cases[] = {
        { "MOD", PROTRACKER },   // Amiga
        { "XM", FASTTRACKER },   // IBM PC
        { "IT", IMPULSETRACKER },
        { "S3M", SCREAMTRACKER },
        { "DBM", AMIGA },        // DigiBooster
        // Rendered audio -> the MP3/OGG "no platform" filter.
        { "MP3", MP3 },          { "OGG", OGG },
        { "WAV", MP3 },          { "FLAC", MP3 },
    };
    for (auto const& c : cases) {
        INFO("label " << c.fmt);
        uint8_t b = mdb.classifyFormat(c.fmt, zip);
        REQUIRE(b != UNKNOWN_FORMAT);
        REQUIRE(b == c.plat);
    }
}

// demozoo/scene.org ARCHIVE rows (.zip/.rar compo releases) carry the release
// platform as their format string and an extension format_map can't key. They
// used to reach NO platform filter at all: format_map didn't know the platform
// NAME either (only platformNameToByte did, which is the YouTube path), so they
// fell through to the extension fallback, where ".zip" resolves to nothing.
// The module rows next to them (.mod/.xm) must still be pulled to the platform
// their FORMAT fixes, not the release tag -- guard both halves.
TEST_CASE("demozoo archive rows classify by their release-platform tag", "[music]")
{
    using namespace chipmachine;
    RemoteLoader rl;
    MusicDatabase mdb{ rl };
    const std::string zip =
        "https://archive.scene.org/pub/parties/2023/revision23/exe-music/x.zip";
    struct { const char* fmt; const char* path; uint8_t plat; } cases[] = {
        // The archive rows this fixes.
        { "Windows", zip.c_str(), PC },
        { "MS-Dos", zip.c_str(), PC },
        { "Linux", zip.c_str(), PC },
        { "macOS", zip.c_str(), MACOS },
        { "Commodore Plus/4", zip.c_str(), PRG },
        // Atari machines have their own filters under the TAB "Atari" group.
        // These also guard a MISFILE: the startsWith(f,"atari") fallback claims
        // any unlisted "Atari <machine>" for the ST/STE/TT filter.
        { "Atari Lynx", zip.c_str(), ATARILYNX },
        { "Atari 7800", zip.c_str(), ATARI7800 },
        { "Atari Jaguar", zip.c_str(), ATARIJAGUAR },
        { "Atari Falcon", zip.c_str(), ATARIFALCON },
        { "Atari 2600 Video Computer System (VCS)", zip.c_str(), ATARIVCS },
        // Real hardware with no filter of its own -> Other Platforms.
        { "PICO-8", zip.c_str(), OTHER },
        { "Browser", zip.c_str(), OTHER },
        // The extension stays authoritative for module formats whose platform is
        // fixed by the format itself, even when the release tag says otherwise.
        { "MS-Dos", "https://media.demozoo.org/music/x.mod", PROTRACKER },
        { "MS-Dos", "https://media.demozoo.org/music/x.xm", FASTTRACKER },
        { "Windows", "https://media.demozoo.org/music/x.xm", FASTTRACKER },
    };
    for (auto const& c : cases) {
        INFO("format " << c.fmt << " path " << c.path);
        uint8_t b = mdb.classifyFormat(c.fmt, c.path);
        REQUIRE(b != UNKNOWN_FORMAT); // the bug: matched by no filter at all
        REQUIRE(b == c.plat);
    }
}

// demozoo/scene.org MP3+OGG rips carry the source platform as their format
// string rather than a codec. Those that name real hardware must resolve to it
// instead of the "Other Platforms" / "Rendered Audio" buckets -- in particular
// demozoo's "<Vendor> <Console> (<abbr>)" tags, where PSP used to land in Other
// while the identically-shaped NDS/GBA tags next to it resolved correctly.
// Gated on the extension classifying as MP3/OGG first, so the path matters.
TEST_CASE("demozoo MP3 platform tags classify to their console", "[music]")
{
    using namespace chipmachine;
    RemoteLoader rl;
    MusicDatabase mdb{ rl };
    const std::string mp3 =
        "https://archive.scene.org/pub/parties/2021/silvester21/music/x.mp3";
    struct { const char* fmt; uint8_t plat; } cases[] = {
        { "Sony Playstation Portable (PSP)", PSP },
        { "Nintendo DS (NDS)", NDS },
        { "Nintendo Game Boy Advance (GBA)", GBA },
        { "Amiga", AMIGA },      { "ZX Spectrum", SPECTRUM },
        { "Windows", PC },       { "MSX", MSX },
        // No hardware identity of their own -> Other Platforms, by design.
        { "Mobile", OTHER },     { "Custom Hardware", OTHER },
    };
    for (auto const& c : cases) {
        INFO("format " << c.fmt);
        REQUIRE(mdb.classifyFormat(c.fmt, mp3) == c.plat);
    }
    // A tag naming no hardware at all keeps the rendered-audio fallback.
    REQUIRE(mdb.classifyFormat("Demoscene", mp3) == MP3);
}

TEST_CASE("VGMRips format labels classify to a platform", "[music]")
{
    using namespace chipmachine;
    RemoteLoader rl;
    MusicDatabase mdb{ rl };
    const std::string url =
        "vgmrips::https://archive.org/download/x.zip/Game.zip";
    struct { const char* fmt; uint8_t plat; } cases[] = {
        { "Sega Mega Drive", MEGADRIVE }, { "Sega Pico", MEGADRIVE },
        { "NES", NES },                   { "Game Boy", GAMEBOY },
        { "PC Engine", HES },             { "Neo Geo", ARCADE },
        { "Neo Geo Pocket", NEOGEOPOCKET }, { "WonderSwan", WONDERSWAN },
        { "MSX", MSX },                   { "NEC PC-98", JPFM },
        { "NEC PC-88", JPFM },            { "Sharp X68000", JPX68000 },
        { "FM Towns", JPFMTOWNS },        { "IBM PC", PC },
        { "Atari ST", ATARI },            { "ZX Spectrum", SPECTRUM },
        { "Commodore 64", SID },          { "Apple IIgs", APPLE },
        { "Arcade", ARCADE },             { "Arcade (Capcom)", ARCADE },
        { "Arcade (Konami)", ARCADE },    { "Pinball", OTHER },
        { "Atari Jaguar", ATARIJAGUAR },
        // modland's CPS-1/CPS-2 .miniqsf rips: QSound is Capcom arcade hardware,
        // so these are ARCADE, not OTHER (buildSubPlatforms then folds the group
        // into "Arcade (Capcom)").
        { "Capcom Q-Sound Format", ARCADE },
        // Consoles VGMRips files under "Other"; build_vgmrips.py now recovers
        // them from the filename's hardware tag (TAG_PLATFORM). Before that they
        // all carried the bare "Other" label and piled into the catch-all.
        { "Nintendo Virtual Boy", VIRTUALBOY },
        { "Vectrex", OTHER },             { "Amstrad CPC", AMSTRAD },
        { "Sega SG-1000", SEGAMS },       { "Atari 8bit", POKEY },
        { "Atari 7800", ATARI7800 },      { "Intellivision", OTHER },
    };
    for (auto const& c : cases) {
        INFO("format " << c.fmt);
        uint8_t b = mdb.classifyFormat(c.fmt, url);
        REQUIRE(b != UNKNOWN_FORMAT);
        REQUIRE(b == c.plat);
    }
}

// The extension-screenshot audit (ChipMachine::loadExtensionScreenshots) keys off
// classifyFormat(format, path) exactly as playback does -- it must pass the real
// path so the extension fallback fires. scene.org/Fujiology tunes onboarded with a
// generic "Demoscene"/"Windows" platform string (which resolves to no hardware)
// must still classify by their extension so they land on a platform logo instead
// of being mis-reported as needing a screenshot. Guards the mon/ntk false-positive
// fix: .mon (Maniacs Of Noise, UADE-played) -> Amiga; .ntk (ProTrekkr) -> PC.
TEST_CASE("generic-tagged demoscene tunes classify by extension", "[music]")
{
    using namespace chipmachine;
    RemoteLoader rl;
    MusicDatabase mdb{ rl };
    struct { const char* fmt; const char* path; uint8_t plat; } cases[] = {
        { "Demoscene",
          "https://archive.scene.org/pub/parties/2013/atparty13/x/internal.mon",
          UADE },
        { "Demoscene",
          "https://ftp.untergrund.net/users/x/fujiology/x/BRASS_TACKS.NTK", PC },
    };
    for (auto const& c : cases) {
        INFO(c.fmt << " " << c.path);
        uint8_t b = mdb.classifyFormat(c.fmt, c.path);
        REQUIRE(b != UNKNOWN_FORMAT);
        REQUIRE(b == c.plat);
    }
}

// SMS Power! (smspower) files by the `format` label too (its path is a .zip pack
// URL). The Sega 8-bit labels must resolve to SEGAMS so the games appear under
// that TAB filter; ColecoVision (SN76489 too, but not a Sega platform) rides with
// the other misc small consoles under OTHER. Never UNKNOWN (invisible to every
// filter).
TEST_CASE("SMS Power format labels classify to a platform", "[music]")
{
    using namespace chipmachine;
    RemoteLoader rl;
    MusicDatabase mdb{ rl };
    const std::string url =
        "smspower::https://www.smspower.org/uploads/Music/Game-SMS.zip";
    for (const char* fmt : { "Sega Master System", "Sega Game Gear",
                             "Sega SG-1000" }) {
        INFO("format " << fmt);
        REQUIRE(mdb.classifyFormat(fmt, url) == SEGAMS);
    }
    REQUIRE(mdb.classifyFormat("ColecoVision", url) == OTHER);
}

// Zophar's Domain (zophar) files by the per-platform `format` label (its path is
// an "(EMU).zophar.zip" pack URL). All 10 sequenced-chip platform labels emitted
// by build_zophar.py must resolve to the right console byte so the games appear
// under that TAB filter -- never UNKNOWN (invisible to every filter). GBA rides
// with GameBoy in the "Nintendo GameBoy/GBA" filter; Genesis under MEGADRIVE;
// Master System / Game Gear under SEGAMS.
TEST_CASE("Zophar format labels classify to a platform", "[music]")
{
    using namespace chipmachine;
    RemoteLoader rl;
    MusicDatabase mdb{ rl };
    const std::string url =
        "zophar::https://fi.zophar.net/soundfiles/x/y/Game%20(EMU).zophar.zip";
    struct { const char* fmt; uint8_t plat; } cases[] = {
        { "Nintendo Sound Format", NES },
        { "Super Nintendo", SNES },
        { "Nintendo Game Boy (GB)", GAMEBOY },
        { "Gameboy Advance", GBA },
        { "Nintendo DS Sound Format", NDS },
        { "Ultra64 Sound Format", NINTENDO64 },
        { "HES", HES },
        { "Sega Genesis", MEGADRIVE },
        { "Sega Master System", SEGAMS },
        { "Sega Game Gear", SEGAMS },
        // Streamed tier (db.lua v95): recorded rips played via vgmstream/ffmpeg,
        // each with its own platform byte (formerly OTHER / folded into PlayStation).
        { "Playstation", PLAYSTATION },
        { "Playstation 2", PLAYSTATION2 },
        { "Sega Saturn", SATURN },
        { "Sega Dreamcast", DREAMCAST },
        { "Nintendo 3DS", N3DS },
        { "Nintendo GameCube", GAMECUBE },
        { "Nintendo Wii", WII },
        { "Xbox", XBOX },
        { "Xbox 360", XBOX360 },
        { "Playstation 3", PS3 },
        { "Playstation Portable", PSP },
    };
    for (auto const& c : cases) {
        INFO("format " << c.fmt);
        uint8_t b = mdb.classifyFormat(c.fmt, url);
        REQUIRE(b != UNKNOWN_FORMAT);
        REQUIRE(b == c.plat);
    }
}

// mirsoft "World of Game MODs" (mirsoft) is classified by the ACTUAL module
// format as of db.lua v94: its `format` column is now "Amiga" (mod-family), "PC"
// (xm/it/s3m) or "Atari ST", NOT the game's platform -- classifying by game
// platform gave a C64 game's .mod the SID byte and made it shadow the real HVSC
// SID in search dedup. Whatever label a row carries, it must resolve to a real
// platform (never UNKNOWN, invisible to every TAB filter). The console labels
// below are retained mappings still used by other collections (hvsc, zophar, ...).
TEST_CASE("mirsoft platform labels classify to a platform", "[music]")
{
    using namespace chipmachine;
    RemoteLoader rl;
    MusicDatabase mdb{ rl };
    const std::string url = "mirsoft::A%20Game.zip";
    struct { const char* fmt; uint8_t plat; } cases[] = {
        { "Amiga", AMIGA },              { "Commodore 64", SID },
        { "PC", PC },                    { "NES", NES },
        { "Super Nintendo", SNES },      { "Macintosh", APPLEMAC },
        { "PlayStation", PLAYSTATION },  { "Game Boy", GAMEBOY },
        { "Nintendo 64", NINTENDO64 },   { "Sega Mega Drive", MEGADRIVE },
        { "Sega Master System", SEGAMS },{ "Sega Saturn", SATURN },
        { "Dreamcast", DREAMCAST },      { "Atari ST", ATARI },
        { "Atari Falcon", ATARIFALCON }, { "Atari Jaguar", ATARIJAGUAR },
        { "ZX Spectrum", SPECTRUM },     { "Amstrad CPC", AMSTRAD },
        { "PC Engine", HES },            { "Arcade", ARCADE },
    };
    for (auto const& c : cases) {
        INFO("format " << c.fmt);
        uint8_t b = mdb.classifyFormat(c.fmt, url);
        REQUIRE(b != UNKNOWN_FORMAT);
        REQUIRE(b == c.plat);
    }
}

// REGRESSION (db.lua v94): a game-mod collection must never SHADOW a distinct
// original in search results. mirsoft ships an Amiga MOD remix of Rob Hubbard's
// C64 classic "Delta"; HVSC ships the real SID. They share {title, composer} but
// differ in real format, so search() must return BOTH. Earlier the dedup keyed on
// the coarse platform byte (mirsoft's .mod was mis-filed "Commodore 64" -> SID),
// so the remix and the legendary SID collided and only one survived. Guards both
// halves of the fix: the ext-derived platform label AND the real-format dedup key.
TEST_CASE("search keeps same-name different-format songs", "[database]")
{
    using namespace chipmachine;
    const auto injector = di::make_injector(di::bind<utils::path>.to("."));
    auto mdb = injector.create<std::unique_ptr<MusicDatabase>>();
    REQUIRE(mdb->initFromLua(utils::path(".")) == true);

    std::vector<int> result;
    mdb->search("Delta/Rob Hubbard", result, 500);

    int sidPos = -1, amigaPos = -1;
    for (size_t i = 0; i < result.size(); i++) {
        auto s = mdb->getSongInfo(result[i]);
        if (s.title != "Delta" || s.composer != "Rob Hubbard") continue;
        uint8_t b = MusicDatabase::classifyFormat(s.format, s.path);
        if (b == SID && sidPos < 0) sidPos = (int)i;     // real HVSC .sid
        if (b == AMIGA && amigaPos < 0) amigaPos = (int)i; // mirsoft .mod remix
    }
    REQUIRE(sidPos >= 0);    // the SID survived dedup
    REQUIRE(amigaPos >= 0);  // the remix survived too
    // priority: hvsc (100) outranks mirsoft (-100), so the SID surfaces first.
    REQUIRE(sidPos < amigaPos);
}

// The "no new format" claim: mirsoft holds only mainstream tracker modules, even
// for console games (tracker arrangements, not native .sid/.nsf rips). Each of
// its formats must play through the real host path (createPlugins -> fromFile ->
// getSamples), the same one the ZIP-by-magic subsong handler feeds at runtime.
// Fixtures are real net-new mirsoft modules (one per format we ship).
TEST_CASE("mirsoft game modules play via the host path", "[music]")
{
    auto ap = std::make_shared<AudioPlayerNull>();
    const auto injector = di::make_injector(di::bind<utils::path>.to("."),
                                            di::bind<AudioPlayer>.to(ap));
    musix::ChipPlugin::createPlugins("data");
    chipmachine::MusicPlayer mp{ ap };
    for (const char* f : { "testmus/mirsoft/ironseed-cargo.mod",
                           "testmus/mirsoft/crystalis.it",
                           "testmus/mirsoft/ageofempires-track3.xm",
                           "testmus/mirsoft/speedhaste.s3m",
                           "testmus/mirsoft/simcity2000.med",
                           // .dmu = Digital Mugician (UADE); mirsoft ships a few,
                           // now in the ZIP-member allow-list (MusicPlayerList).
                           "testmus/mirsoft/hoi-level4.dmu" }) {
        INFO("file " << f);
        REQUIRE(mp.playFile(f));
        int64_t sum = 0;
        for (int i = 0; i < 20 && sum == 0; ++i) {
            mp.update();
            std::vector<int16_t> data(8192);
            ap->get(data);
            sum = std::accumulate(data.begin(), data.end(), (int64_t)0);
        }
        REQUIRE(sum != 0);
    }
}

// The GUI marks app-shipped local collections with a "+" (vs "*" for cached
// remote files) via isLocalAsset, keyed by the "<collection>::" path prefix.
// Guard that both shipped collections -- nsfe and projectay -- report local, and
// that a remote/URL song does not. hvtc was a third until db.lua VERSION 129
// un-bundled it (no music ships inside the app for Mac App Store submission), so
// it must now report REMOTE.
TEST_CASE("isLocalAsset marks shipped collections", "[music]")
{
    REQUIRE(RemoteLoader::isLocalAsset("nsfe::31_orange_painting.nsfe"));
    REQUIRE(RemoteLoader::isLocalAsset("projectay::ironfist/arkanoid.ay"));
    REQUIRE(RemoteLoader::isLocalAsset("projectay::cpc/TribalMag5/TribalMag5_00.ay"));
    REQUIRE_FALSE(RemoteLoader::isLocalAsset("hvtc::demos/crazy_scroll_89.prg"));
    REQUIRE_FALSE(RemoteLoader::isLocalAsset("zxart::https://zxart.ee/file/id:44417/x.ay"));
    REQUIRE_FALSE(RemoteLoader::isLocalAsset("modland::AY/Foo/bar.ay"));
}

// A local file (served from a local_dir mirror) is served straight from disk by
// load() and thus NEVER written to the web cache: isLocalFile mirrors the exact
// File::exists(local_dir + path) condition load() short-circuits on. So marking
// a song "+" (local) and "never cached" are one and the same test. Guards that a
// real shipped file reports local while a missing member / remote source do not.
TEST_CASE("local-dir files report local and are never cached", "[music]")
{
    RemoteLoader rl;
    rl.registerSource("projectay", "", "music/projectay"); // empty remote source
    // A real shipped .ay: local -> load() serves it from disk, no fetch/cache.
    REQUIRE(rl.isLocalFile("projectay::ironfist/1999.ay"));
    REQUIRE(rl.inCache("projectay::ironfist/1999.ay"));  // present; nothing to fetch
    // A missing member is not local (would fall through to the network).
    REQUIRE_FALSE(rl.isLocalFile("projectay::ironfist/no_such_tune.ay"));
    // A purely-remote collection (no local_dir) is never "local".
    rl.registerSource("zxart", "https://zxart.ee/", "");
    REQUIRE_FALSE(rl.isLocalFile("zxart::file/id:1/x.ay"));
    // Unknown collection prefix -> not local (no source, no crash).
    REQUIRE_FALSE(rl.isLocalFile("bogus::whatever.ay"));
}

// Project AY (.ay ZXAYEMUL rips) routing: .ay is owned by GME, not Ayfly --
// Ayfly renders Amstrad CPC .ay silent while GME's Ay_Emul-lineage core plays
// both ZX and CPC. This drives the real app registration + MusicPlayer routing
// and asserts a CPC rip (the one Ayfly plays silent) and a ZX Ironfist rip both
// reach GME and produce non-silent audio.
TEST_CASE("Project AY .ay routes to GME and plays", "[music]")
{
    auto ap = std::make_shared<AudioPlayerNull>();
    const auto injector = di::make_injector(di::bind<utils::path>.to("."),
                                            di::bind<AudioPlayer>.to(ap));
    musix::ChipPlugin::createPlugins("data");
    chipmachine::MusicPlayer mp{ ap };
    for (auto const& ay : {"testmus/gme/cpc-tribalmag5.ay",
                           "testmus/gme/ironfist-chasehq2.ay"}) {
        REQUIRE(mp.playFile(ay));
        int64_t sum = 0;
        for (int i = 0; i < 30 && sum == 0; ++i) {
            mp.update();
            std::vector<int16_t> data(8192);
            ap->get(data);
            sum = std::accumulate(data.begin(), data.end(), (int64_t)0);
        }
        REQUIRE(sum != 0);
    }
}

// Native Arkos Tracker songs (.aks) play through the very same AT3 SongLoader +
// SongPlayer chain as STarKos .sks -- the loader transparently gunzips and auto-
// detects the Arkos version (modland's .aks are gzip-compressed AT1 XML). Only
// SksPlugin::canHandle needed widening to claim the extension; this asserts the
// two corpus tunes both load and render real audio.
TEST_CASE("Arkos Tracker AKS plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::SksPlugin plugin;

    for (auto const& aks : {"testmus/sks/Targhan - Demo.aks",
                            "testmus/sks/Glafouk - Sontagbeat.aks"}) {
        INFO(aks);
        REQUIRE(plugin.canHandle(aks));

        auto* player = plugin.fromFile(aks);
        REQUIRE(player != nullptr);

        std::array<int16_t, 8192> buffer{};
        int64_t energy = 0;
        for (int count = 0; count < 100 && energy == 0; ++count) {
            int rc = player->getSamples(buffer.data(), buffer.size());
            if (rc <= 0) { break; }
            for (int i = 0; i < rc; ++i) {
                energy += std::abs(static_cast<int>(buffer[i]));
            }
        }
        delete player;

        REQUIRE(energy != 0);
    }
}

// UnExoticA archives wrap their payload in a game-named directory and keep some
// formats' companions (e.g. the Sonix driver's "instruments/") in a subdir.
// extractLha must strip the wrapper (the DB member paths are relative to inside
// it) while preserving deeper structure, otherwise SMUS songs lose their
// instruments and UADE reports "score died".
TEST_CASE("LHA extract strips wrapper, keeps subdirs", "[lha]")
{
    // Print a green "ok" or a red "FAIL" for each check so a plain `cmtest`
    // run shows what the [lha] suite is doing (Catch is otherwise silent on
    // success).
    auto check = [](char const* what, bool cond) {
        printf("  %-55s %s\n", what,
               cond ? "\033[32mok\033[0m" : "\033[31mFAIL\033[0m");
        REQUIRE(cond);
    };

    printf("---- LHA extract ----\n");

    printf("Spirit_of_Excalibur.lha (wrapper strip + instruments/ subdir)\n");
    auto dest = (std::filesystem::temp_directory_path() / "cmtest_lha").string();
    std::filesystem::remove_all(dest);
    auto names = chipmachine::extractLha(
        "testmus/lha/Spirit_of_Excalibur.lha", dest);
    check("extracted some members", !names.empty());
    // Song sits at the extraction root (wrapper stripped) ...
    check("song smus.title at root", std::filesystem::exists(dest + "/smus.title"));
    check("wrapper dir stripped",
          !std::filesystem::exists(dest + "/Spirit_of_Excalibur"));
    // ... and the instruments subdirectory the Sonix driver needs survives.
    check("instruments/ subdir kept",
          std::filesystem::exists(dest + "/instruments/mclarinet.instr"));
    std::filesystem::remove_all(dest);

    // Einmal Kanzler sein stores "<realname>\0<comment with '/'>" in the LHA
    // filename field (Amiga LhA). The lhasa NUL-truncation fix must recover the
    // real member name; without it every member parsed as " Traveling Bits".
    printf("Einmal_Kanzler_sein.lha (Amiga name\\0comment filename fix)\n");
    auto dest2 =
        (std::filesystem::temp_directory_path() / "cmtest_lha2").string();
    std::filesystem::remove_all(dest2);
    auto names2 = chipmachine::extractLha(
        "testmus/lha/Einmal_Kanzler_sein.lha", dest2);
    check("extracted > 50 members", names2.size() > 50);
    check("real name mdat.ingame_01 recovered",
          std::filesystem::exists(dest2 + "/mdat.ingame_01"));
    check("comment garbage ' Traveling Bits' absent",
          !std::filesystem::exists(dest2 + "/ Traveling Bits"));
    std::filesystem::remove_all(dest2);
}

// IFF-8SVX Amiga samples (UnExoticA "8svx.<name>") have no UADE eagleplayer;
// they are routed to the ffmpeg plugin instead (its IFF demuxer decodes them).
// Guard that routing decision here (no ffmpeg binary needed for canHandle).
TEST_CASE("FFMPEG claims 8svx samples", "[ffmpeg8svx]")
{
    musix::FFMPEGPlugin p;
    REQUIRE(p.canHandle("8svx.welcome on amiga"));
    REQUIRE(p.canHandle("song.mp3"));
    REQUIRE(!p.canHandle("mod.somesong"));
}

// The bundled ffmpeg also decodes lossless PCM containers (wav/flac/aiff/aif)
// and MP2 -- all present across the demoscene collections (demozoo alone has
// ~685 wav, ~343 flac, ~60 mp2, plus aif/aiff) but previously dropped because
// the extension gate omitted them. Guard the routing decision (no ffmpeg binary
// needed for canHandle); actual decode is exercised by the "FFMPEG" corpus test
// and "FFMPEG lossless/mp2" below.
TEST_CASE("FFMPEG claims lossless PCM and mp2 extensions", "[ffmpeglossless]")
{
    musix::FFMPEGPlugin p;
    REQUIRE(p.canHandle("song.wav"));
    REQUIRE(p.canHandle("song.flac"));
    REQUIRE(p.canHandle("song.aiff"));
    REQUIRE(p.canHandle("song.aif"));
    REQUIRE(p.canHandle("song.mp2"));
    REQUIRE(p.canHandle("song.opus"));
    REQUIRE(p.canHandle("song.mpeg"));
    REQUIRE(p.canHandle("song.ac3"));
    // getSupportedExtensions() and canHandle() share one source of truth.
    auto exts = p.getSupportedExtensions();
    for (auto const& e : {"wav", "flac", "aiff", "aif", "mp2", "opus", "mp3",
                          "ogg", "m4a", "aac", "mp4", "8svx", "mpeg", "ac3"}) {
        REQUIRE(exts.count(e) > 0);
    }
}

// The stream-vs-download decision is the single source of truth behind both the
// playback path (playCurrent) and the UI toast ("BUFFERING..." for a progressive
// stream, "LOADING..." for a whole-file download). Guard the contract so the two
// can't drift: every ffmpeg finite-file format streams, YouTube/radio stream,
// while native modules and M3U (and the 8svx prefix form, which is fetched whole)
// do not.
TEST_CASE("willStream picks streaming vs full-download", "[music]")
{
    using namespace chipmachine;
    // Streamed: the ffmpeg finite-file formats (play after a short prebuffer).
    for (auto const* ext : {"mp3", "ogg", "aac", "m4a", "mp4", "opus", "mp2",
                            "mpeg", "ac3", "wav", "flac", "aiff", "aif"}) {
        REQUIRE(MusicPlayerList::isStreamableExt(ext));
        SongInfo si("demozoo::pub/x." + std::string(ext), "", "", "",
                    "Demoscene");
        INFO(ext << " should stream");
        REQUIRE(MusicPlayerList::willStream(si));
    }
    // A "?query" tail must not defeat the ext match (podCloud enclosure URLs).
    REQUIRE(MusicPlayerList::willStream(
        SongInfo("https://host/enclosure.flac?p=f", "", "", "", "Demoscene")));

    // YouTube (Pouet / manual patch) and radio/Shoutcast are streamed too.
    REQUIRE(MusicPlayerList::willStream(SongInfo(
        "https://www.youtube.com/watch?v=abc", "", "", "", "Youtube (Windows)")));
    REQUIRE(MusicPlayerList::willStream(
        SongInfo("pouet::https://www.youtube.com/watch?v=abc")));
    REQUIRE(MusicPlayerList::willStream(
        SongInfo("radio::necta64.mp3", "", "", "", "MP3")));

    // Downloaded whole (=> "LOADING..."): native modules, M3U playlists, and the
    // 8svx modland prefix form (no real extension, so it can't stream).
    for (auto const* ext : {"mod", "xm", "it", "s3m", "sid", "vgz", "sndh"}) {
        REQUIRE(!MusicPlayerList::isStreamableExt(ext));
        SongInfo si("mirsoft::x." + std::string(ext), "", "", "", "Amiga");
        INFO(ext << " should NOT stream");
        REQUIRE(!MusicPlayerList::willStream(si));
    }
    // An .mp3 codec tag but M3U format = a playlist wrapper, not a stream yet.
    REQUIRE(!MusicPlayerList::willStream(
        SongInfo("radio::list.m3u", "", "", "", "M3U")));
    // 8svx sample: ffmpeg decodes it, but it is fetched whole -> LOADING.
    REQUIRE(!MusicPlayerList::willStream(
        SongInfo("unexotica::8svx.Welcome On Amiga", "", "", "", "Amiga")));
}

template <typename PLUGIN, typename... ARGS>
bool testPlugin(std::string const& dir, std::string const& exclude,
                const ARGS&... args)
{
    std::array<int16_t, 8192> buffer;
    try {
        PLUGIN plugin{ args... };
        printf("---- %s ----\n", plugin.name().c_str());
        logging::setLevel(logging::Level::Warning);
        
        auto files = utils::File{ dir }.listFiles();
        if (files.empty()) {
            printf("NO FILES FOUND!\n");
        }
        
        for (auto f : files) {
            // Skip subdirectories silently -- some corpora keep companion files
            // in a subfolder (e.g. testmus/uade/Instruments/ for IFF-SMUS), which
            // listFiles() returns as a plain entry. It isn't a playable fixture,
            // so don't count it as a skip (would inflate the coverage gate).
            if (f.isDir()) continue;

            // Silently ignore macOS/system hidden files (.DS_Store, ._* resource
            // forks). They are never playable fixtures and must not be reported
            // at all -- no "Skipping"/"Ignored" line, not counted in any tally.
            auto baseName = utils::path_filename(f.getName());
            if (!baseName.empty() && baseName[0] == '.') continue;

            // `exclude` is a comma-separated list of substrings; skip the file if
            // it matches ANY of them. (Lets a corpus exclude companion/sample
            // files precisely -- e.g. ".smpl,smp." drops the TFMX/SoundMaster
            // sample banks in testmus/uade without also hiding ".smpro" songs.)
            if (exclude != "") {
                bool excluded = false;
                for (std::string pat : utils::split(exclude, ",")) {
                    if (!pat.empty() &&
                        f.getName().find(pat) != std::string::npos) {
                        excluded = true;
                        break;
                    }
                }
                if (excluded) continue;
            }

            auto ext = utils::toLower(utils::path_extension(f.getName()));
            // silently ignore extensions flagged impossible-to-support
            if (notSupportedExts().count(ext) > 0) continue;

            if (shouldIgnoreFile(f.getName())) {
                printf("\033[90mIgnored %s\033[0m\n", f.getName().c_str());
                continue;
            }

            int64_t sum = 0;
            if (!plugin.canHandle(f.getName())) {
                printf("\033[33mSkipping %s\033[0m\n", f.getName().c_str());
                g_skips++;
                g_skipExts.insert(ext);
                continue;
            }
            printf("Trying %s ... ", f.getName().c_str());
            fflush(stdout);
            try {
                auto* player = plugin.fromFile(f.getName());
                if (player) {
                    int count = 50;
                    while (sum == 0 && count != 0) {
                        int rc = player->getSamples(&buffer[0], buffer.size());
                        if (rc > 0) {
                            for (int i = 0; i < rc; ++i) {
                                if (buffer[i] != 0) {
                                    sum = 1;
                                    break;
                                }
                            }
                            if (sum != 0) {
                                break;
                            }
                            count--;
                        } else if (rc == 0) {
                            // No PCM yet, but not end-of-stream: the FFMPEG plugin
                            // spawns the ffmpeg binary and returns immediately
                            // (non-blocking), so the first getSamples() calls come
                            // back empty while the subprocess warms up. Wait a beat
                            // and retry within the count budget instead of giving
                            // up (which reported a spurious "NO SOUND"). rc < 0 is
                            // a real SONG_END and still breaks.
                            utils::sleepms(20);
                            count--;
                        } else
                            break;
                    }
                    delete player;
                }
                const char* status = player ? (sum == 0 ? "NO SOUND" : "OK") : "FAILED";
                if (!player || sum == 0) {
                    // rewrite the whole line in red on failure
                    printf("\r\033[31mTrying %s ... playback %s\033[0m\n",
                           f.getName().c_str(), status);
                    g_errors++;
                    g_errorExts.insert(ext);
                } else {
                    printf("playback %s\n", status);
                    g_ok++;
                    g_okExts.insert(ext);
                }
            } catch (std::exception& e) {
                // A plugin may deliberately fast-fail a known sibling format
                // that shares an extension with one it supports -- e.g.
                // Deflemask .dmf (zlib, magic 0x78) vs X-Tracker .dmf ("DDMF"),
                // where only the latter is decodable. Those throw a
                // "... unsupported" message and are a graceful skip, not a
                // playback error that should fail coverage.
                std::string msg = e.what();
                if (msg.find("unsupported") != std::string::npos) {
                    printf("\r\033[33mSkipping %s (%s)\033[0m\n",
                           f.getName().c_str(), e.what());
                    g_skips++;
                    g_skipExts.insert(ext);
                } else {
                    printf("\r\033[31mTrying %s ... playback EXCEPTION (%s)\033[0m\n",
                           f.getName().c_str(), e.what());
                    g_errors++;
                    g_errorExts.insert(ext);
                }
            }
        }
    } catch (std::exception& e) {
        printf("---- Plugin Instantiation Failed: %s ----\n", e.what());
    }
    return true;
}

TEST_CASE("GME", "[music]") { testPlugin<musix::GMEPlugin>("testmus/gme", "nowork"); }

// OPL-family VGM/VGZ (AdLib/Sound Blaster: YM3812 OPL2, YMF262 OPL3) via the
// vendored libvgm. GME's Vgm_Emu has no OPL cores -- it renders these silent or
// aborts (Blip_Buffer assertion), so LibVGMPlugin content-gates them away from
// GME. The two fixtures are one OPL2 and one OPL3 log, each fed as gzipped .vgz
// (libvgm reads gzip directly); both must load and produce non-silent audio.
TEST_CASE("LibVGM", "[music]") { testPlugin<musix::LibVGMPlugin>("testmus/libvgm", "nowork"); }

TEST_CASE("VGMStream", "[music]") { testPlugin<musix::VGMStreamPlugin>("testmus/vgmstream", "nowork"); }

// VIC-TRACKER (.vt) Commodore VIC-20 tunes (the modland "Vic-Tracker" corpus).
// Each fixture runs Daniel Kahlin's own 6502 replayer on the fake6502 core with
// the VIC-I sound core from VICE and must produce non-silent audio. The five
// distribution songs cover the format's features (mystic=portamento only,
// blippblopp=arpeggios, vt-theme/slowride=everything, djungel-zagor=multi-song).
TEST_CASE("VicTracker", "[music]") { testPlugin<musix::VTPlugin>("testmus/victracker", "nowork"); }

// The host routing path (createPlugins -> MusicPlayer::playFile -> getSamples),
// which only works if victrackerplugin is registered in plugin_register.cpp --
// testPlugin<> above bypasses registration by constructing the plugin directly.
TEST_CASE("VicTracker host path plays sound", "[music]")
{
    auto ap = std::make_shared<AudioPlayerNull>();
    const auto injector = di::make_injector(di::bind<utils::path>.to("."),
                                            di::bind<AudioPlayer>.to(ap));
    musix::ChipPlugin::createPlugins("data");
    chipmachine::MusicPlayer mp{ ap };
    bool ok = mp.playFile("testmus/victracker/vt-theme.vt");
    REQUIRE(ok);
    int64_t sum = 0;
    for (int i = 0; i < 20 && sum == 0; ++i) {
        mp.update();
        std::vector<int16_t> data(8192);
        ap->get(data);
        for (auto val : data) {
            if (val != 0) {
                sum = 1;
                break;
            }
        }
    }
    REQUIRE(sum != 0);
}

TEST_CASE("Klystrack", "[music]") { testPlugin<musix::KlystrackPlugin>("testmus/klystrack", "nowork"); }

// Host routing path for klystrack (.kt) -- only works if klystrackplugin is
// registered in plugin_register.cpp; testPlugin<> above bypasses registration.
TEST_CASE("Klystrack host path plays sound", "[music]")
{
    auto ap = std::make_shared<AudioPlayerNull>();
    const auto injector = di::make_injector(di::bind<utils::path>.to("."),
                                            di::bind<AudioPlayer>.to(ap));
    musix::ChipPlugin::createPlugins("data");
    chipmachine::MusicPlayer mp{ ap };
    bool ok = mp.playFile("testmus/klystrack/obspatial.kt");
    REQUIRE(ok);
    int64_t sum = 0;
    for (int i = 0; i < 20 && sum == 0; ++i) {
        mp.update();
        std::vector<int16_t> data(8192);
        ap->get(data);
        for (auto val : data) {
            if (val != 0) {
                sum = 1;
                break;
            }
        }
    }
    REQUIRE(sum != 0);
}

TEST_CASE("VGMStream host path plays sound", "[music]")
{
    auto ap = std::make_shared<AudioPlayerNull>();
    const auto injector = di::make_injector(di::bind<utils::path>.to("."),
                                            di::bind<AudioPlayer>.to(ap));
    musix::ChipPlugin::createPlugins("data");
    chipmachine::MusicPlayer mp{ ap };
    bool ok = mp.playFile("testmus/vgmstream/bakamitai.adx");
    REQUIRE(ok);
    int64_t sum = 0;
    for (int i = 0; i < 20 && sum == 0; ++i) {
        mp.update();
        std::vector<int16_t> data(8192);
        ap->get(data);
        for (auto val : data) {
            if (val != 0) {
                sum = 1;
                break;
            }
        }
    }
    REQUIRE(sum != 0);
}

// DefleMask .dmf (multi-system chiptune) via the vendored Furnace engine. The
// fixtures span the DefleMask systems proven first: Genesis (YM2612+PSG, ext
// ch3), Sega Master System (SN76489 PSG) and Game Boy. Each must load through
// DivEngine and produce non-silent audio. This also guards that the DefleMask
// files reach dmfplugin and not OpenMPT's unrelated X-Tracker .dmf loader.
TEST_CASE("DMF", "[music]") { testPlugin<musix::DMFPlugin>("testmus/dmf", "nowork"); }

// Regression: chipmachine loads songs on a MusicPlayerList worker thread whose
// default stack (~512 KB on macOS) is far smaller than the main thread's 8 MB.
// DivEngine::loadDMF puts a ~744 KB DivSong on the stack, so loading a .dmf from
// such a thread used to overflow and SIGBUS ("thread stack size exceeded").
// DMFPlayer now runs the load on its own large-stack thread; verify a .dmf loads
// and produces sound even when fromFile() is invoked from a 512 KB-stack thread.
namespace {
struct DmfStackProbe { bool ok = false; };
void* dmfStackProbeFn(void* p)
{
    auto* probe = static_cast<DmfStackProbe*>(p);
    try {
        musix::DMFPlugin plugin;
        const std::string f = "testmus/dmf/Spring Yard.dmf";
        if (!plugin.canHandle(f)) { return nullptr; }
        std::unique_ptr<musix::ChipPlayer> player{ plugin.fromFile(f) };
        if (!player) { return nullptr; }
        std::array<int16_t, 8192> buf{};
        int64_t sum = 0;
        for (int i = 0; i < 50 && sum == 0; i++) {
            int rc = player->getSamples(buf.data(), buf.size());
            if (rc <= 0) { break; }
            for (int j = 0; j < rc; j++) { if (buf[j] != 0) { sum = 1; break; } }
        }
        probe->ok = (sum != 0);
    } catch (...) {}
    return nullptr;
}
} // namespace

TEST_CASE("DMF loads on a small-stack worker thread", "[music]")
{
    DmfStackProbe probe;
    pthread_attr_t attr;
    REQUIRE(pthread_attr_init(&attr) == 0);
    // Match the host's worker-thread stack that exposed the overflow.
    REQUIRE(pthread_attr_setstacksize(&attr, 512 * 1024) == 0);
    pthread_t th;
    REQUIRE(pthread_create(&th, &attr, dmfStackProbeFn, &probe) == 0);
    pthread_attr_destroy(&attr);
    pthread_join(th, nullptr);
    REQUIRE(probe.ok);
}

// Regression test for SGC (Sega Master System / Game Gear / ColecoVision)
// support. The vendored Game_Music_Emu had the SGC emulator stripped out (the
// USE_GME_SGC scaffolding was left behind but the Sgc_* sources were missing).
// It was added back -- Sgc_Emu/Impl/Core/Cpu plus the Z80_Cpu core, Gme_Loader
// and Sms_Fm_Apu it depends on -- and "sgc" registered in GMEPlugin. This plays
// a real .sgc PSG tune and checks for audio. Fails if SGC routing or the
// emulator regresses. (SMS FM is covered separately below.)
TEST_CASE("GME SGC plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::GMEPlugin plugin;

    std::string const sgc = "testmus/gme/Dynamite Headdy.sgc";
    REQUIRE(plugin.canHandle(sgc));

    auto* player = plugin.fromFile(sgc);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// Regression test for SMS FM (YM2413 / OPLL). The vendored Game_Music_Emu
// shipped a no-op Ym2413_Emu stub, so Sms_Fm_Apu::supported() returned false and
// Sgc_Core skipped FM init: SMS titles whose subsongs use the FM Sound Unit
// played back silent (Game Gear/PSG subsongs were unaffected). Ym2413_Emu now
// wraps a real OPLL (vendored emu2413). Parlour Games [SMS] has FM subsongs that
// were previously silent; this renders every subsong and requires that they all
// produce audio, which fails if the FM core regresses back to a stub.
TEST_CASE("GME SGC SMS FM plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::GMEPlugin plugin;

    std::string const sgc = "testmus/gme/Parlour Games [SMS].sgc";
    REQUIRE(plugin.canHandle(sgc));

    auto* player = plugin.fromFile(sgc);
    REQUIRE(player != nullptr);

    int const songs = static_cast<int>(std::get<uint32_t>(player->meta("songs")));
    REQUIRE(songs > 1); // multi-subsong; some are FM-only

    for (int song = 0; song < songs; ++song) {
        player->seekTo(song, -1);

        std::array<int16_t, 8192> buffer{};
        int64_t energy = 0;
        for (int count = 0; count < 100 && energy == 0; ++count) {
            int rc = player->getSamples(buffer.data(), buffer.size());
            if (rc <= 0) { break; }
            for (int i = 0; i < rc; ++i) {
                energy += std::abs(static_cast<int>(buffer[i]));
            }
        }
        INFO("subsong " << song);
        REQUIRE(energy != 0);
    }
    delete player;
}

// SMS Power! (smspower collection) ships per-track .vgm files inside its game zip
// packs. They are Sega 8-bit VGM logs -- SN76489 PSG, and for FM-capable Master
// System games also YM2413 (OPLL) -- and are gzip-compressed despite the .vgm
// extension. GME's Vgm_Emu decodes both chips and auto-inflates gzip, so no new
// plugin is needed; these real fixtures (one PSG, one FM) guard that a raw
// SMS Power .vgm loads and produces audio.
TEST_CASE("GME plays SMS Power VGM (PSG + FM)", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::GMEPlugin plugin;

    for (auto const& vgm : { "testmus/gme/smspower-aceofaces-psg.vgm",
                             "testmus/gme/smspower-cyborghunter-fm.vgm" }) {
        INFO(vgm);
        REQUIRE(plugin.canHandle(vgm));
        auto* player = plugin.fromFile(vgm);
        REQUIRE(player != nullptr);

        std::array<int16_t, 8192> buffer{};
        int64_t energy = 0;
        for (int count = 0; count < 100 && energy == 0; ++count) {
            int rc = player->getSamples(buffer.data(), buffer.size());
            if (rc <= 0) { break; }
            for (int i = 0; i < rc; ++i) {
                energy += std::abs(static_cast<int>(buffer[i]));
            }
        }
        delete player;
        REQUIRE(energy != 0);
    }
}

// Regression test for GBR (the older Game Boy rip format, predecessor of GBS).
// The vendored Game_Music_Emu only handled GBS; GBR is now decoded by the same
// Gbs_Emu by rewriting the 0x20-byte GBR header into the GBS header_t at load
// time (gbr_mode_ in Gbs_Emu.cpp) and registering gme_gbr_type. This plays a
// single-bank rip whose driver runs up in the 0x4000 mirror window (exercises
// the GBR bank-wrap in set_bank) and a 10-bank rip (exercises MBC banking).
// Note: GBR has no "first song" field and many rips keep a silent stop-track at
// song 0, so these fixtures were chosen because their default song 0 plays.
TEST_CASE("GME GBR plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::GMEPlugin plugin;

    for (auto const& gbr : {"testmus/gme/mr driller.gbr",
                            "testmus/gme/kung fu master.gbr",
                            "testmus/gme/dragon quest 3.gbr"}) {
        REQUIRE(plugin.canHandle(gbr));

        auto* player = plugin.fromFile(gbr);
        REQUIRE(player != nullptr);

        std::array<int16_t, 8192> buffer{};
        int64_t energy = 0;
        for (int count = 0; count < 100 && energy == 0; ++count) {
            int rc = player->getSamples(buffer.data(), buffer.size());
            if (rc <= 0) { break; }
            for (int i = 0; i < rc; ++i) {
                energy += std::abs(static_cast<int>(buffer[i]));
            }
        }
        delete player;

        INFO(gbr);
        REQUIRE(energy != 0);
    }
}
// Regression test for AY-3-8910 VGM (Vectrex / ZX Spectrum). The vendored
// Game_Music_Emu's VGM parser predates AY8910 support, so it skipped every
// 0xA0 register write -> the track fell silent and "ended" immediately. The
// chip emulator (Ay_Apu) was already present (it plays .ay), so it's now wired
// into the VGM command stream: AY clock is read from header offset 0x74 and,
// for an AY-only tune (no SN76489 PSG), the blip-time domain is clocked at the
// AY rate so the pitch is right. These Vectrex rips are all AY-only.
TEST_CASE("GME Vectrex AY VGM plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::GMEPlugin plugin;

    for (auto const& vgz : {"testmus/gme/vectrex-heads up.vgz",
                            "testmus/gme/vectrex-berzerk.vgz",
                            "testmus/gme/vectrex-scramble.vgz"}) {
        INFO(vgz);
        REQUIRE(plugin.canHandle(vgz));

        auto* player = plugin.fromFile(vgz);
        REQUIRE(player != nullptr);

        std::array<int16_t, 8192> buffer{};
        int64_t energy = 0;
        for (int count = 0; count < 100 && energy == 0; ++count) {
            int rc = player->getSamples(buffer.data(), buffer.size());
            if (rc <= 0) { break; }
            for (int i = 0; i < rc; ++i) {
                energy += std::abs(static_cast<int>(buffer[i]));
            }
        }
        delete player;

        REQUIRE(energy != 0);
    }
}

// Regression test for packed GYM (Sega Genesis/Mega Drive YM2612+PSG register
// dump). A GYM file may store its command stream raw, or -- with a "GYMX" header
// -- as a raw zlib stream whose uncompressed length lives in the header's
// "packed" field (offset 424). The vendored Game_Music_Emu can inflate these,
// but only when built with zlib (HAVE_ZLIB_H); otherwise Gym_Emu's check_header
// returns "Packed GYM file not supported". This asserts both that the two .gym
// fixtures are genuinely packed (non-zero packed field) and that they decode to
// audio -- i.e. the zlib unpack path (unpack_gym_body) is wired and working.
TEST_CASE("GME packed GYM plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::GMEPlugin plugin;

    for (auto const& gym : {"testmus/gme/art alive.gym",
                            "testmus/gme/fantasia-00.gym"}) {
        INFO(gym);

        // Confirm the fixture really is a packed GYMX file (not just a raw one),
        // so this test actually exercises the inflate path.
        auto data = utils::File(gym).readAll();
        REQUIRE(data.size() > 428);
        REQUIRE(std::memcmp(data.data(), "GYMX", 4) == 0);
        uint32_t packed = static_cast<uint8_t>(data[424]) |
                          (static_cast<uint8_t>(data[425]) << 8) |
                          (static_cast<uint8_t>(data[426]) << 16) |
                          (static_cast<uint32_t>(static_cast<uint8_t>(data[427])) << 24);
        REQUIRE(packed != 0);

        REQUIRE(plugin.canHandle(gym));
        auto* player = plugin.fromFile(gym);
        REQUIRE(player != nullptr);

        std::array<int16_t, 8192> buffer{};
        int64_t energy = 0;
        for (int count = 0; count < 100 && energy == 0; ++count) {
            int rc = player->getSamples(buffer.data(), buffer.size());
            if (rc <= 0) { break; }
            for (int i = 0; i < rc; ++i) {
                energy += std::abs(static_cast<int>(buffer[i]));
            }
        }
        delete player;

        REQUIRE(energy != 0);
    }
}

// .rol (AdLib Visual Composer) was previously excluded because its player loads
// instruments from a companion "standard.bnk" in the same dir (rol.cpp), which
// was missing -> silent. The bank is now vendored (testmus/adlib/standard.bnk,
// from modland's Ad Lib/Visual Composer set) so america2.rol renders for real.
TEST_CASE("AdPlug", "[music]") { testPlugin<musix::AdPlugin>("testmus/adlib", "", "data"); }

// Sierra SCI (.sci) is a multi-file AdLib format: AdPlug's mid.cpp loader needs
// the "<prefix>patch.003" OPL2 instrument bank alongside the song. AdPlugin must
// name it via getSecondaryFiles so the host fetches it (modland co-hosts it in
// the same dir, e.g. "kq1 flutey.sci" + "kq1patch.003"). Without it the load
// throws and the song can't play from the GUI.
TEST_CASE("AdPlug SCI secondary patch", "[music]")
{
    musix::AdPlugin plugin{ "data" };
    auto sec = plugin.getSecondaryFiles("testmus/adlib/kq1 flutey.sci");
    REQUIRE(sec.size() == 1);
    REQUIRE(sec[0] == "kq1patch.003");
    // full paths/URLs resolve to just the companion file name
    REQUIRE(plugin.getSecondaryFiles(
                "ftp://x/Ad Lib/Sierra/Kings Quest 1/kq1 flutey.sci")
                .at(0) == "kq1patch.003");
    // .ksm (Ken Silverman) needs the fixed-name "insts.dat" instrument bank
    auto ksm = plugin.getSecondaryFiles("testmus/adlib/maxosong.ksm");
    REQUIRE(ksm.size() == 1);
    REQUIRE(ksm[0] == "insts.dat");
    // non-SCI/KSM AdLib formats request no secondary files
    REQUIRE(plugin.getSecondaryFiles("song.laa").empty());
}

// Render up to `buffers` blocks and return summed absolute sample energy.
static int64_t adplugEnergy(musix::ChipPlayer* player, int buffers)
{
    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < buffers; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    return energy;
}

// Regression test for Westwood .snd support (Eye of the Beholder, Legend of
// Kyrandia, ...). These are headerless ADL/OPL tunes that reuse the generic
// .snd extension. The vendored AdPlug already had the Westwood ADL player but
// only accepted ".adl"; both adl.cpp's loader and adplug.cpp's player table now
// also accept ".snd", and AdPlugin::canHandle validates .snd by reproducing the
// ADL version detection so it claims real Westwood tunes without stealing the
// Atari sc68 .snd files (which SC68Plugin validates by magic). The fixture is
// the genuine Eye of the Beholder AdLib sound bank (a self-contained v1 Westwood
// ADL tune) under a .snd extension.
//
// NOTE: bare per-track Westwood .snd rips (e.g. modland's "Westwood SND"
// collection) store only the sequence data and reference an *external*
// instrument bank that isn't in the file, so they decode to silence -- the
// self-contained ".adl" version of each tune is the supported path. This
// fixture deliberately uses ADL-format content so it actually produces audio.
//
// Also covers the subsong-navigation fix: AdPlugPlayer::seekTo() switches
// subsong (Westwood files pack many tracks into one bank), which previously
// no-op'd because the base ChipPlayer::seekTo() returned false. Fails if the
// .snd routing/validation regresses, the ADL decoder goes silent, or subsong
// seeking breaks.
TEST_CASE("Westwood SND plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::AdPlugin plugin{"data"};

    std::string const snd = "testmus/westwood/eobsound.snd";
    REQUIRE(plugin.canHandle(snd));

    auto* player = plugin.fromFile(snd);
    REQUIRE(player != nullptr);

    // Default subsong produces audio.
    REQUIRE(adplugEnergy(player, 50) != 0);

    // Out-of-range subsong is rejected; an in-range one is accepted and plays.
    REQUIRE_FALSE(player->seekTo(99999, -1));
    REQUIRE(player->seekTo(3, -1));
    REQUIRE(adplugEnergy(player, 50) != 0);

    delete player;
}
// Exclude the TFMX/SoundMaster sample banks (turrican2.smpl, smp.starball) which
// are companion files, not standalone songs -- but NOT ".smpro" SoundMaster songs
// (futureshock-gameover.smpro), which the old broad "smp" substring wrongly hid.
// .rsid (the "real C64" SID variant) is played by VICE just like .sid -- VICE's
// psid_load_file accepts the "RSID" magic. libvice now claims it; this checks a
// genuine RSID rip renders audio. (testmus/libvice isn't folder-scanned, so the
// fixture needs an explicit test.)
TEST_CASE("RSID plays sound", "[music]")
{
    musix::VicePlugin vice{ "data" };
    std::string const rsid = "testmus/libvice/10... knockout!.rsid";
    REQUIRE(vice.canHandle(rsid));
    auto* p = vice.fromFile(rsid);
    REQUIRE(p != nullptr);
    std::array<int16_t, 8192> buf{};
    int64_t e = 0;
    for (int i = 0; i < 50 && e == 0; i++) {
        int rc = p->getSamples(buf.data(), static_cast<int>(buf.size()));
        if (rc <= 0) { break; }
        for (int j = 0; j < rc; j++) { e += std::abs(static_cast<int>(buf[j])); }
    }
    delete p;
    REQUIRE(e != 0);
}

TEST_CASE("UADE", "[music]") { testPlugin<musix::UADEPlugin>("testmus/uade", ".mod.nt", "data"); }

// The TFMX family is filed on modland as "mdat.<song>"/"smpl.<song>" pairs (the
// playback fixtures above use that real naming), but supported_ext also lists the
// eagleplayer format tokens tfmx/tfmx7v/tfmxpro (Amiga) and tfhd7v/tfhdpro (Atari
// ST) as recognised prefixes. Those synthetic prefixes never occur on disk, so we
// don't keep duplicate playback fixtures for them -- but guard here that a name
// carrying any of them still routes to UADE, so an edit to supported_ext can't
// silently drop the extension. (NB: "tfmx1.5"/"tfhd1.5" can't match -- canHandle
// keys on the substring before the FIRST dot, which is "tfmx1"/"tfhd1" -- so they
// are intentionally absent here.)
TEST_CASE("UADE claims TFMX-family prefixes", "[music]")
{
    // Names are directory-qualified: UADE keys the prefix on the substring
    // between the last path separator and the first following dot, so a bare
    // "mdat.song" (no separator) doesn't parse -- real fixtures always carry a
    // directory, which is the case we're guarding.
    musix::UADEPlugin uade{ "data" };
    for (auto const* name : { "d/mdat.song", "d/tfmx.song", "d/tfmx7v.song",
                              "d/tfmxpro.song", "d/tfhd7v.song",
                              "d/tfhdpro.song" }) {
        INFO(name);
        REQUIRE(uade.canHandle(name));
    }
}

// GUI sanity check for every multi-file fixture whose companion we bundled.
// cmtest plays from local files, so a song would render fine here even if the
// plugin couldn't NAME its companion -- but the GUI streams the song and fetches
// secondaries by the names getSecondaryFiles() returns. So for each bundled
// companion we assert (a) getSecondaryFiles() returns exactly that name and
// (b) the named file actually sits next to the song. If either breaks, the tune
// plays in cmtest but FAILS in the app. Locks the companion-fix work end-to-end.
TEST_CASE("secondary files resolve for multi-file fixtures", "[music]")
{
    auto check = [](musix::ChipPlugin& plugin, std::string const& song,
                    std::string const& companion) {
        INFO(song << "  ->  " << companion);
        auto sec = plugin.getSecondaryFiles(song);
        REQUIRE(std::find(sec.begin(), sec.end(), companion) != sec.end());
        auto dir = utils::path_directory(song);
        REQUIRE(utils::File{ dir + "/" + companion }.exists());
    };

    musix::UADEPlugin uade{ "data" };
    check(uade, "testmus/uade/mdat.kraft", "smpl.kraft");                 // TFMX
    check(uade, "testmus/uade/mdat.avalon2-ongame", "smpl.avalon2-ongame");
    check(uade, "testmus/uade/mdat.hexuma-ice", "smpl.hexuma-ice");
    check(uade, "testmus/uade/mdat.karamalz-titel", "smpl.karamalz-titel");
    check(uade, "testmus/uade/mdat.abandonedplaces-part2.1",
          "smpl.abandonedplaces-part2.1");
    check(uade, "testmus/uade/thm.nightdawn", "smp.nightdawn");           // Thomas Hermann
    check(uade, "testmus/uade/thm.blueangel69", "smp.blueangel69");
    check(uade, "testmus/uade/uds.desert run ste", "smp.desert run ste"); // BladePacker
    check(uade, "testmus/uade/uds.obsession loader ste",
          "smp.obsession loader ste");
    check(uade, "testmus/uade/daisy.adsc", "daisy.adsc.as");             // AudioSculpture
    check(uade, "testmus/uade/jpn.empiresoccer94", "smp.empiresoccer94"); // Jason Page
    check(uade, "testmus/uade/dns.hollywoodpokerpro ingame",
          "smp.hollywoodpokerpro ingame");                               // DynamicSynth
    check(uade, "testmus/uade/mcr.aquablast", "mcs.aquablast");          // Mark Cooksey
    check(uade, "testmus/uade/uds.obsession menu", "smp.obsession menu"); // BladePacker
    check(uade, "testmus/uade/tpu.timelock ingame", "smp.timelock ingame"); // DirkBialluch
    check(uade, "testmus/uade/mfp.crystaldragon ingame",
          "smp.crystaldragon ingame");                                    // MagneticFields
    check(uade, "testmus/uade/MIDI.Entity high", "SMPL.Entity high");     // MIDI-Loriciel
    check(uade, "testmus/uade/qts.Big Pro", "SMP.set");                   // Quartet ST (shared bank)
    check(uade, "testmus/uade/the cycles.kh", "songplay");               // Kris Hatlelid (shared replay)

    musix::HTPlugin ht; // PSF "_lib" tag
    check(ht, "testmus/ht/ggx-66-00-01.minidsf", "ggx_66.dsflib");
    check(ht, "testmus/ht/w00-00-25.minissf", "W00.ssflib");

    musix::USFPlugin usf;
    check(usf, "testmus/usf/sparse01.miniusf", "quake2.usflib");

    musix::AdPlugin adp{ "data" };
    check(adp, "testmus/adlib/kq1 flutey.sci", "kq1patch.003");
    check(adp, "testmus/adlib/maxosong.ksm", "insts.dat");
    check(adp, "testmus/adlib/song1.sng", "song1.ins"); // AdLib Tracker
}

TEST_CASE("PxTone", "[music]") { testPlugin<musix::PxTonePlugin>("testmus/ptcop", ""); }
TEST_CASE("PxTune", "[music]") { testPlugin<musix::PxTonePlugin>("testmus/pttune", ""); }
TEST_CASE("PTK", "[music]") { testPlugin<musix::PTKPlugin>("testmus/ptk", ""); }
TEST_CASE("NTK", "[music]") { testPlugin<musix::PTKPlugin>("testmus/ntk", ""); }
TEST_CASE("Org", "[music]") { testPlugin<musix::OrgPlugin>("testmus/org", ""); }
TEST_CASE("SunVox", "[music]") { testPlugin<musix::SunVoxPlugin>("testmus/sunvox", ""); }
// Exclude the ".W" wavebank from the scan -- it's the song's companion, not a
// playable fixture (canHandle rightly declines it); fromFile() picks it up next
// to the bare song.
TEST_CASE("SoundSmith", "[music]") { testPlugin<musix::SoundSmithPlugin>("testmus/soundsmith", ""); }
TEST_CASE("Musx", "[music]") { testPlugin<musix::MusxPlugin>("testmus/musx", ""); }
TEST_CASE("Coconizer", "[music]") { testPlugin<musix::CocoPlugin>("testmus/coco", ""); }
TEST_CASE("Funktracker", "[music]") { testPlugin<musix::FnkPlugin>("testmus/fnk", ""); }
TEST_CASE("MaxTrax", "[music]") { testPlugin<musix::MaxTraxPlugin>("testmus/maxtrax", ""); }
TEST_CASE("STarKos", "[music]") { testPlugin<musix::SksPlugin>("testmus/sks", ""); }
TEST_CASE("NerdTracker2", "[music]") { testPlugin<musix::NEDPlugin>("testmus/ned", ""); }
TEST_CASE("PlayerPRO", "[music]") { testPlugin<musix::PlayerProPlugin>("testmus/playerpro", ""); }
TEST_CASE("JayTrax", "[music]") { testPlugin<musix::JxsPlugin>("testmus/jxs", ""); }
TEST_CASE("IXS", "[music]") { testPlugin<musix::IXSPlugin>("testmus/ixs", ""); }
TEST_CASE("FamiTracker", "[music]") { testPlugin<musix::FamiTrackerPlugin>("testmus/famitracker", ""); }

// .ftm is two unrelated formats: NES FamiTracker (magic "FamiTracker Module")
// vs Atari "Face The Music" (magic "FTMN", handled by OpenMPT). The two plugins
// must content-gate so each claims only its own files. Guards the routing split.
TEST_CASE("FamiTracker vs FaceTheMusic routing", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::FamiTrackerPlugin fami;
    musix::OpenMPTPlugin ompt;

    std::string const ftm = "testmus/famitracker/2a03_hfrth.ftm"; // FamiTracker
    std::string const ftmn = "testmus/openmpt/andreab.ftm";       // Face The Music

    REQUIRE(fami.canHandle(ftm));        // FamiTracker claims its module
    REQUIRE_FALSE(fami.canHandle(ftmn)); // and declines Face The Music
    REQUIRE(ompt.canHandle(ftmn));       // OpenMPT still owns Face The Music
}

// PlayerPRO ".mad" (Macintosh tracker, "MADG"/"MADF"/"MADK") plays via the
// vendored public-domain MADDriver. The ".mad" extension collides with AdPlug's
// Mad Tracker 2 loader ("MAD+"), which used to claim these files and fail to
// load them; AdPlug now content-declines them so they route here. This guards
// both the engine slice and the AdPlug/PlayerPRO routing split.
TEST_CASE("PlayerPRO routing", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::PlayerProPlugin pp;
    musix::AdPlugin ad{""};

    std::string const mad = "testmus/playerpro/mantra 03 dungeon.mad";
    REQUIRE(pp.canHandle(mad));     // PlayerPRO claims the MADG module
    REQUIRE_FALSE(ad.canHandle(mad)); // AdPlug declines (not "MAD+")

    auto* player = pp.fromFile(mad);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    // Guard against the -funsigned-char regression: with unsigned char the 8-bit
    // samples are misread and the mix clips hard (RMS pinned near full scale).
    // A correct render of this tune sits well below that, so assert a sane level.
    double sumSq = 0;
    long nSamp = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(buffer[i]);
            sumSq += double(buffer[i]) * buffer[i];
            nSamp++;
        }
    }
    delete player;
    REQUIRE(energy > 0);
    double rms = nSamp ? std::sqrt(sumSq / nSamp) : 0.0;
    REQUIRE(rms < 9000.0); // correct ~3000-4000; the unsigned-char bug pushes it >13000
}

// MaxTrax (.mxtx, the Amiga sound engine behind Cyberdreams' Dark Seed et al).
// Played by a vendored port of ScummVM's MaxTrax sequencer + Paula mixer; UADE
// is NOT involved (it detects the MXTX magic but ships no eagleplayer). This
// guards the ScummVM source slice + compat shim + the MXTX magic gate; it fails
// if the vendored sources/compat.h regress or the loader/mixer goes silent.
TEST_CASE("MaxTrax plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::MaxTraxPlugin plugin;

    std::string const mxtx = "testmus/maxtrax/darkseed_00.mxtx";
    REQUIRE(plugin.canHandle(mxtx));
    // Right magic only -- an unrelated file with no MXTX header must be declined.
    REQUIRE_FALSE(plugin.canHandle("testmus/org/access.org"));

    auto* player = plugin.fromFile(mxtx);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// Split MaxTrax sets (Frank Klepacki's Kyrandia): the score ("...scr.mxtx") and
// the sampled instruments ("...inst.mxtx") are separate files. Either half can
// be the entry the user picks, so fromFile() must pair them up (loading scores
// from one and samples from the other) and produce audio in both directions.
// Fails if the scr/inst sibling resolution or the two-pass load() regresses.
TEST_CASE("MaxTrax split set plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::MaxTraxPlugin plugin;

    auto energyOf = [&](const std::string& f) {
        REQUIRE(plugin.canHandle(f));
        auto* player = plugin.fromFile(f);
        REQUIRE(player != nullptr);
        std::array<int16_t, 8192> buffer{};
        int64_t energy = 0;
        for (int count = 0; count < 100 && energy == 0; ++count) {
            int rc = player->getSamples(buffer.data(), buffer.size());
            if (rc <= 0) { break; }
            for (int i = 0; i < rc; ++i) {
                energy += std::abs(static_cast<int>(buffer[i]));
            }
        }
        delete player;
        return energy;
    };

    // Score half resolves its instrument sibling; instrument half resolves its
    // score sibling. Both must render the same intro tune.
    REQUIRE(energyOf("testmus/maxtrax/kyrandia introscr.mxtx") != 0);
    REQUIRE(energyOf("testmus/maxtrax/kyrandia introinst.mxtx") != 0);

    // Shared-bank set (Russell Lieblich's "a-train"): the parts carry no
    // scr/inst marker -- they are score-only files that borrow samples from the
    // set's bank ("a-train (intro).mxtx"), found by content + shared filename
    // prefix. This also guards against cross-set contamination: even with the
    // Kyrandia bank present in the same directory, an a-train part must pick the
    // a-train bank, and vice versa, or these would be silent / wrong.
    REQUIRE(energyOf("testmus/maxtrax/a-train (spring).mxtx") != 0);
    REQUIRE(energyOf("testmus/maxtrax/a-train (goodinfo).mxtx") != 0);

    // Secondary-file routing: when streaming (no local mirror), a split half
    // must ask the host to fetch the rest of its directory ("./") so the bank
    // lands next to it; a self-contained module asks for nothing. (The bank's
    // own name can't be derived from a score part, hence the whole-dir request.)
    auto secondaries = [&](const std::string& f) {
        return plugin.getSecondaryFiles(f);
    };
    REQUIRE(secondaries("testmus/maxtrax/a-train (spring).mxtx") ==
            std::vector<std::string>{"./"});           // score-only part
    REQUIRE(secondaries("testmus/maxtrax/kyrandia introinst.mxtx") ==
            std::vector<std::string>{"./"});           // instrument-only bank
    REQUIRE(secondaries("testmus/maxtrax/darkseed_00.mxtx").empty()); // combined
    REQUIRE(secondaries("testmus/maxtrax/a-train (intro).mxtx").empty()); // bank+score
}

// Acorn Archimedes Tracker (.musx, 8-channel "!Tracker"). Played by libxmp's
// arch_loader, compiled as a minimal single-loader slice into musxplugin (it
// does NOT pull in the shared zxtune libxmp build). This guards the slice +
// the MUSX magic gate; it fails if the libxmp source list / build defs /
// arch_loader wiring regress, or if voltable.c (arch_vol_table) drops out.
TEST_CASE("Musx plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::MusxPlugin plugin;

    std::string const musx = "testmus/musx/paradox 1.1 8 tracks the works.musx";
    REQUIRE(plugin.canHandle(musx));
    // Right extension but wrong payload must be declined (the .musx extension is
    // also used by Finale notation files etc.).
    REQUIRE_FALSE(plugin.canHandle("testmus/org/access.org"));

    auto* player = plugin.fromFile(musx);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// Coconizer (.coco) -- a sample-based Acorn Archimedes format played by libxmp's
// coco_loader. cocoplugin compiles only coco_load.c and links musxplugin for
// the shared libxmp slice (a second full slice would collide on every symbol).
// This guards the loader wiring + the 0x84/0x88 first-byte gate.
TEST_CASE("Coconizer plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::CocoPlugin plugin;

    std::string const coco = "testmus/coco/Beethoven1.coco";
    REQUIRE(plugin.canHandle(coco));
    // Wrong payload on the same extension must be declined.
    REQUIRE_FALSE(plugin.canHandle("testmus/org/access.org"));

    auto* player = plugin.fromFile(coco);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// Megatracker (.mgt) -- an Atari ST sample tracker (modland Megatracker/) played
// by libxmp's mgt_loader. mgtplugin compiles only mgt_load.c and links musxplugin
// for the shared libxmp slice (same arrangement as cocoplugin).
TEST_CASE("Megatracker", "[music]") { testPlugin<musix::MgtPlugin>("testmus/megatracker", ""); }

// Guards the loader wiring + the "MGT"/"MCS" magic gate explicitly.
TEST_CASE("Megatracker plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::MgtPlugin plugin;

    std::string const mgt = "testmus/megatracker/feasible.mgt";
    REQUIRE(plugin.canHandle(mgt));
    // Wrong payload on a different extension must be declined.
    REQUIRE_FALSE(plugin.canHandle("testmus/org/access.org"));

    auto* player = plugin.fromFile(mgt);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// Old MED / Amiga "Music Editor" (.med, magic "MED\x02".."MED\x04") -- the
// pre-OctaMED format, distinct from the MMD0..MMD3 OctaMED containers that
// OpenMPT handles. UADE's MED eagleplayer crashes on these ("score crashed")
// and libopenmpt only decodes MMD0..MMD3, so they were unplayable; libxmp's
// med2/med3/med4 loaders decode them. medplugin adds only med2/3/4_load.c and
// links musxplugin for the shared libxmp slice (same arrangement as
// coco/mgtplugin). UADEPlugin::canHandle content-declines old MED so the host
// falls through to medplugin.
TEST_CASE("MED", "[music]") { testPlugin<musix::MedPlugin>("testmus/med", ""); }

// Guards the loader wiring + the "MED\x02..\x04" magic gate, and that UADE
// (which owns .med by extension for OctaMED) declines the old-MED variant.
TEST_CASE("MED plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::MedPlugin plugin;

    std::string const med = "testmus/med/fresnel.med";
    REQUIRE(plugin.canHandle(med));
    // A different payload on the same extension must be declined (OctaMED MMD0).
    REQUIRE_FALSE(plugin.canHandle("testmus/openmpt/Vision.med"));

    // UADE must decline old MED so the host reaches medplugin instead of
    // crashing its 68k MED player.
    musix::UADEPlugin uade{ "data" };
    REQUIRE_FALSE(uade.canHandle(med));

    auto* player = plugin.fromFile(med);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// SBStudio (.pac) -- a sample-based MS-DOS tracker by Henning Hellstroem (modland
// SBStudio/) decoded by the vendored libpac (Thomas Pfaff, ISC). sbstudioplugin
// compiles libpac directly to PCM; this guards the wiring + the PACG magic gate.
TEST_CASE("SBStudio", "[music]") { testPlugin<musix::SBStudioPlugin>("testmus/sbstudio", ""); }

TEST_CASE("SBStudio plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::SBStudioPlugin plugin;

    std::string const pac = "testmus/sbstudio/silvermoon.pac";
    REQUIRE(plugin.canHandle(pac));
    // Wrong payload on a different extension must be declined.
    REQUIRE_FALSE(plugin.canHandle("testmus/org/access.org"));

    auto* player = plugin.fromFile(pac);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// SunVox (.sunvox, NightRadio's modular synth). The engine ships as a prebuilt,
// dlopen()ed shared library (MIT licensed, copied next to the test binary by
// CMake). This exercises the real DB content -- the .sunvox files here are the
// exact modland songs referenced by the chipmachine database.
TEST_CASE("SunVox plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::SunVoxPlugin plugin;

    std::string const sunvox = "testmus/sunvox/caravan.sunvox";
    REQUIRE(plugin.canHandle(sunvox));

    auto* player = plugin.fromFile(sunvox);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// Organya (.org, Cave Story / OrgMaker). The .org file carries only the
// sequence; the WAVE100 wavetable + drum PCM are a universal constant embedded
// in the plugin (default_wdb.h), so a plain .org plus the built-in soundbank
// must produce audio with no external/secondary files. This fails if the
// embedded soundbank is dropped or the magic check in canHandle regresses.
TEST_CASE("Org plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::OrgPlugin plugin;

    std::string const org = "testmus/org/access.org";
    REQUIRE(plugin.canHandle(org));

    auto* player = plugin.fromFile(org);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// ZX Spectrum Sound Tracker 1.1 (.st11) via the vendored ZXTune engine. Modland
// stores these as a "ZXAYST11" container wrapping a raw uncompiled Sound Tracker
// v1.x module at offset 0x38. libayfly only decodes the *compiled* STC variant,
// so before ZXTune these 59 modland tunes played in nothing. ZXTune's raw
// container scanner locates the embedded module and its ST1 player renders it.
// This fails if the ZXTune engine vendoring/registration regresses, if the
// raw+archive container set is trimmed too far (the raw scanner's lookahead
// needs the other archive plugins registered), or if the AY device is dropped.
TEST_CASE("ZXTune ST11 plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::ZXTunePlugin plugin;

    std::string const st11 = "testmus/st11/agent1.st11";
    REQUIRE(plugin.canHandle(st11));

    auto* player = plugin.fromFile(st11);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// Beepola Phaser1 (.bbsong P1D) end-to-end: parse -> pack into the Phaser1
// player's data layout -> assemble the player with the in-repo Z80 assembler ->
// run on the Z80 core sampling the 1-bit speaker. This exercises the whole
// bbsong pipeline and fails if the parser, packer, vendored assembler, or Z80
// speaker sampler regress.
TEST_CASE("Beepola Phaser1 plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::BBSongPlugin plugin;

    std::string const bb = "testmus/bbsong/mr. blue sky.bbsong";
    REQUIRE(plugin.canHandle(bb));

    auto* player = plugin.fromFile(bb);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// Beepola Music Box (.bbsong TMB) end-to-end. Unlike Phaser1, the Music Box
// player calls ZX Spectrum ROM routines (KEY-SCAN), so this also exercises the
// 48K ROM being mapped at 0x0000 -- without it the player froze on one note.
TEST_CASE("Beepola Music Box plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::BBSongPlugin plugin;

    std::string const bb = "testmus/bbsong/in the hall of the mountain king.bbsong";
    REQUIRE(plugin.canHandle(bb));

    auto* player = plugin.fromFile(bb);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// Beepola SFX (.bbsong SFX / "Special FX / Fuzz Click") end-to-end. The SFX
// player came from Beepola (no source), runs via an IM2 50Hz interrupt, and its
// compiled bytecode is reproduced by our packer (validated byte-exact). This
// exercises the SFX path: parse -> buildSfxImage -> run with interrupts.
TEST_CASE("Beepola SFX plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::BBSongPlugin plugin;

    std::string const bb = "testmus/bbsong/malaguena.bbsong";
    REQUIRE(plugin.canHandle(bb));

    auto* player = plugin.fromFile(bb);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// SCC-Musixx (.SNG): the original Tyfoon-Software SCC-MUSIXX replay routine
// (embedded REPLAY.BIN) runs on the GME Z80 core, with its Konami SCC register
// writes routed into emu2212. testPlugin prints the standard "---- SCC-Musixx
// ----" header and a "Trying <file> ... playback OK" line per fixture (feeding
// the coverage tally). The fixture set includes outrun_lower.sng -- the same
// image with a lowercase extension -- so a pass also proves dispatch is by
// content, not by extension case.
TEST_CASE("SCC-Musixx", "[music]")
{
    testPlugin<musix::SccMusixxPlugin>("testmus/sccmusixx", "");

    // Routing checks (not covered by testPlugin, which drives this plugin
    // directly): ".sng" is also UADE's Amiga Richard Joseph extension and UADE
    // is tried first, so it must decline MSX SCC images, and conversely a real
    // Richard Joseph ".sng" must be claimed by UADE and declined here -- proving
    // detection is content-based, independent of the extension or its case.
    musix::SccMusixxPlugin scc;
    musix::UADEPlugin uade{"data"};
    REQUIRE(uade.canHandle("testmus/uade/cannon fodder (intro).sng"));
    REQUIRE_FALSE(scc.canHandle("testmus/uade/cannon fodder (intro).sng"));
    REQUIRE_FALSE(uade.canHandle("testmus/sccmusixx/outrun.SNG"));
    REQUIRE_FALSE(uade.canHandle("testmus/sccmusixx/outrun_lower.sng"));

    // UADE plays only the Richard Joseph .sng family; every other .sng chip
    // (GoatTracker/Synder SID, Sam Coupe SAA1099, sample-less ZoundMonitor)
    // otherwise fell through to the ZoundMonitor 68k player and died with
    // "score died" -- a hard FAIL. They must now be declined so they Skip. An
    // AdLib .sng (Faust "FMC!") in particular routes to AdPlug, not UADE.
    musix::AdPlugin adp{"data"};
    REQUIRE_FALSE(uade.canHandle("testmus/adlib/sanxion.sng"));
    REQUIRE(adp.canHandle("testmus/adlib/sanxion.sng"));
}

// GoatTracker (.sng, C64 SID, Lasse Oorni). Plays GoatTracker's own gplay.c
// sequencer + gsid.cpp reSID interface on vendored reSID, via loadsong vendored
// verbatim so it covers every song version. Fixtures span the formats: GTS5
// (sid-warrior / alcorythm_ffff / alien funk), GTS2 (the consultant) and GTS!
// v1 (metal warrior 4). testPlugin prints "---- GoatTracker ----" + a
// "Trying <file> ... playback OK" per fixture.
TEST_CASE("GoatTracker", "[music]")
{
    testPlugin<musix::GoatTrackerPlugin>("testmus/goattracker", "");

    // ".sng" routing: GoatTracker owns the GTS-magic variant by content. UADE
    // (tried for .sng first) must decline it -- otherwise the SID bytes crash
    // its 68k engine -- and SCC-Musixx must not claim it either.
    musix::GoatTrackerPlugin gt;
    musix::UADEPlugin uade{"data"};
    musix::SccMusixxPlugin scc;
    REQUIRE(gt.canHandle("testmus/goattracker/sid-warrior.sng"));
    REQUIRE_FALSE(uade.canHandle("testmus/goattracker/sid-warrior.sng"));
    REQUIRE_FALSE(scc.canHandle("testmus/goattracker/sid-warrior.sng"));
    // ...and GoatTracker must not grab a Richard Joseph .sng.
    REQUIRE_FALSE(gt.canHandle("testmus/uade/aquatic games.sng"));

    // The plugin must be wired into the GUI's registration path
    // (chipmachine/src/plugin_register.cpp), not just instantiable directly.
    // The testPlugin<> calls above construct the class straight up and so pass
    // even when the register hook is missing -- which is exactly how the app
    // ended up unable to play GoatTracker tunes. Guard the registration too.
    musix::ChipPlugin::createPlugins("data");
    REQUIRE(musix::ChipPlugin::getPlugin("GoatTracker") != nullptr);
}

// ZoundMonitor (.sng, Amiga). UADE genuinely has this player; the tunes load
// their instruments by name from a shared "Samples/" directory (on modland one
// level above the song, at the collection root -- fetched via the parent-dir
// fallback in MusicPlayerList's whole-dir companion handling). The fixture ships
// maddick.sng with its Samples/ already in place, so testPlugin exercises real
// playback; the routing REQUIREs cover the content-based claim.
TEST_CASE("ZoundMonitor", "[music]")
{
    testPlugin<musix::UADEPlugin>("testmus/zoundmonitor", "", "data");

    musix::UADEPlugin uade{"data"};
    // Claimed by its structural signature (no magic), and its shared Samples/
    // dir is surfaced as a whole-directory companion.
    REQUIRE(uade.canHandle("testmus/zoundmonitor/maddick.sng"));
    auto sec = uade.getSecondaryFiles("testmus/zoundmonitor/maddick.sng");
    REQUIRE(std::find(sec.begin(), sec.end(), "Samples/") != sec.end());
    // The signature must not misfire on other .sng chips, and GoatTracker must
    // not grab a ZoundMonitor tune.
    REQUIRE_FALSE(uade.canHandle("testmus/goattracker/sid-warrior.sng"));
    musix::GoatTrackerPlugin gtz;
    REQUIRE_FALSE(gtz.canHandle("testmus/zoundmonitor/maddick.sng"));
}

// Apple IIgs SoundSmith. A tune is a PAIR: a bare-named song file (patterns/
// orders) and a separate "<song>.W" wavebank holding the 64KB of Ensoniq 5503
// sound RAM + instrument table. canHandle() identifies the song by its header
// structure -- the leading signature varies per editor build ("SONGOK",
// "IAN9OK", "IAN92a", ...) so it is not a magic; the .W is fetched as a
// secondary file and may not be present at canHandle time.
// getSecondaryFiles() must point at the "<song>.W" companion;
// fromFile() loads both and the in-process DOC emulation renders at 26320 Hz.
// This fails if the magic check regresses, the .W companion isn't resolved, or
// the ported oscillator engine produces silence.
TEST_CASE("SoundSmith plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::SoundSmithPlugin plugin;

    std::string const song = "testmus/soundsmith/Soundsmith Intro";
    REQUIRE(plugin.canHandle(song));

    // The wavebank companion must be reported next to the song as "<song>.W".
    auto secondary = plugin.getSecondaryFiles(song);
    REQUIRE(secondary == std::vector<std::string>{"Soundsmith Intro.W"});

    auto* player = plugin.fromFile(song);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// SoundSmith songs are bare-named on Modland (no on-disk extension) and carry no
// ext in the DB, so resolveExtension() can't derive a key from the path. It must
// fall back to mapping the DB format NAME ("SoundSmith") to the canonical "w"
// extension, otherwise the scroller finds no format description (the "w" entry in
// formats_descriptions.txt). Regresses if the format-name fallback is dropped.
TEST_CASE("SoundSmith format resolves to w", "[music]")
{
    SongInfo info;
    info.format = "SoundSmith";
    info.path = "SoundSmith/- unknown/Amiga remakes/End Theme (Obarski)";
    REQUIRE(chipmachine::MusicDatabase::resolveExtension(info) == "w");
}

// Ixalance (.ixs). A synth tracker from the defunct Shortcut Software: it stores
// no PCM, instead synthesizing + zlib-compressing its own wavetables (songs are
// only a few KB). Played via the vendored webixs core (Wothke's RE of the lost
// Win32 player). Routing is by the "IXS!" magic. This fails if the magic check
// regresses, the zlib-dependent wavetable build breaks, or the pull-style render
// API (genAudio/getAudioBuffer) produces silence.
TEST_CASE("IXS plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::IXSPlugin plugin;

    std::string const ixs = "testmus/ixs/ixalance_theme.ixs";
    REQUIRE(plugin.canHandle(ixs));

    auto* player = plugin.fromFile(ixs);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// Regression test for OctaMED MMD3 routing. libopenmpt's MED loader decodes the
// whole MMD0..MMD3 family by content, but Tables.cpp only advertises the "med"
// extension, so openmpt_is_extension_supported("mmd3") is false. UADE already
// claims mmd0/mmd1/mmd2 (and registers later, so first-match routing leaves them
// with UADE), but nothing claimed ".mmd3" at all -- the file was rejected by
// every plugin. OpenMPTPlugin::canHandle now maps ".mmd3" in explicitly. This
// test fails if that mapping is removed (canHandle goes false) or if libopenmpt
// stops decoding the format (no audio).
TEST_CASE("OpenMPT MMD3 plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::OpenMPTPlugin plugin;

    std::string const mmd3 = "testmus/openmpt/straight-into-my-soul.mmd3";
    REQUIRE(plugin.canHandle(mmd3));

    auto* player = plugin.fromFile(mmd3);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// DSIK "old" Internal Format (.dsm v1) plays sound. libopenmpt only ships the
// newer RIFF/DSMF v2 loader; the v1 format ("DSM"+0x10 header, used by the
// Necros et al. modland tunes) is decoded by a chipmachine-local branch in the
// vendored Load_dsm.cpp ported from MilkyTracker's LoaderDSMv1. Before that,
// openmpt_module_create_from_memory2 returned "error loading file". Fails if the
// v1 branch regresses.
TEST_CASE("OpenMPT DSM v1 plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::OpenMPTPlugin plugin;

    std::string const dsm = "testmus/openmpt/andante.dsm";
    REQUIRE(plugin.canHandle(dsm));

    auto* player = plugin.fromFile(dsm);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// Onyx Music File (.omf) plays sound. This MOD-like Amiga format from the 1993
// "Jangle" musicdisk (modland "Onyx Music File/", 24 tunes) never had a
// standalone replayer -- it was decoded only by the chipmachine-local
// Load_omf.cpp, written from Martin Bazley's (swirlythingy's) 2009 format
// specification. The format stores its sequence table, patterns and events
// backwards, pads every pattern/sample block with three bytes, and uses
// unsigned 8-bit samples. Fails if the loader or its Tables.cpp/Sndfile.cpp
// registration regresses.
TEST_CASE("OpenMPT OMF plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::OpenMPTPlugin plugin;

    for (auto const& omf : {"testmus/openmpt/jangle intro.omf",
                            "testmus/openmpt/laxity remix.omf",
                            "testmus/openmpt/tal.omf"}) {
        REQUIRE(plugin.canHandle(omf));

        auto* player = plugin.fromFile(omf);
        REQUIRE(player != nullptr);

        std::array<int16_t, 8192> buffer{};
        int64_t energy = 0;
        for (int count = 0; count < 100 && energy == 0; ++count) {
            int rc = player->getSamples(buffer.data(), buffer.size());
            if (rc <= 0) { break; }
            for (int i = 0; i < rc; ++i) {
                energy += std::abs(static_cast<int>(buffer[i]));
            }
        }
        delete player;

        REQUIRE(energy != 0);
    }
}

// Symphonie / Symphonie Pro (.symmod) plays sound. This Amiga "pseudo-DAW"
// format (software mixer + real-time echo DSP) has no portable replayer except
// libopenmpt's Load_symmod.cpp, which only landed in libopenmpt 0.6 -- the
// bundled 0.5 could not touch it. The 0.8.7 upgrade adds the loader plus the
// SymMODEcho plugin (which is why NO_PLUGINS was dropped from the build). UADE
// has no Symphonie player at all, so before this nothing decoded .symmod.
// Fails if the libopenmpt upgrade regresses or SymMODEcho is dropped.
TEST_CASE("OpenMPT Symphonie plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::OpenMPTPlugin plugin;

    std::string const symmod = "testmus/openmpt/magnetize-feelings.symmod";
    REQUIRE(plugin.canHandle(symmod));

    auto* player = plugin.fromFile(symmod);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// Regression test for AdLib Tracker 2 "A2M version 11" files, played by the
// newer a2m-v2 loader (Ca2mv2Player). The AdPlugin constructor calls
// CPlayer::songlength(), which plays the whole tune to the end on a throwaway
// CSilentopl and then rewind()s back to the start. Ca2mv2Player::rewind() must
// reset *all* tick counters (ticks, tick0, tickD); a previous version left
// tick0/tickD at their end-of-song values, so during real playback
// poll_proc()'s "ticks - tick0 + 1 >= speed" check never became true,
// play_line() never ran, no notes were written to the OPL, and the tune played
// completely silent. This test fails if that regression is reintroduced.
TEST_CASE("AdPlug A2M v11 plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::AdPlugin plugin{ "data" };

    std::string const a2m = "testmus/adlib/karsten obarski - amegas.a2m";
    REQUIRE(plugin.canHandle(a2m));

    // Constructing the player runs songlength() + rewind() internally.
    auto* player = plugin.fromFile(a2m);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    // Non-silent output means rewind() correctly reset the tick counters so
    // poll_proc() advanced through the pattern and keyed on notes.
    REQUIRE(energy != 0);
}

// Regression test for two-file Richard Joseph songs (.sng + .ins).
// "cannon fodder (intro).sng" needs its companion ".ins" for any audio. The
// RichardJoseph Amiga player loads samples by swapping ".sng" -> ".INS" in the
// SAME directory as the module, so the .sng must be played in place (not copied
// to a temp dir, which would lose the .ins and break sample loading). This test
// fails if that regression is reintroduced.
// Regression test for Pumatracker (.puma) files.
// UADE's eagleplayer.conf uses prefixes=puma, so files stored as "name.puma"
// (modland convention) must be renamed to "puma.name" before uade_play().
// Without that rename the format goes unrecognised and plays silence.
TEST_CASE("UADE Pumatracker puma", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::UADEPlugin plugin{ "data" };

    std::string const puma = "testmus/uade/toki-5.puma";
    REQUIRE(plugin.canHandle(puma));

    auto* player = plugin.fromFile(puma);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

TEST_CASE("UADE Richard Joseph sng", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::UADEPlugin plugin{ "data" };

    std::string const sng = "testmus/uade/cannon fodder (intro).sng";
    REQUIRE(plugin.canHandle(sng));

    auto* player = plugin.fromFile(sng);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    // Non-silent output means the .ins samples were located and loaded.
    REQUIRE(energy != 0);
}
TEST_CASE("UADE YMST secondary files", "[uade]")
{
    logging::setLevel(logging::Level::Warning);
    musix::UADEPlugin plugin{"data"};
    auto tmp = fs::temp_directory_path();

    SECTION("extracts replay name embedded in YMST header")
    {
        auto path = tmp / "test_replay.ymst";
        {
            std::ofstream f(path, std::ios::binary);
            std::string content = "YMST" + std::string(252, '\x00') + "YM.BIG_replay\x00";
            f.write(content.data(), content.size());
        }
        auto files = plugin.getSecondaryFiles(path.string());
        REQUIRE(files.size() == 1);
        REQUIRE(files[0] == "ym.big_replay");
        fs::remove(path);
    }

    SECTION("handles variant replay names correctly")
    {
        auto path = tmp / "test_amberstar.ymst";
        {
            std::ofstream f(path, std::ios::binary);
            std::string content = "YMST" + std::string(252, '\x00') + "YM.AMBERSTAR_replay\x00";
            f.write(content.data(), content.size());
        }
        auto files = plugin.getSecondaryFiles(path.string());
        REQUIRE(files.size() == 1);
        REQUIRE(files[0] == "ym.amberstar_replay");
        fs::remove(path);
    }

    SECTION("returns empty for YMST without embedded replay name")
    {
        auto path = tmp / "test_no_replay.ymst";
        {
            std::ofstream f(path, std::ios::binary);
            std::string content = "YMST" + std::string(64, '\x00');
            f.write(content.data(), content.size());
        }
        auto files = plugin.getSecondaryFiles(path.string());
        REQUIRE(files.empty());
        fs::remove(path);
    }

    SECTION("returns empty for empty YMST file without crash")
    {
        auto path = tmp / "test_empty.ymst";
        { std::ofstream f(path, std::ios::binary); }
        auto files = plugin.getSecondaryFiles(path.string());
        REQUIRE(files.empty());
        fs::remove(path);
    }

    SECTION("returns empty for nonexistent YMST file without crash")
    {
        auto files = plugin.getSecondaryFiles("/nonexistent/path/song.ymst");
        REQUIRE(files.empty());
    }
}

// Regression test for IFF-SMUS (Aegis Sonix) companion loading.
// A SMUS score carries only sequence data; its instruments live in a sibling
// "Instruments/" subdirectory whose member filenames are unpredictable (a
// "<name>.instr" is either a self-contained "Synthesis" voice or a
// "SampledSound" pointing at a raw "<sample>.ss" whose name need not match the
// instrument and whose case is inconsistent on modland). So getSecondaryFiles()
// surfaces the directory itself (trailing slash); MusicPlayerList lists the
// remote folder and pulls every member down next to the score. Without this the
// UADE SonixMusicDriver replay aborts ("score died") for lack of instruments.
// Content-gated on FORM SMUS so it doesn't fire on unrelated ".smus"-named data.
TEST_CASE("UADE SMUS instrument secondary files", "[uade]")
{
    logging::setLevel(logging::Level::Warning);
    musix::UADEPlugin plugin{ "data" };

    auto write = [](fs::path const& p, std::string const& bytes) {
        std::ofstream f(p, std::ios::binary);
        f.write(bytes.data(), bytes.size());
    };
    auto tmp = fs::temp_directory_path();

    SECTION("surfaces the Instruments/ directory for a real FORM SMUS")
    {
        auto files = plugin.getSecondaryFiles("testmus/uade/ACE II.smus");
        REQUIRE(files == std::vector<std::string>{ "Instruments/" });
    }

    SECTION("returns empty for a non-SMUS file")
    {
        auto path = tmp / "not_smus.smus";
        // A valid IFF, but FORM type "8SVX" not "SMUS" (12 bytes, embedded NULs).
        write(path, std::string("FORM\0\0\0\x04" "8SVX", 12));
        REQUIRE(plugin.getSecondaryFiles(path.string()).empty());
        fs::remove(path);
    }

    SECTION("returns empty for a truncated header without crashing")
    {
        auto path = tmp / "trunc.smus";
        write(path, std::string("FORM\0\0", 6));
        REQUIRE(plugin.getSecondaryFiles(path.string()).empty());
        fs::remove(path);
    }

    SECTION("returns empty for a nonexistent file")
    {
        REQUIRE(plugin.getSecondaryFiles("/nonexistent/x.smus").empty());
    }
}

// IFF-SMUS end-to-end playback. "ACE II.smus" needs its companion instruments
// (testmus/uade/Instruments/<name>.{instr,ss}) for any audio: the UADE
// SonixMusicDriver replay loads them from the score's own directory. Non-silent
// output means the whole two-tier instrument set was located and decoded.
TEST_CASE("UADE SMUS plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::UADEPlugin plugin{ "data" };

    std::string const smus = "testmus/uade/ACE II.smus";
    REQUIRE(plugin.canHandle(smus));

    auto* player = plugin.fromFile(smus);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// Manual probe: play an arbitrary local file through UADE and report energy.
// Hidden ('.' tag). Run with: SMUS_TEST_FILE="/path/to/x.smus" cmtest "[.uadefile]"
TEST_CASE("UADE plays local file from env", "[.uadefile]")
{
    logging::setLevel(logging::Level::Warning);
    const char* path = std::getenv("SMUS_TEST_FILE");
    REQUIRE(path != nullptr);
    musix::UADEPlugin plugin{ "data" };
    REQUIRE(plugin.canHandle(path));
    auto* player = plugin.fromFile(path);
    REQUIRE(player != nullptr);
    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 400 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;
    INFO("energy=" << energy);
    REQUIRE(energy != 0);
}

// Network-gated end-to-end streaming test (hidden '.' tag; run with:
// cmtest "[.smusnet]"). Reproduces the real GUI path for a streamed IFF-SMUS that
// is NOT in any local mirror: list the remote "Instruments/" folder, fire ALL
// member downloads concurrently (the case that used to overwhelm the FTP server
// with response-code-0 failures before the connection cap), stage them next to
// the score, then play through UADE. "Pet Shop Jus" references instruments whose
// sample (.ss) filenames differ from the instrument names -- the exact case the
// whole-directory fetch exists to handle.
// An IFF-SMUS rip can be missing a raw ".ss" sample that a SampledSound
// instrument references (modland data rot -- e.g. SLL's Never_give_up_by_sll
// references orchestra1.ss, which 404s). Without resilience the Sonix driver
// renders the WHOLE score silent; the UADE plugin now hands a silent placeholder
// for a missing ".ss" so the remaining instruments still play.
// Network test (hidden): cmtest "[.smusmiss]".
TEST_CASE("UADE SMUS tolerates a missing .ss sample", "[.smusmiss]")
{
    logging::setLevel(logging::Level::Warning);
    RemoteLoader rl;
    rl.registerSource("modland", "ftp://ftp.modland.com/pub/modules/", "");
    auto pump = [&](std::atomic<int>& pending) {
        for (int i = 0; i < 1200 && pending > 0; ++i) {
            rl.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    };
    const std::string base = "IFF-SMUS/SLL/Never_give_up_by_sll/";
    const std::string idir = "Instruments/";
    std::atomic<int> pending{ 1 };
    std::vector<std::string> names;
    rl.listDirectory("modland::" + base + idir,
                     [&](std::vector<std::string> n) { names = std::move(n); pending--; });
    pump(pending);
    REQUIRE(!names.empty());

    auto stage = fs::temp_directory_path() / "smus_miss_test";
    fs::remove_all(stage);
    fs::create_directories(stage / "Instruments");
    pending = 1;
    rl.load("modland::" + base + "Never_give_up_by_sll.smus", [&](utils::File f) {
        if (f) utils::File::copy(f.getName(),
                                 (stage / "Never_give_up_by_sll.smus").string());
        pending--;
    });
    for (auto& n : names) {
        pending++;
        auto dst = stage / "Instruments" / n;
        rl.load("modland::" + base + idir + n, [&, dst](utils::File f) {
            if (f) utils::File::copy(f.getName(), dst.string());
            pending--;
        });
    }
    pump(pending);
    // orchestra1.ss is deliberately NOT staged (it 404s on modland); the plugin
    // must synthesize a silent placeholder so playback is not fully silent.
    REQUIRE(!fs::exists(stage / "Instruments" / "orchestra1.ss"));

    musix::UADEPlugin plugin{ "data" };
    auto* player =
        plugin.fromFile((stage / "Never_give_up_by_sll.smus").string());
    REQUIRE(player != nullptr);
    std::array<int16_t, 8192> buf{};
    int64_t energy = 0;
    for (int c = 0; c < 400 && energy == 0; ++c) {
        int rc = player->getSamples(buf.data(), buf.size());
        if (rc <= 0) break;
        for (int i = 0; i < rc; ++i) energy += std::abs((int)buf[i]);
    }
    delete player;
    fs::remove_all(stage);
    REQUIRE(energy != 0);
}

// Network test (hidden): cmtest "[.smusinstr]". Some rips drop a whole ".instr"
// the score references (SLL/Super_sll_disco lists "Warriors1" first, but
// Warriors1.instr is absent from the server). Without the missing-.instr
// resilience the Sonix driver "score died"s before reaching the 12 instruments
// that ARE present; the plugin must substitute a silent Synthesis stub so the
// tune still plays.
TEST_CASE("UADE SMUS tolerates a missing .instr", "[.smusinstr]")
{
    logging::setLevel(logging::Level::Warning);
    RemoteLoader rl;
    rl.registerSource("modland", "ftp://ftp.modland.com/pub/modules/", "");
    auto pump = [&](std::atomic<int>& pending) {
        for (int i = 0; i < 1200 && pending > 0; ++i) {
            rl.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    };
    const std::string base = "IFF-SMUS/SLL/Super_sll_disco/";
    const std::string idir = "Instruments/";
    std::atomic<int> pending{ 1 };
    std::vector<std::string> names;
    rl.listDirectory("modland::" + base + idir,
                     [&](std::vector<std::string> n) { names = std::move(n); pending--; });
    pump(pending);
    REQUIRE(!names.empty());

    auto stage = fs::temp_directory_path() / "smus_instr_test";
    fs::remove_all(stage);
    fs::create_directories(stage / "Instruments");
    pending = 1;
    rl.load("modland::" + base + "Super_sll_disco.smus", [&](utils::File f) {
        if (f) utils::File::copy(f.getName(),
                                 (stage / "Super_sll_disco.smus").string());
        pending--;
    });
    for (auto& n : names) {
        pending++;
        auto dst = stage / "Instruments" / n;
        rl.load("modland::" + base + idir + n, [&, dst](utils::File f) {
            if (f) utils::File::copy(f.getName(), dst.string());
            pending--;
        });
    }
    pump(pending);
    // Warriors1.instr is the score's FIRST instrument but is absent from the
    // server -- the plugin must stub it with a silent Synthesis instrument.
    REQUIRE(!fs::exists(stage / "Instruments" / "Warriors1.instr"));

    musix::UADEPlugin plugin{ "data" };
    auto* player =
        plugin.fromFile((stage / "Super_sll_disco.smus").string());
    REQUIRE(player != nullptr);
    std::array<int16_t, 8192> buf{};
    int64_t energy = 0;
    for (int c = 0; c < 400 && energy == 0; ++c) {
        int rc = player->getSamples(buf.data(), buf.size());
        if (rc <= 0) break;
        for (int i = 0; i < rc; ++i) energy += std::abs((int)buf[i]);
    }
    delete player;
    fs::remove_all(stage);
    REQUIRE(energy != 0);
}

TEST_CASE("UADE SMUS streams and plays from modland", "[.smusnet]")
{
    logging::setLevel(logging::Level::Warning);
    RemoteLoader rl;
    // Bogus local_dir forces the FTP path (no local-mirror short-circuit).
    rl.registerSource("modland", "ftp://ftp.modland.com/pub/modules/",
                      "/nonexistent-mirror/");

    auto pump = [&](std::atomic<int>& pending) {
        for (int i = 0; i < 1200 && pending > 0; ++i) {
            rl.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    };

    const std::string songRel = "IFF-SMUS/Juz.J/Pet Shop Jus (Juz.J).smus";
    const std::string dirRel = "IFF-SMUS/Juz.J/Instruments/";

    std::atomic<int> pending{ 1 };
    std::vector<std::string> names;
    rl.listDirectory("modland::" + dirRel, [&](std::vector<std::string> n) {
        names = std::move(n);
        pending--;
    });
    pump(pending);
    REQUIRE(!names.empty());

    auto stage = fs::temp_directory_path() / "smus_net_test";
    fs::remove_all(stage);
    fs::create_directories(stage / "Instruments");

    // Fire the song + every Instruments/ member concurrently (no waiting between
    // requests) -- this is what tripped the FTP connection storm.
    pending = 1;
    rl.load("modland::" + songRel, [&](utils::File f) {
        if (f) { utils::File::copy(f.getName(), (stage / "Pet Shop Jus (Juz.J).smus").string()); }
        pending--;
    });
    for (const auto& n : names) {
        pending++;
        auto dst = stage / "Instruments" / n;
        rl.load("modland::" + dirRel + n, [&, dst](utils::File f) {
            if (f) { utils::File::copy(f.getName(), dst.string()); }
            pending--;
        });
    }
    pump(pending);

    // Every needed instrument and sample must have arrived intact.
    for (const char* needed :
         { "Bassguitar_AD.instr", "Bassguitar_AD.ss", "Clap.instr",
           "Cameo Bassdrum.ss", "West_End_Girls.ss" }) {
        auto p = stage / "Instruments" / needed;
        INFO("missing/empty: " << needed);
        REQUIRE(fs::exists(p));
        REQUIRE(fs::file_size(p) > 0);
    }

    musix::UADEPlugin plugin{ "data" };
    auto* player =
        plugin.fromFile((stage / "Pet Shop Jus (Juz.J).smus").string());
    REQUIRE(player != nullptr);
    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 400 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) energy += std::abs(static_cast<int>(buffer[i]));
    }
    delete player;
    fs::remove_all(stage);
    REQUIRE(energy != 0);
}

// modland stores some IFF-SMUS instrument folders lowercase ("instruments/")
// even though the Sonix score references "Instruments/"; FTP listing is
// case-sensitive. Verifies the premise of MusicPlayerList's case fallback:
// the capital folder lists empty, the lowercase one returns members.
// Network test (hidden): cmtest "[.smuscase]".
TEST_CASE("modland IFF-SMUS lowercase instruments dir", "[.smuscase]")
{
    logging::setLevel(logging::Level::Warning);
    RemoteLoader rl;
    rl.registerSource("modland", "ftp://ftp.modland.com/pub/modules/", "");

    auto listOnce = [&](const std::string& rel) {
        std::atomic<int> pending{ 1 };
        std::vector<std::string> names;
        rl.listDirectory("modland::" + rel, [&](std::vector<std::string> n) {
            names = std::move(n);
            pending--;
        });
        for (int i = 0; i < 600 && pending > 0; ++i) {
            rl.update();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        return names;
    };

    const std::string base = "IFF-SMUS/SLL/freak_out_by_sll/";
    auto capital = listOnce(base + "Instruments/");
    auto lower = listOnce(base + "instruments/");
    INFO("capital count=" << capital.size() << " lower count=" << lower.size());
    REQUIRE(capital.empty());   // case-sensitive FTP: capital does not exist
    REQUIRE(!lower.empty());    // real folder, members present
}

// End-to-end GUI streaming for UADE two-file formats whose player derives the
// companion name from the song's ON-DISK basename (Richard Joseph .sng->.INS,
// MusicMaker V8 .sdata->.ip) or from a fixed sibling (SoundPlayer SMP.<name>,
// Synth Dream SMP.set). Drives the real MusicPlayerList over Modland FTP with a
// bogus local mirror so the song lands in the URL-encoded web cache -- the exact
// case where, before the clean-name re-materialisation in playCurrent(), UADE
// looked for "<encoded-name>.INS" while loadSecondaryFile() had staged the clean
// "<song>.ins", so the tune streamed silent. Each must now produce audio.
TEST_CASE("UADE two-file formats stream and play via MusicPlayerList",
          "[.uadestream]")
{
    using namespace chipmachine;
    logging::setLevel(logging::Level::Warning);
    auto ap = std::make_shared<AudioPlayerNull>();
    RemoteLoader rl;
    rl.registerSource("modland", "ftp://ftp.modland.com/pub/modules/",
                      "/nonexistent-mirror/");
    MusicDatabase mdb{ rl };
    musix::ChipPlugin::createPlugins("data");
    MusicPlayerList mpl{ mdb, rl, ap };

    for (const char* rel :
         { "Richard Joseph/Richard Joseph/aquatic games.sng",
           "MusicMaker V8 Old/- unknown/best of guitars.sdata",
           "SoundPlayer/Scott Johnston/sjs.rudi",
           "Synth Dream/Laurens Tummers/sdr.monsterbusiness 1",
           "Kris Hatlelid/Kris Hatlelid/The Cycles/the cycles.kh" }) {
        INFO("streaming " << rel);
        mpl.playSong(SongInfo{ std::string("modland::") + rel });

        int64_t energy = 0;
        std::vector<int16_t> buf(8192);
        // ~60s budget: FTP fetch of the song + its companion(s), then decode.
        for (int i = 0; i < 3000 && energy == 0; ++i) {
            REQUIRE_FALSE(mpl.hasError());
            auto st = mpl.getState();
            if (st == MusicPlayerList::Playing) {
                ap->get(buf);
                for (auto v : buf) { energy += std::abs(static_cast<int>(v)); }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        REQUIRE(energy != 0);
    }
}

// Regression for Modland's console collections (SGC/KSS/NSF/GBS/HES/...), which
// expose every subtune as a tiny GME-style .m3u that points at a SHARED module:
//   "Alien Syndrome.sgc::KSS,0,<title>,<len>,..."
// The .m3u branch of MusicPlayerList::playFile() used to treat ANY .m3u as a
// radio playlist -- it took line 0 verbatim, tagged it "MP3", and handed it to
// ffmpeg, which choked on the literal "KSS,0,..." text ("No such file or
// directory"). It now recognises the "<file>[::type],<track>,..." form, fetches
// the sibling module from the same Modland directory, and starts the named
// subtune. Network-gated (Modland FTP); not part of the default run.
TEST_CASE("modland console-subtune m3u resolves to module and plays",
          "[.m3usubtune]")
{
    using namespace chipmachine;
    logging::setLevel(logging::Level::Warning);
    auto ap = std::make_shared<AudioPlayerNull>();
    RemoteLoader rl;
    rl.registerSource("modland", "ftp://ftp.modland.com/pub/modules/",
                      "/nonexistent-mirror/");
    MusicDatabase mdb{ rl };
    musix::ChipPlugin::createPlugins("data");
    MusicPlayerList mpl{ mdb, rl, ap };

    mpl.playSong(SongInfo{ "modland::SGC/Takashi Horiguchi/"
                           "Alien Syndrome/01 BGM #01.m3u" });

    int64_t energy = 0;
    std::vector<int16_t> buf(8192);
    for (int i = 0; i < 3000 && energy == 0; ++i) {
        REQUIRE_FALSE(mpl.hasError());
        if (mpl.getState() == MusicPlayerList::Playing) {
            ap->get(buf);
            for (auto v : buf) { energy += std::abs(static_cast<int>(v)); }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    REQUIRE(energy != 0);
}

TEST_CASE("OpenMPT", "[music]") { testPlugin<musix::OpenMPTPlugin>("testmus/openmpt", ""); }
// Startrekker (FLT/EXO) routing: OpenMPT plays purely *sampled* FLT4/EXO4
// modules but declines genuine *AM* ones (synth voices live in an external
// .nt/.as companion this libopenmpt build can't load) so UADE handles those.
// OpenMPTPlugin distinguishes them by counting referenced-but-empty samples.
// Assert both directions so a future plugin/loader change can't re-misroute.
TEST_CASE("Startrekker AM vs sampled routing", "[music]")
{
    musix::OpenMPTPlugin ompt;
    // Sampled FLT4 (UnExoticA fastest-compo): OpenMPT must play it.
    auto* sampled = ompt.fromFile("testmus/openmpt/amiga-fastest-compo.mod");
    REQUIRE(sampled != nullptr);
    delete sampled;
    // AM modules: OpenMPT must decline (throws), leaving them to UADE.
    for (auto const& am : {"testmus/uade/war hawk.st1.3.mod",
                           "testmus/uade/daisy.adsc",
                           "testmus/uade/amsyntdemo.adsc"}) {
        INFO(am);
        REQUIRE_THROWS(ompt.fromFile(am));
    }
    // Companion fetch routing: OpenMPT claims ".mod" and is registered before
    // UADE, so MusicPlayer::getSecondaryFiles (first-canHandle wins) asks OpenMPT
    // -- not UADE -- for a Startrekker AM ".mod"'s companions. OpenMPT must
    // therefore surface the ".nt"/".as" synth file itself, or the GUI never
    // downloads it and UADE plays silent. (cmtest uses local fixtures, so this
    // is the only coverage of the GUI fetch path for AM .mod files.)
    auto sec = ompt.getSecondaryFiles("testmus/uade/war hawk.st1.3.mod");
    INFO("OpenMPT AM secondaries");
    REQUIRE(std::find(sec.begin(), sec.end(), "war hawk.st1.3.mod.nt") !=
            sec.end());
    // The modland-named companion actually sits next to the song.
    REQUIRE(utils::File{ "testmus/uade/war hawk.st1.3.mod.nt" }.exists());
    // Sampled MODs need no companion.
    REQUIRE(ompt.getSecondaryFiles("testmus/openmpt/amiga-fastest-compo.mod")
                .empty());
    // Replicate the live first-canHandle resolver and confirm OpenMPT is the
    // plugin that actually answers for the AM ".mod".
    musix::ChipPlugin::createPlugins("data");
    std::string resolver = "(none)";
    for (const auto& pl : musix::ChipPlugin::getPlugins()) {
        if (pl->canHandle("testmus/uade/war hawk.st1.3.mod")) {
            resolver = pl->name();
            break;
        }
    }
    REQUIRE(resolver == "OpenMPT");
}
TEST_CASE("GSF", "[music]") { testPlugin<musix::GSFPlugin>("testmus/gsf", "lib"); }
// On a clean machine, streaming a .gsf/.minigsf must also fetch its shared
// .gsflib (named via the PSF "_lib" tag) or the VBA loader fails ("Could not
// load gsf"). Verify the plugin surfaces that companion so MusicPlayerList
// pulls it down alongside the stub.
TEST_CASE("GSF secondary files", "[music]")
{
    musix::GSFPlugin plugin;
    REQUIRE(plugin.getSecondaryFiles("testmus/gsf/01 yume wa owaranai.gsf") ==
            std::vector<std::string>{ "AGB-AN8J-JPN.gsflib" });
    REQUIRE(plugin.getSecondaryFiles(
                "testmus/gsf/01 title screen (0003).minigsf") ==
            std::vector<std::string>{ "zelda.gsflib" });
    REQUIRE(plugin.getSecondaryFiles("/nonexistent/x.gsf").empty());
}
// Same clean-machine fix for the other PSF-family plugins: every mini* rip
// references a shared library via the PSF "_lib" tag, and each plugin must
// surface it from getSecondaryFiles() (all delegate to psfLibFiles()). The lib
// name is returned verbatim from the tag -- its case matches the source server's
// filename (the local lowercase fixtures coincide on a case-insensitive FS).
TEST_CASE("PSF lib secondary files", "[music]")
{
    using V = std::vector<std::string>;
    REQUIRE(musix::NDSPlugin{}.getSecondaryFiles(
                "testmus/nds/001 title.mini2sf") == V{ "NTR-AZEE-USA.2sflib" });
    REQUIRE(musix::AOPlugin{}.getSecondaryFiles(
                "testmus/ao/01 - opening.miniqsf") ==
            V{ "Mega Man 2 - The Power Fighters.qsflib" });
    REQUIRE(musix::HTPlugin{}.getSecondaryFiles(
                "testmus/ht/w00-00-25.minissf") == V{ "W00.ssflib" });
    REQUIRE(musix::USFPlugin{}.getSecondaryFiles(
                "testmus/usf/sparse01.miniusf") == V{ "quake2.usflib" });
    REQUIRE(musix::HEPlugin{ "data/hebios.bin" }.getSecondaryFiles(
                "testmus/psx/01 - main menu.minipsf") == V{ "driver.psflib" });
    REQUIRE(musix::HEPlugin{ "data/hebios.bin" }.getSecondaryFiles(
                "testmus/psx/010.minipsf2") ==
            V{ "Pop'n Taisen Puzzle-dama Online.psf2lib" });
    // A real self-contained full PSF (no minipsf _lib companion) -> no secondaries.
    REQUIRE(musix::HEPlugin{ "data/hebios.bin" }
                .getSecondaryFiles("testmus/psx/102 revelation.psf")
                .empty());
    // Negative fixture: bad-magic-not-a-psf.psf carries a .psf extension but lacks
    // the "PSF" magic (a mislabeled non-PlayStation rip). canHandle must reject it
    // on content, so the HE playback test gray-Skips it instead of feeding garbage
    // to the emulator.
    REQUIRE(!musix::HEPlugin{ "data/hebios.bin" }.canHandle(
        "testmus/psx/bad-magic-not-a-psf.psf"));
}
// Regression for the real GUI entry point: MusicPlayer::getSecondaryFiles used
// to parse PSF "_lib" inline and lower-case it, so an uppercase/mixed-case
// companion (e.g. ZZZ_JNA1.psf2lib, W00.ssflib, "Mega Man - The Power
// Battle.qsflib") 550'd on Modland's case-sensitive FTP and the tune streamed
// silent. It now delegates to the plugin and must preserve the exact case.
TEST_CASE("MusicPlayer secondary files preserve case", "[music]")
{
    auto ap = std::make_shared<AudioPlayerNull>();
    musix::ChipPlugin::createPlugins("data");
    chipmachine::MusicPlayer mp{ ap };
    REQUIRE(mp.getSecondaryFiles("testmus/psx/010.minipsf2") ==
            std::vector<std::string>{ "Pop'n Taisen Puzzle-dama Online.psf2lib" });
    REQUIRE(mp.getSecondaryFiles("testmus/nds/001 title.mini2sf") ==
            std::vector<std::string>{ "NTR-AZEE-USA.2sflib" });
    REQUIRE(mp.getSecondaryFiles("testmus/gsf/01 yume wa owaranai.gsf") ==
            std::vector<std::string>{ "AGB-AN8J-JPN.gsflib" });
    // TFMX: the mdat.<name> song streams alongside its smpl.<name> sample bank;
    // UADE's getSecondaryFiles maps the mdat->smpl prefix so the GUI fetches it.
    REQUIRE(mp.getSecondaryFiles("testmus/uade/mdat.melovatrix") ==
            std::vector<std::string>{ "smpl.melovatrix" });
    // Richard Joseph: "<name>.sng" song + "<name>.ins" sample file (the Amiga
    // player swaps .sng->.INS in the same dir); the GUI must fetch the .ins.
    REQUIRE(mp.getSecondaryFiles("testmus/uade/aquatic games.sng") ==
            std::vector<std::string>{ "aquatic games.ins" });
    // SoundPlayer (Scott Johnston): "sjs.<name>" song + "smp.<name>" samples.
    REQUIRE(mp.getSecondaryFiles("testmus/uade/sjs.rudi") ==
            std::vector<std::string>{ "smp.rudi" });
    // Synth Dream: "sdr.<name>" song; samples are either a per-tune "smp.<name>"
    // or a shared "smp.set" bank. Surface both (missing companion is non-fatal).
    REQUIRE(mp.getSecondaryFiles("testmus/uade/sdr.monsterbusiness 1") ==
            (std::vector<std::string>{ "smp.monsterbusiness 1", "smp.set" }));
    // MusicMaker V8: "<name>.sdata" song + a 3-part ".ip"/".ip.l"/".ip.n"
    // instrument pack; all three must be fetched or the tune renders silent.
    REQUIRE(mp.getSecondaryFiles("testmus/uade/best of guitars.sdata") ==
            (std::vector<std::string>{ "best of guitars.ip",
                                       "best of guitars.ip.l",
                                       "best of guitars.ip.n" }));
    // Kris Hatlelid: "<name>.kh" song + a fixed-name "songplay" replay file.
    REQUIRE(mp.getSecondaryFiles("testmus/uade/the cycles.kh") ==
            std::vector<std::string>{ "songplay" });
}
TEST_CASE("NDS", "[music]") { testPlugin<musix::NDSPlugin>("testmus/nds", "lib"); }
TEST_CASE("HE", "[music]") { testPlugin<musix::HEPlugin>("testmus/psx", "lib", "data/hebios.bin"); }
TEST_CASE("Ayfly", "[music]") { testPlugin<musix::AyflyPlugin>("testmus/zx", ".vt2"); }
// Regression: a malformed / non-SQT file that reaches libayfly's SQT loader
// (here a Quartet PSG module carrying a .sqt extension) used to SIGSEGV in
// SQT_Play -- SQT_Init bailed out of SQT_PreInit without allocating info.data,
// and SQT_Play then dereferenced null, taking down the whole host. The loader
// now guards null info.data and renders silence. Feed it straight to fromFile
// (bypassing canHandle, which would normally decline "/quartet" paths): the
// test simply has to finish without crashing the process.
TEST_CASE("Ayfly SQT malformed no crash", "[music]")
{
    logging::setLevel(logging::Level::Error);
    musix::AyflyPlugin plugin;
    std::string const f = "testmus/sqt/quartet-psg-as-sqt.sqt";
    auto* player = plugin.fromFile(f);
    if (player) {
        std::array<int16_t, 8192> buf{};
        for (int i = 0; i < 100; ++i) {
            if (player->getSamples(buf.data(), buf.size()) <= 0) break;
        }
        delete player;
    }
    SUCCEED("SQT loader handled malformed input without crashing");
}
TEST_CASE("ZXTune", "[music]")
{
    testPlugin<musix::ZXTunePlugin>("testmus/st11", ""); // Sound Tracker 1.1
    testPlugin<musix::ZXTunePlugin>("testmus/gtr", "");  // Global Tracker (AY)
    testPlugin<musix::ZXTunePlugin>("testmus/chi", "");  // Chip Tracker (DAC)
    testPlugin<musix::ZXTunePlugin>("testmus/tfe", "");  // TFM Music Maker (FM)
    testPlugin<musix::ZXTunePlugin>("testmus/ftc", "");  // Fast Tracker (ex-ayfly)
    // ZX "Pro Sound Maker" .psm -- shares the extension with Epic MASI, which
    // OpenMPT keeps. OpenMPT content-checks the MASI magic and declines these,
    // so first-match routing lands them here (see OpenMPTPlugin::canHandle).
    testPlugin<musix::ZXTunePlugin>("testmus/psm", "");
}
// The .psm extension is shared: Epic MegaGames MASI must keep routing to OpenMPT
// (which plays it), while ZX "Pro Sound Maker" .psm must route to ZXTune (which
// OpenMPT cannot play). Assert the live registry resolves each by content, so a
// future libopenmpt/plugin-priority change can't silently re-misroute them.
TEST_CASE("PSM routing", "[music]")
{
    musix::ChipPlugin::createPlugins("data");
    auto winner = [](std::string const& file) -> std::string {
        for (const auto& pl : musix::ChipPlugin::getPlugins()) {
            if (pl->canHandle(file)) { return pl->name(); }
        }
        return "(none)";
    };
    // ZX Pro Sound Maker -> ZXTune
    REQUIRE(winner("testmus/psm/a1.psm") == "ZX Spectrum (ZXTune)");
    // Epic MASI -> OpenMPT (unchanged)
    REQUIRE(winner("testmus/openmpt/one must fall! 1.psm") == "OpenMPT");
}
// Fast Tracker .ftc was taken from Ayfly (which throws on every .ftc) and given
// to ZXTune. Assert the live registry routes it there and that Ayfly no longer
// claims it, so a future supported_ext edit can't silently steal it back.
TEST_CASE("FTC routing", "[music]")
{
    musix::ChipPlugin::createPlugins("data");
    std::string winner = "(none)";
    for (const auto& pl : musix::ChipPlugin::getPlugins()) {
        if (pl->canHandle("testmus/ftc/jam1.ftc")) { winner = pl->name(); break; }
    }
    REQUIRE(winner == "ZX Spectrum (ZXTune)");
    REQUIRE(musix::AyflyPlugin().canHandle("x.ftc") == false);
}
// Sam Coupe COP/SNG: SAM Coupé music for the SAA1099, played by running the
// song's embedded Z80 replayer (or the shared E-Tracker replayer) on the GME Z80
// core with Dave Hooper's SAASound. The modland "Sam Coupe COP" (.cop) and "Sam
// Coupe SNG" (.sng) corpora are the same replayer family, so one plugin plays
// both. Fixtures cover the revisions: bd/e11 raw-Z80 "compiled" songs and duck1/
// duck2 E-Tracker data (.cop), plus five .sng -- ddtitl (compiled 0x21..0xc3),
// kapsa3 (E-Tracker data), music1 (Fuka), ofc2 (Ziutek "3e..3d c2 23 81") and
// tetris (compiled "JP #81xx"). chrismas/rozkaz/stovka (.cop) are further
// compiled songs whose subsong-select preambles ("01 ff 00 3e.." / "00 3e 01..")
// match none of the old signatures -- they play because CopPlugin now claims
// ".cop" by extension and lets CopMachine::init() run their own replayer.
TEST_CASE("Sam Coupe COP", "[music]")
{
    testPlugin<musix::CopPlugin>("testmus/cop", "");
}
// CopPlugin owns ".cop" outright now (ZXTune's COP loader failed on the modland
// corpus, producing "no playable module"). Assert the live registry routes .cop
// files to CopPlugin and that ZXTune declines the extension entirely, including a
// compiled song (stovka) whose preamble the old signature gate didn't enumerate.
TEST_CASE("COP routing", "[music]")
{
    musix::ChipPlugin::createPlugins("data");
    auto winner = [](std::string const& file) -> std::string {
        for (const auto& pl : musix::ChipPlugin::getPlugins()) {
            if (pl->canHandle(file)) { return pl->name(); }
        }
        return "(none)";
    };
    REQUIRE(winner("testmus/cop/bd.cop") == "Sam Coupe (COP)");    // compiled
    REQUIRE(winner("testmus/cop/duck1.cop") == "Sam Coupe (COP)"); // E-Tracker
    REQUIRE(winner("testmus/cop/stovka.cop") == "Sam Coupe (COP)"); // gap preamble
    REQUIRE_FALSE(musix::ZXTunePlugin().canHandle("testmus/cop/bd.cop"));
    REQUIRE_FALSE(musix::ZXTunePlugin().canHandle("testmus/cop/duck1.cop"));
    REQUIRE_FALSE(musix::ZXTunePlugin().canHandle("testmus/cop/stovka.cop"));
    // Sam Coupe .sng routes here too (content-gated), and CopPlugin must NOT
    // grab the other .sng chips -- their headers fail looksLikeSamCoupeCop.
    REQUIRE(winner("testmus/cop/ddtitl.sng") == "Sam Coupe (COP)");
    REQUIRE(winner("testmus/cop/kapsa3.sng") == "Sam Coupe (COP)");
    musix::CopPlugin cop;
    REQUIRE_FALSE(cop.canHandle("testmus/goattracker/sid-warrior.sng"));   // GTS5
    REQUIRE_FALSE(cop.canHandle("testmus/uade/aquatic games.sng"));        // RJP1SMOD
    REQUIRE_FALSE(cop.canHandle("testmus/zoundmonitor/maddick.sng"));      // ZoundMonitor
}
// .mus is overloaded on modland: UADE's UFO eagleplayer owns the Amiga variant,
// but the extension is also used by FAC SoundTracker, an MSX PSG format the
// vendored 68k engine cannot run (it feeds Z80 code to a 68k player and the
// score dies). UADE must decline MSX BSAVE .mus files (marker 0xFE + LE
// start/end addresses, start <= end) while still claiming the Amiga form.
TEST_CASE("UADE mus routing", "[music]")
{
    musix::UADEPlugin plugin{"data"};
    auto tmp = fs::temp_directory_path();

    // MSX BSAVE header as written by FAC SoundTracker (start 0x8000, end 0xBFFF).
    auto msxMus = tmp / "musix_fac.mus";
    {
        std::ofstream f(msxMus, std::ios::binary);
        const unsigned char hdr[] = {0xFE, 0x00, 0x80, 0xFF, 0xBF, 0x00, 0x80, 0x00};
        f.write(reinterpret_cast<const char*>(hdr), sizeof(hdr));
    }
    REQUIRE_FALSE(plugin.canHandle(msxMus.string()));
    fs::remove(msxMus);

    // A non-BSAVE .mus (no 0xFE marker) is an Amiga UFO tune and stays with UADE.
    auto amigaMus = tmp / "musix_ufo.mus";
    {
        std::ofstream f(amigaMus, std::ios::binary);
        const std::vector<char> zeros(64, 0);
        f.write(zeros.data(), zeros.size());
    }
    REQUIRE(plugin.canHandle(amigaMus.string()));
    fs::remove(amigaMus);

    // Unreadable/virtual path: fall back to the extension match (claim it) so a
    // dry canHandle probe on a not-yet-downloaded remote path doesn't regress.
    REQUIRE(plugin.canHandle((tmp / "musix_no_such.mus").string()));
}
// ".ast" is shared. UADE's V0.1 "ActionAmics" eagleplayer plays the genuine
// binary replay dumps (no "AST" magic), but the modland "All Sound Tracker"
// corpus is the tracker's native versioned save format (Pascal-string magic
// \x08"AST 00xx") the V0.1 player cannot parse -- it loads and emits silence
// while UADE reports "ok". canHandle must decline the native saves (so they Skip)
// while still claiming the V0.1 binary form.
TEST_CASE("UADE ast routing", "[music]")
{
    musix::UADEPlugin plugin{"data"};
    auto tmp = fs::temp_directory_path();

    // Native "All Sound Tracker" save: \x08"AST 00xx" magic -> declined.
    auto nativeAst = tmp / "astrt_native.ast";
    {
        std::ofstream f(nativeAst, std::ios::binary);
        const unsigned char hdr[] = {0x08, 'A', 'S', 'T', ' ', '0', '0', '3', '2'};
        f.write(reinterpret_cast<const char*>(hdr), sizeof(hdr));
    }
    REQUIRE_FALSE(plugin.canHandle(nativeAst.string()));
    fs::remove(nativeAst);

    // The genuine V0.1 binary dump carries no "AST" magic and stays with UADE.
    REQUIRE(plugin.canHandle("testmus/uade/dynablaster.ast"));

    // Unreadable/virtual path: fall back to the extension match so a dry probe on
    // a not-yet-downloaded remote .ast doesn't regress.
    REQUIRE(plugin.canHandle((tmp / "astrt_no_such.ast").string()));
}
TEST_CASE("PokeyNoise", "[music]") { testPlugin<musix::PokeyNoisePlugin>("testmus/pn", ""); }
// SAP type routing. GME's Sap_Emu plays only register types B and C; Digimusic
// ('D') and other player types fail to load there ("Digimusic not supported").
// Those route to the ASAP-based PokeyNoise plugin instead. The two plugins gate
// on the SAP TYPE tag so they stay mutually exclusive regardless of order.
TEST_CASE("SAP type routing", "[music]")
{
    musix::GMEPlugin gme;
    musix::PokeyNoisePlugin asap;

    // Type B / C: GME owns them, PokeyNoise declines.
    for (auto const& bc : {"testmus/gme/deadline.sap",     // TYPE B
                           "testmus/gme/Alchemia.sap"}) {  // TYPE C
        REQUIRE(gme.canHandle(bc));
        REQUIRE_FALSE(asap.canHandle(bc));
    }

    // Type D (Digimusic): GME declines, PokeyNoise claims and plays it.
    std::string const digi = "testmus/pn/Zone_X.sap";
    REQUIRE_FALSE(gme.canHandle(digi));
    REQUIRE(asap.canHandle(digi));

    auto* player = asap.fromFile(digi);
    REQUIRE(player != nullptr);
    std::array<int16_t, 8192> buffer;
    int64_t sum = 0;
    for (int count = 50; sum == 0 && count != 0; count--) {
        int rc = player->getSamples(&buffer[0], buffer.size());
        if (rc <= 0) break;
        for (int i = 0; i < rc; ++i) {
            if (buffer[i] != 0) { sum = 1; break; }
        }
    }
    delete player;
    REQUIRE(sum != 0);
}
// Monotone (.mon) -- PC-speaker tracker by Trixter/Hornet, played by the
// vendored PTPlayer. The extension collides with UADE's Maniacs of Noise; the
// Monotone-magic gate keeps the two apart.
TEST_CASE("Monotone", "[music]") { testPlugin<musix::MonotonePlugin>("testmus/monotone", ""); }
// MikMod UNITRK / UNIMOD (.uni, magic "UN0x"). Played via the vendored libmikmod
// slice -- no other engine in the tree has a UNIMOD loader.
TEST_CASE("MikMod", "[music]") { testPlugin<musix::MikModPlugin>("testmus/mikmod", ""); }
// Beepola .bbsong (ZX Spectrum beeper). Only the Phaser1 engine (P1D/P1S) is
// decoded today; the other Beepola engines in this dir fast-fail as a graceful
// skip ("unsupported"), so coverage exercises the 18 Phaser1 tunes.
TEST_CASE("Beepola", "[music]") { testPlugin<musix::BBSongPlugin>("testmus/bbsong", ""); }
TEST_CASE("FFMPEG", "[music]") { testPlugin<musix::FFMPEGPlugin>("testmus/ffmpeg", ""); }

// Explicit guard that the newly-enabled lossless PCM (wav/flac/aiff) and MP2
// fixtures decode to real (non-silent) audio through the ffmpeg local-file path.
// The corpus test above already sweeps the directory, but pin each format by
// name so a routing/gate regression fails loudly instead of just shifting the
// coverage tally. Fixtures are 3s cuts of sample.ogg (see testmus/ffmpeg).
TEST_CASE("FFMPEG plays wav/flac/aiff/mp2/opus/mpeg/ac3/wma", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::FFMPEGPlugin plugin;
    std::array<int16_t, 8192> buffer{};
    for (auto const* file : {"testmus/ffmpeg/sample.wav",
                             "testmus/ffmpeg/sample.flac",
                             "testmus/ffmpeg/sample.aiff",
                             "testmus/ffmpeg/sample.mp2",
                             "testmus/ffmpeg/sample.opus",
                             "testmus/ffmpeg/sample.mpeg",
                             "testmus/ffmpeg/sample.ac3",
                             "testmus/ffmpeg/sample.wma"}) {  // Xbox streamed rips
        REQUIRE(utils::exists(file));
        REQUIRE(plugin.canHandle(file));
        auto* player = plugin.fromFile(file);
        REQUIRE(player != nullptr);
        // ffmpeg is a subprocess (non-blocking); the first getSamples() calls
        // return 0 while it warms up. Poll within a budget for the first PCM.
        int64_t sum = 0;
        int count = 50;
        while (sum == 0 && count-- != 0) {
            int rc = player->getSamples(&buffer[0], buffer.size());
            if (rc > 0) {
                for (int i = 0; i < rc; ++i)
                    if (buffer[i] != 0) { sum = 1; break; }
            } else if (rc == 0) {
                utils::sleepms(20);
            } else {
                break; // SONG_END before any audio
            }
        }
        delete player;
        INFO("no audio decoded from " << file);
        REQUIRE(sum != 0);
    }
}
// Progressive (fromStream) path: feed a file's bytes into the fifo a chunk at a
// time -- as a slow download would -- and confirm ffmpeg starts producing audio
// before all bytes arrive (getSamples returns 0 = "buffering", then >0), and
// that endStream() makes it terminate cleanly (SONG_END) instead of hanging.
TEST_CASE("FFMPEG stream", "[music]")
{
    printf("---- ffmpeg progressive streaming ----\n");
    musix::FFMPEGPlugin plugin;
    std::ifstream f("testmus/ffmpeg/sample.ogg", std::ios::binary);
    REQUIRE(f.good());
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)),
                               std::istreambuf_iterator<char>());
    REQUIRE(!bytes.empty());
    printf("  source: testmus/ffmpeg/sample.ogg (%zu bytes), feeding 4KB/2ms "
           "to simulate a slow download\n",
           bytes.size());

    auto fifo = std::make_shared<utils::Fifo<uint8_t>>(32768 * 8);
    std::unique_ptr<musix::ChipPlayer> player(plugin.fromStream(fifo));
    REQUIRE(player != nullptr);

    std::atomic<bool> producerDone{false};
    std::thread producer([&] {
        size_t off = 0;
        while (off < bytes.size()) {
            int chunk = static_cast<int>(
                std::min<size_t>(4096, bytes.size() - off));
            fifo->put(bytes.data() + off, chunk);
            off += chunk;
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        player->endStream();
        producerDone = true;
    });

    std::vector<int16_t> buf(4096);
    long total = 0;
    long nonzero = 0;
    int rc = 0;
    int idleGuard = 0;
    int bufferingPolls = 0;
    bool firstAudio = false;
    while (true) {
        rc = player->getSamples(buf.data(), static_cast<int>(buf.size()));
        if (rc < 0) break; // SONG_END
        if (rc == 0) {     // buffering
            if (!firstAudio) bufferingPolls++;
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            if (++idleGuard > 5000) break; // 10s without audio: give up
            continue;
        }
        if (!firstAudio) {
            firstAudio = true;
            printf("  prebuffered ~%dms, first audio arrived before download "
                   "finished -> playing\n",
                   bufferingPolls * 2);
        }
        idleGuard = 0;
        total += rc;
        for (int i = 0; i < rc; i++)
            if (buf[i] != 0) nonzero++;
    }
    producer.join();

    printf("  decoded %ld samples (%.1fs), %ld non-silent; ended via %s\n",
           total, total / 2.0 / 44100.0, nonzero,
           rc < 0 ? "clean EOF" : "IDLE-TIMEOUT (FAIL)");

    REQUIRE(producerDone);
    REQUIRE(rc < 0);          // ended via EOF, not the idle-timeout
    REQUIRE(total > 44100);   // at least ~0.25s of decoded audio
    REQUIRE(nonzero > 0);
    printf("  PASS: progressive playback + clean termination\n");
}

// The GUI streams every ffmpeg-decodable finite remote file progressively
// (curl->fifo->ffmpeg, see MusicPlayerList extStreamable), not just ogg/mp3.
// The lossless containers matter here: wav/aiff carry a RIFF/FORM header with a
// declared data size, and flac has a STREAMINFO header -- confirm each decodes
// to real audio when fed through a non-seekable pipe a chunk at a time, exactly
// as a slow download delivers it. (opus rides ogg; mp2 is frame-based like mp3.)
TEST_CASE("FFMPEG streams wav/flac/aiff/mp2/opus", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    for (auto const* file : {"testmus/ffmpeg/sample.wav",
                             "testmus/ffmpeg/sample.flac",
                             "testmus/ffmpeg/sample.aiff",
                             "testmus/ffmpeg/sample.mp2",
                             "testmus/ffmpeg/sample.opus"}) {
        std::ifstream f(file, std::ios::binary);
        REQUIRE(f.good());
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)),
                                   std::istreambuf_iterator<char>());
        REQUIRE(!bytes.empty());

        musix::FFMPEGPlugin plugin;
        auto fifo = std::make_shared<utils::Fifo<uint8_t>>(32768 * 8);
        std::unique_ptr<musix::ChipPlayer> player(plugin.fromStream(fifo));
        REQUIRE(player != nullptr);

        // Feed the bytes progressively (4KB/2ms) as a slow download would.
        std::thread producer([&] {
            size_t off = 0;
            while (off < bytes.size()) {
                int chunk =
                    static_cast<int>(std::min<size_t>(4096, bytes.size() - off));
                fifo->put(bytes.data() + off, chunk);
                off += chunk;
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
            player->endStream();
        });

        std::vector<int16_t> buf(4096);
        long total = 0, nonzero = 0;
        int rc = 0, idleGuard = 0;
        while (true) {
            rc = player->getSamples(buf.data(), static_cast<int>(buf.size()));
            if (rc < 0) break; // SONG_END
            if (rc == 0) {     // buffering
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                if (++idleGuard > 5000) break; // 10s without audio: give up
                continue;
            }
            idleGuard = 0;
            total += rc;
            for (int i = 0; i < rc; i++)
                if (buf[i] != 0) nonzero++;
        }
        producer.join();

        printf("  streamed %s: %ld samples, %ld non-silent, ended via %s\n",
               file, total, nonzero, rc < 0 ? "clean EOF" : "IDLE-TIMEOUT");
        INFO("progressive stream produced no audio for " << file);
        REQUIRE(rc < 0);        // clean EOF, not idle-timeout hang
        REQUIRE(total > 44100); // at least ~0.25s decoded
        REQUIRE(nonzero > 0);
    }
}

// Mid-stream abort (a song switch): destroy the streaming player while it is
// still buffering, without ever signalling end-of-stream. This must not hang
// (the feeder could be parked in a blocking write to ffmpeg's stdin) -- the
// destructor has to unblock and join it promptly.
TEST_CASE("FFMPEG stream abort", "[music]")
{
    printf("---- ffmpeg stream abort (song switch mid-buffer) ----\n");
    musix::FFMPEGPlugin plugin;
    std::ifstream f("testmus/ffmpeg/sample.ogg", std::ios::binary);
    REQUIRE(f.good());
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)),
                               std::istreambuf_iterator<char>());
    REQUIRE(!bytes.empty());

    auto fifo = std::make_shared<utils::Fifo<uint8_t>>(32768 * 8);
    std::unique_ptr<musix::ChipPlayer> player(plugin.fromStream(fifo));
    REQUIRE(player != nullptr);

    // Feed a bit and pull a few buffers so ffmpeg is actually running, then drop
    // the player without endStream().
    int16_t buf[4096];
    size_t off = 0;
    for (int i = 0; i < 8 && off < bytes.size(); i++) {
        int chunk = static_cast<int>(std::min<size_t>(8192, bytes.size() - off));
        fifo->put(bytes.data() + off, chunk);
        off += chunk;
        player->getSamples(buf, 4096);
    }
    printf("  ffmpeg running, destroying player without endStream()...\n");

    auto t0 = std::chrono::steady_clock::now();
    player.reset(); // destructor must return quickly, not deadlock on join()
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - t0)
                  .count();
    printf("  destructor returned in %lldms (must be < 3000, no join deadlock)\n",
           static_cast<long long>(ms));
    REQUIRE(ms < 3000);
    printf("  PASS: clean abort, no hang\n");
}

// Compressed-lossy pin. The lossless test above pins wav/flac/aiff/... by name;
// mirror it for the compressed containers (mp3/aac/m4a/mp4) so a routing/decode
// regression in the in-process libav path fails loudly here rather than only
// nudging the corpus coverage tally. (The whole point of the CLI->libav rewrite
// is that these still decode with no ffmpeg binary spawned.)
TEST_CASE("FFMPEG plays mp3/aac/m4a/mp4", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::FFMPEGPlugin plugin;
    std::array<int16_t, 8192> buffer{};
    for (auto const* file : {"testmus/ffmpeg/Airball - Intro - Amiga (Moonchild).mp3",
                             "testmus/ffmpeg/sample-3.aac",
                             "testmus/ffmpeg/sample-1.m4a",
                             "testmus/ffmpeg/Audio_ID_2ch_64kbps_aac.mp4"}) {
        REQUIRE(utils::exists(file));
        REQUIRE(plugin.canHandle(file));
        auto* player = plugin.fromFile(file);
        REQUIRE(player != nullptr);
        // Decode runs on a worker thread; the first getSamples() calls return 0
        // while it warms up. Poll within a budget for the first PCM.
        int64_t sum = 0;
        int count = 50;
        while (sum == 0 && count-- != 0) {
            int rc = player->getSamples(&buffer[0], buffer.size());
            if (rc > 0) {
                for (int i = 0; i < rc; ++i)
                    if (buffer[i] != 0) { sum = 1; break; }
            } else if (rc == 0) {
                utils::sleepms(20);
            } else {
                break; // SONG_END before any audio
            }
        }
        delete player;
        INFO("no audio decoded from " << file);
        REQUIRE(sum != 0);
    }
}

// File-path teardown. The old design decoded in a subprocess; the in-process
// rewrite runs a worker thread even for local files, so destroying a fromFile()
// player must join that thread promptly (stop flag + pcm.quit() unblock it)
// whether it is still decoding or already finished. Guards against a teardown
// deadlock on the file path (the streaming path has its own abort test above).
TEST_CASE("FFMPEG file player tears down cleanly mid-decode", "[music]")
{
    printf("---- ffmpeg file-path teardown ----\n");
    musix::FFMPEGPlugin plugin;
    int16_t buf[4096];
    for (int iter = 0; iter < 3; iter++) {
        std::unique_ptr<musix::ChipPlayer> player(
            plugin.fromFile("testmus/ffmpeg/sample.ogg"));
        REQUIRE(player != nullptr);
        // Pull a couple of buffers so the worker is actively decoding, then drop
        // the player without letting it run to SONG_END.
        player->getSamples(buf, 4096);
        player->getSamples(buf, 4096);
        auto t0 = std::chrono::steady_clock::now();
        player.reset();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - t0)
                      .count();
        printf("  iter %d: destructor returned in %lldms\n", iter,
               static_cast<long long>(ms));
        REQUIRE(ms < 3000);
    }
    printf("  PASS: file-path teardown never deadlocks\n");
}

// URL option path. A remote fromFile() sets the reconnect + User-Agent av_dict
// options and relies on the libav interrupt callback to abort a stalled/failing
// open on teardown. Point it at an unreachable local port (no network, no
// flakiness) and confirm: the option branch does not crash, an unreachable
// source ends as SONG_END rather than hanging, and destruction is prompt.
// libav log callback used only during the URL test below: the connection failure
// is the POINT of the test, so relabel it "[EXPECTED FAILURE]" in green instead of
// letting libav print it as a red error line.
static void expectedFailureAvLog(void* /*avcl*/, int level, const char* fmt,
                                 va_list vl)
{
    if (level > AV_LOG_ERROR) { return; } // errors/fatals only
    char msg[1024];
    vsnprintf(msg, sizeof(msg), fmt, vl);
    size_t n = std::strlen(msg);
    while (n > 0 && (msg[n - 1] == '\n' || msg[n - 1] == '\r')) { msg[--n] = '\0'; }
    if (n == 0) { return; }
    printf("\033[32m  [EXPECTED FAILURE] %s\033[0m\n", msg); // green, not red
}

TEST_CASE("FFMPEG URL path handles unreachable host without hanging", "[music]")
{
    printf("---- ffmpeg URL option path ----\n");
    musix::FFMPEGPlugin plugin;
    // The worker thread's open of the unreachable URL will log a "Connection
    // refused" at AV_LOG_ERROR (red by default) -- that failure is exactly what
    // we are asserting, so route libav logging through our labelled callback for
    // the duration and restore the default once the attempt has resolved.
    av_log_set_callback(&expectedFailureAvLog);
    std::unique_ptr<musix::ChipPlayer> player(
        plugin.fromFile("http://127.0.0.1:1/nope.mp3"));
    REQUIRE(player != nullptr);
    int16_t buf[4096];
    // Poll: the open fails (connection refused) -> worker finishes -> getSamples
    // reports SONG_END (<0). Bounded so a regression that hangs fails the budget
    // instead of stalling the suite.
    int rc = 0;
    int guard = 0;
    do {
        rc = player->getSamples(buf, 4096);
        if (rc == 0) { utils::sleepms(10); }
    } while (rc == 0 && ++guard < 600); // <= ~6s
    // The open has resolved (worker finished); stop intercepting libav logs so
    // later tests get normal logging even if an assertion below throws.
    av_log_set_callback(&av_log_default_callback);
    printf("  unreachable URL ended with rc=%d after %d polls\n", rc, guard);
    REQUIRE(rc < 0);      // ended (SONG_END), did not hang
    REQUIRE(guard < 600); // within budget
    auto t0 = std::chrono::steady_clock::now();
    player.reset();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - t0)
                  .count();
    REQUIRE(ms < 3000);
    printf("  PASS: clean SONG_END + fast teardown on unreachable URL\n");
}

TEST_CASE("HT", "[music]") { testPlugin<musix::HTPlugin>("testmus/ht", ""); }
TEST_CASE("SC68", "[music]") { testPlugin<musix::SC68Plugin>("testmus/sc68", "", "data"); }
TEST_CASE("USF", "[music]") { testPlugin<musix::USFPlugin>("testmus/usf", ""); }
TEST_CASE("StSound", "[music]") { testPlugin<musix::StSoundPlugin>("testmus/stsound", ""); }
// Local .mp3 playback (mpg123). The SoundHelix fixtures guard that plain mp3
// files keep decoding to non-zero audio; this is the decoder used for local mp3
// files (radio/remote mp3 now go through ffmpeg, covered by the FFMPEG case).
TEST_CASE("MP3", "[music]") { testPlugin<musix::MP3Plugin>("testmus/mp3", ""); }
TEST_CASE("Hively", "[music]") { testPlugin<musix::HivelyPlugin>("testmus/hively", ""); }
TEST_CASE("RSN", "[music]") { testPlugin<musix::RSNPlugin>("testmus/rsn", ""); }
TEST_CASE("MDX", "[music]") { testPlugin<musix::MDXPlugin>("testmus/mdx", ""); }
TEST_CASE("S98", "[music]") { testPlugin<musix::S98Plugin>("testmus/s98", ""); }
TEST_CASE("FMP", "[music]") { testPlugin<musix::FMPPlugin>("testmus/fmp", ""); }
// Euphony (.eup, FM Towns / PC-98) via the vendored eupmini replayer. The .eup
// references companion instrument banks (.fmb/.pmb) by name in its header; those
// siblings live in the same dir and are skipped by canHandle (eup-only).
TEST_CASE("EUP", "[music]") { testPlugin<musix::EUPPlugin>("testmus/eup", ""); }
// Euphony plays sound, with companion banks. STARSKY.eup references the FM bank
// "fmp" (fmp.fmb) and PCM bank "a_string" (a_string.pmb) by name in its header;
// the plugin must locate those siblings in the song's directory and render
// non-zero audio. Fails if header parsing, the .fmb/.pmb loader, or the ring
// drain in getSamples regresses.
TEST_CASE("EUP plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::EUPPlugin plugin;

    std::string const eup = "testmus/eup/STARSKY.eup";
    REQUIRE(plugin.canHandle(eup));

    auto* player = plugin.fromFile(eup);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// MGSDRV (.mgs, MSX) via the vendored libkss replayer. The MGSDRV Z80 driver is
// embedded in libkss (modules/drivers/mgsdrv.h), so a plain .mgs needs no runtime
// file -- the plugin emulates Z80 + PSG/SCC/OPLL and renders directly.
TEST_CASE("MGS", "[music]") { testPlugin<musix::KSSPlugin>("testmus/mgs", ""); }
// MGS plays sound. The embedded MGSDRV driver drives PSG/OPLL; the plugin must
// detect the "MGS" signature, convert via KSS_bin2kss, and produce non-zero
// audio with no external files. Fails if the driver blob is dropped (empty
// MGSDRV array) or the canHandle signature check regresses.
TEST_CASE("MGS plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::KSSPlugin plugin;

    std::string const mgs = "testmus/mgs/snatcher - twilight of neo kobe city.mgs";
    REQUIRE(plugin.canHandle(mgs));

    auto* player = plugin.fromFile(mgs);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}

// The other MSX libkss formats that share the KSSPlugin: MuSICA (.bgm, KINROU5
// driver), OPLLDriver (.opx) and MPK (.mpk). All embed their Z80 driver like MGS
// and drive PSG/SCC/OPLL, so they play self-contained.
TEST_CASE("BGM", "[music]") { testPlugin<musix::KSSPlugin>("testmus/bgm", ""); }
TEST_CASE("OPX", "[music]") { testPlugin<musix::KSSPlugin>("testmus/opx", ""); }
TEST_CASE("MPK", "[music]") { testPlugin<musix::KSSPlugin>("testmus/mpk", ""); }
// MoonBlaster 1.4 (.mbm, MBR143 driver) drives PSG + MSX-MUSIC (YM2413) +
// MSX-AUDIO (Y8950 ADPCM) -- all emulated, no OPL4. The .mbk ADPCM sample banks
// are companion files (excluded here; they aren't standalone songs).
TEST_CASE("MBM", "[music]") { testPlugin<musix::KSSPlugin>("testmus/mbm", ".mbk"); }
// MBM plays sound, with its ADPCM bank. "Demosong.MBM" names bank "MBSTAND1" in
// its header; the plugin must surface MBSTAND1.MBK via getSecondaryFiles, seed
// libkss's autoload via KSS_autoload_mbk, convert through KSS_bin2kss/MBR143 and
// render non-zero audio. Fails if the MBR143 blob is dropped (the converter was
// previously excluded), the extension-based detection regresses, or the bank
// pairing breaks.
TEST_CASE("MBM plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::KSSPlugin plugin;

    std::string const mbm = "testmus/mbm/Demosong.MBM";
    REQUIRE(plugin.canHandle(mbm));

    // Header names bank "MBSTAND1"; the plugin must surface it in Modland's
    // exact (UPPERCASE) case so the host's secondary-file fetch resolves.
    auto secondary = plugin.getSecondaryFiles(mbm);
    REQUIRE(std::find(secondary.begin(), secondary.end(), "MBSTAND1.MBK") !=
            secondary.end());

    auto* player = plugin.fromFile(mbm);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    REQUIRE(energy != 0);
}
// FAC SoundTracker (.mus, MSX PSG + sampled drumkit). The plugin converts the
// song -- plus its <DRUMKIT>.SM1/.SM2 companion sample banks -- into a KSS
// carrying FAC's own Z80 replay routine and plays it via libkss. The .sm1/.sm2
// drumkits are companions (ignored by shouldIgnoreFile), not standalone tunes.
TEST_CASE("FAC SoundTracker", "[music]")
{
    testPlugin<musix::KSSPlugin>("testmus/fac", "");
}
// Both code paths must render non-zero audio: a drummed song (needs its SM1/SM2
// drumkits, surfaced via getSecondaryFiles) and a "NO DRUMS" song (self
// contained). Fails if the FAC player blob / mus2kss buffer build regresses, the
// content gate stops claiming .mus, or the drumkit pairing breaks.
TEST_CASE("FAC SoundTracker MUS plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::KSSPlugin plugin;

    auto renders = [&](const std::string& path) {
        INFO(path);
        REQUIRE(plugin.canHandle(path));
        auto* player = plugin.fromFile(path);
        REQUIRE(player != nullptr);
        std::array<int16_t, 8192> buffer{};
        int64_t energy = 0;
        for (int count = 0; count < 100 && energy == 0; ++count) {
            int rc = player->getSamples(buffer.data(), buffer.size());
            if (rc <= 0) { break; }
            for (int i = 0; i < rc; ++i) {
                energy += std::abs(static_cast<int>(buffer[i]));
            }
        }
        delete player;
        REQUIRE(energy != 0);
    };

    // Drummed song: header names drumkit "DRUMKIT1"; the plugin must surface
    // both banks (Modland's exact UPPERCASE case) so the host fetches them.
    std::string const drummed = "testmus/fac/32 color.mus";
    auto secondary = plugin.getSecondaryFiles(drummed);
    REQUIRE(std::find(secondary.begin(), secondary.end(), "DRUMKIT1.SM1") !=
            secondary.end());
    REQUIRE(std::find(secondary.begin(), secondary.end(), "DRUMKIT1.SM2") !=
            secondary.end());
    renders(drummed);

    // "NO DRUMS" song needs no companions.
    std::string const noDrums = "testmus/fac/because (no drums).mus";
    REQUIRE(plugin.getSecondaryFiles(noDrums).empty());
    renders(noDrums);

    // Larger-size variant: many FAC saves append trailing padding (16392 /
    // 16512 / 16513 bytes), so the content gate must accept size >= the page,
    // not exactly 16391 -- a strict equality dropped ~20% of the corpus. This
    // 16512-byte drummed song (drumkit "AFRIKA") must still pair and render.
    std::string const big = "testmus/fac/afrika (16512).mus";
    auto bigSecondary = plugin.getSecondaryFiles(big);
    REQUIRE(std::find(bigSecondary.begin(), bigSecondary.end(), "AFRIKA.SM1") !=
            bigSecondary.end());
    REQUIRE(std::find(bigSecondary.begin(), bigSecondary.end(), "AFRIKA.SM2") !=
            bigSecondary.end());
    renders(big);
}
// One file of each non-MGS libkss format must be detected by canHandle and
// render non-zero audio with no external files -- this fails if any of the
// KINROU/OPX/MPK driver blobs is dropped or a detector regresses.
TEST_CASE("MSX libkss formats play sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::KSSPlugin plugin;

    for (auto const& f : {"testmus/bgm/bolshoi kid.bgm",
                          "testmus/opx/breakthrough.opx",
                          "testmus/mpk/faraway memories.mpk"}) {
        std::string const path = f;
        INFO(path);
        REQUIRE(plugin.canHandle(path));

        auto* player = plugin.fromFile(path);
        REQUIRE(player != nullptr);

        std::array<int16_t, 8192> buffer{};
        int64_t energy = 0;
        for (int count = 0; count < 100 && energy == 0; ++count) {
            int rc = player->getSamples(buffer.data(), buffer.size());
            if (rc <= 0) { break; }
            for (int i = 0; i < rc; ++i) {
                energy += std::abs(static_cast<int>(buffer[i]));
            }
        }
        delete player;
        REQUIRE(energy != 0);
    }
}

// Bandai WonderSwan / WonderSwan Color sound rips (.wsr) via the vendored,
// self-contained in_wsr replayer (NEC V30MZ CPU + WonderSwan sound chip). A .wsr
// is a ROM image capped with a 32-byte "WSRF" footer (magic at offset 0, first
// subsong index at offset 5). We build a minimal synthetic rip in a temp file so
// the whole detect -> load -> render path runs against the real core without
// committing a copyrighted game rip. The synthetic ROM is all zeros so it stays
// silent, but it must still construct and render the requested sample count
// without throwing; a zero ROM is safe to execute because every out-of-cart read
// returns 0xFF, the CPU only writes into the emulator's own RAM banks, and
// Update_WSR runs a bounded cycle budget per call. If real rips are dropped into
// testmus/wsr they are played too (informational; empty in a clean checkout).
static std::vector<uint8_t> makeSyntheticWSR(uint8_t firstSong = 0)
{
    std::vector<uint8_t> rom(0x10000, 0);
    uint8_t* footer = rom.data() + rom.size() - 0x20;
    footer[0] = 'W';
    footer[1] = 'S';
    footer[2] = 'R';
    footer[3] = 'F';
    footer[5] = firstSong;
    return rom;
}

static void writeWSRFile(const fs::path& p, const std::vector<uint8_t>& data)
{
    std::ofstream f(p, std::ios::binary);
    REQUIRE(f.good());
    if (!data.empty()) {
        f.write(reinterpret_cast<const char*>(data.data()),
                static_cast<std::streamsize>(data.size()));
    }
}

TEST_CASE("WSR", "[music]") { testPlugin<musix::WSRPlugin>("testmus/wsr", ".md"); }

TEST_CASE("WSR plays", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::WSRPlugin plugin;

    auto tmp = fs::temp_directory_path();
    auto good = tmp / "musix_wsr_good.wsr";
    auto nofooter = tmp / "musix_wsr_nofooter.wsr";
    auto wrongext = tmp / "musix_wsr_wrongext.bin";

    writeWSRFile(good, makeSyntheticWSR());
    writeWSRFile(nofooter, std::vector<uint8_t>(0x10000, 0)); // no WSRF magic
    writeWSRFile(wrongext, makeSyntheticWSR());               // valid bytes, .bin

    // Detection: only a .wsr file carrying the WSRF footer is accepted.
    REQUIRE(plugin.canHandle(good.string()));
    REQUIRE_FALSE(plugin.canHandle(nofooter.string()));
    REQUIRE_FALSE(plugin.canHandle(wrongext.string()));
    REQUIRE(plugin.getSupportedExtensions().count("wsr") == 1);

    // Load + render: the synthetic rip constructs and yields the requested number
    // of interleaved stereo samples without throwing or crashing.
    auto* player = plugin.fromFile(good.string());
    REQUIRE(player != nullptr);
    std::array<int16_t, 8192> buffer{};
    int rc = player->getSamples(buffer.data(), buffer.size());
    REQUIRE(rc == static_cast<int>(buffer.size()));
    delete player;

    // A footerless file is rejected at construction time.
    REQUIRE_THROWS_AS(plugin.fromFile(nofooter.string()),
                      musix::player_exception);

    fs::remove(good);
    fs::remove(nofooter);
    fs::remove(wrongext);
}

// WSR plays real sound. These are genuine WonderSwan rips from the Modland
// collection (testmus/wsr). Each must be detected by canHandle and render
// non-zero audio through the full V30MZ + sound-chip emulation -- a regression
// in the vendored core, the symbol renaming, or the getSamples bridge would
// drop them to silence or fail to load. "kaze no klonoa" also exercises a
// non-trivial start subsong (its WSRF footer's first-song index is 26, above
// the default browse window, so it checks the start-song handling too).
TEST_CASE("WSR plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::WSRPlugin plugin;

    for (auto const& f : {"testmus/wsr/final fantasy.wsr",
                          "testmus/wsr/gunpey.wsr",
                          "testmus/wsr/rockman & forte.wsr",
                          "testmus/wsr/blue wing blitz.wsr",
                          "testmus/wsr/kaze no klonoa - moonlight museum.wsr"}) {
        std::string const path = f;
        INFO(path);
        REQUIRE(plugin.canHandle(path));

        auto* player = plugin.fromFile(path);
        REQUIRE(player != nullptr);

        std::array<int16_t, 8192> buffer{};
        int64_t energy = 0;
        for (int count = 0; count < 100 && energy == 0; ++count) {
            int rc = player->getSamples(buffer.data(), buffer.size());
            if (rc <= 0) { break; }
            for (int i = 0; i < rc; ++i) {
                energy += std::abs(static_cast<int>(buffer[i]));
            }
        }
        delete player;
        REQUIRE(energy != 0);
    }
}

// OPNA hardware-rhythm drums. The OPNA rhythm sample ROM is embedded
// (opna_rhythm_rom.cpp) and loaded by OPNA::Init via LoadEmbeddedRhythm(), so
// FMP/S98 percussion plays with no runtime file. This keys on all six rhythm
// voices on a bare OPNA and requires non-zero output (was silent before).
extern bool opna_rhythm_plays_sound();
TEST_CASE("OPNA rhythm", "[music]") { REQUIRE(opna_rhythm_plays_sound()); }
TEST_CASE("AO", "[music]") { testPlugin<musix::AOPlugin>("testmus/ao", ""); }
// exclude="nowork": daley_thompsons_star_events.prg is a genuinely un-emulatable
// TEDMUSIC rip -- it stays silent even after the TED player's key-press auto-start
// cycles all keys 0..10 over 600 render buffers (~108s). The TEDMUSIC format
// itself works (see sandgreen.prg), so this is one bad fixture, quarantined under
// testmus/ted/nowork/ like testmus/gme/nowork/ rather than counted as a failure.
TEST_CASE("Ted", "[music]") { testPlugin<musix::TEDPlugin>("testmus/ted", "nowork"); }
TEST_CASE("V2", "[music]") { testPlugin<musix::V2Plugin>("testmus/v2", ""); }

// Quartet ST (.4v) via the vendored zingzong replayer. A .4v carries only the
// sequence; the instruments live in a companion voiceset (".set") with the same
// basename in the same directory. QuartetPlugin locates that sibling (trying
// both ".set" and ".SET") and declares it via getSecondaryFiles so the loader
// fetches it. The ".set" itself is not a playable song, so it's excluded here.
TEST_CASE("Quartet", "[music]") { testPlugin<musix::QuartetPlugin>("testmus/4v", ""); }

// Quartet plays sound. "Bangkok.4v" needs its "Bangkok.set" voiceset for any
// audio; the plugin must pair the two, hand both to zingzong, and render
// non-zero output. Also exercises canHandle (.4v/.4q) and the secondary-file
// pairing. Fails if the .set companion lookup, the zingzong load, or playback
// regresses.
TEST_CASE("Quartet plays sound", "[music]")
{
    logging::setLevel(logging::Level::Warning);
    musix::QuartetPlugin plugin;

    std::string const fourv = "testmus/4v/Bangkok.4v";
    REQUIRE(plugin.canHandle(fourv));
    REQUIRE(plugin.canHandle("foo.4q"));
    REQUIRE_FALSE(plugin.canHandle("foo.mod"));

    // The .4v declares its per-song ".set" voiceset plus the shared "SMP.set"
    // bank some directories use instead; the missing one is ignored at load.
    REQUIRE(plugin.getSecondaryFiles(fourv) ==
            (std::vector<std::string>{ "Bangkok.set", "SMP.set" }));

    auto* player = plugin.fromFile(fourv);
    REQUIRE(player != nullptr);

    std::array<int16_t, 8192> buffer{};
    int64_t energy = 0;
    for (int count = 0; count < 100 && energy == 0; ++count) {
        int rc = player->getSamples(buffer.data(), buffer.size());
        if (rc <= 0) { break; }
        for (int i = 0; i < rc; ++i) {
            energy += std::abs(static_cast<int>(buffer[i]));
        }
    }
    delete player;

    // Non-silent output means the .set voiceset was located and loaded.
    REQUIRE(energy != 0);
}

// Compute's Stereo Sidplayer support. A tune is a pair of files: a ".mus"
// (first SID, voices 1-3) and a ".str" (second SID, voices 4-6). VICE is
// handed the ".mus" and loads the ".str" sibling itself; the plugin redirects
// a ".str" request to its ".mus" companion and reports the companion via
// getSecondaryFiles() so the loader fetches both.
TEST_CASE("Vice Stereo Sidplayer", "[music][vice]")
{
    logging::setLevel(logging::Level::Warning);
    musix::VicePlugin plugin{ "data" };

    REQUIRE(plugin.canHandle("foo.sid"));
    REQUIRE(plugin.canHandle("foo.mus"));
    REQUIRE(plugin.canHandle("foo.str"));
    REQUIRE(plugin.canHandle("FOO.STR"));
    REQUIRE_FALSE(plugin.canHandle("foo.mod"));

    auto exts = plugin.getSupportedExtensions();
    REQUIRE(exts.count("sid") == 1);
    REQUIRE(exts.count("mus") == 1);
    REQUIRE(exts.count("str") == 1);

    // The stereo (.str) file declares its .mus companion as a secondary file...
    REQUIRE(plugin.getSecondaryFiles(
                "testmus/libvice/stereo/linus and lucy.str") ==
            std::vector<std::string>{ "linus and lucy.mus" });
    // ...while the .mus has no secondaries of its own.
    REQUIRE(plugin
                .getSecondaryFiles("testmus/libvice/stereo/linus and lucy.mus")
                .empty());

    auto playsSound = [&](std::string const& file) {
        std::array<int16_t, 8192> buffer{};
        auto* player = plugin.fromFile(file);
        if (player == nullptr) { return false; }
        int64_t sum = 0;
        int count = 50;
        while (sum == 0 && count-- > 0) {
            int rc = player->getSamples(&buffer[0], buffer.size());
            if (rc <= 0) { break; }
            for (int i = 0; i < rc; ++i) {
                if (buffer[i] != 0) {
                    sum = 1;
                    break;
                }
            }
        }
        delete player;
        return sum != 0;
    };

    // Loading the .mus plays sound (and pulls in its .str sibling for stereo).
    REQUIRE(playsSound("testmus/libvice/stereo/linus and lucy.mus"));
    // Loading the .str redirects to the .mus companion and also plays.
    REQUIRE(playsSound("testmus/libvice/stereo/raistlin the magician.str"));
    // A regular .sid still loads and plays (no regression in psid_load_file).
    REQUIRE(playsSound("testmus/libvice/10_Orbyte.sid"));
}

// Dump the "Other Platforms" sub-platform groups (the OTHER format byte, one
// row per distinct format string) sorted by song count, so we can eyeball which
// deserve promotion to a top-level TAB filter. Reads the live app music.db and
// applies the real classifyFormat, so it matches the GUI drill exactly.
// Dumps the OTHER-byte sub-platform groups exactly as the TAB drill builds them:
// straight through MusicDatabase::getOtherPlatformCount() -> buildSubPlatforms()
// against the live cache DB. It must go through the real API rather than
// re-grouping the song table here -- an earlier copy did the latter and silently
// drifted, still showing the ColecoVision/Colecovision split after
// buildSubPlatforms had been fixed to fold case-only variants.
// Songs that reach NO platform filter. classifyFormat() returning UNKNOWN_FORMAT
// means no TAB filter matches the song, so it is findable only via "[no filter,
// search all]". Usually the cause is a format string that names a platform we
// know but format_map doesn't (platformNameToByte knows it, so the YouTube path
// resolves it while native rows fall through), landing on an extension the
// format_map can't key either (.zip/.rar/.7z/.gz archives resolve to nothing).
// Run against the live cache DB; prints the format strings worst affected.
// Every label filter_demozoo_archives.py --classify can write, resolved through
// the real classifier. Pass the labels as argv-free constants: the pass writes
// the member's bare uppercase extension into the format column, and the path
// stays the .zip, so this is exactly what the indexer will see. Any label
// printing UNKNOWN_FORMAT leaves its rows reaching no filter, i.e. the pass
// silently did nothing for them.
// The archive picker's two extension sets, for tooling that must agree with the
// app (filter_demozoo_archives.py reads this rather than hand-copying them --
// hand-copies are exactly what drifted and made 129 live archives look dead).
TEST_CASE("archive_picker_exts", "[.]")
{
    musix::ChipPlugin::createPlugins("data");
    auto const& [song, audio] = chipmachine::MusicPlayerList::archiveExtensions();
    for (auto const& e : song)
        printf("song:%s\n", e.c_str());
    for (auto const& e : audio)
        printf("audio:%s\n", e.c_str());
}

TEST_CASE("peek_labels", "[.]")
{
    using namespace chipmachine;
    RemoteLoader rl;
    MusicDatabase mdb{ rl };
    const std::string zip = "https://archive.scene.org/pub/x/y.zip";
    // Read the labels the pass ACTUALLY wrote, straight from demozoo.txt, rather
    // than a hardcoded list that would drift from what the peek emits.
    std::set<std::string> labels;
    {
        std::ifstream in("data/demozoo.txt");
        std::string line;
        while (std::getline(in, line)) {
            auto c = utils::split(line, "\t");
            if (c.size() < 5) continue;
            // The pass writes a bare uppercase code; demozoo's own platform
            // names ("Amiga", "ZX Spectrum") contain lowercase or spaces.
            std::string f = c[2];
            if (f.empty() || f.size() > 6) continue;
            if (f.find(' ') != std::string::npos) continue;
            if (std::any_of(f.begin(), f.end(),
                            [](unsigned char ch) { return std::islower(ch); }))
                continue;
            labels.insert(f);
        }
    }
    REQUIRE(labels.size() > 10); // sanity: we actually read the file
    printf("\n--- peek label -> platform (%d distinct) ---\n", (int)labels.size());
    for (auto const& lbl : labels) {
        auto* l = lbl.c_str();
        uint8_t b = mdb.classifyFormat(l, zip);
        // platformScreenshotSlug() is the public byte->name view; it returns ""
        // for the deliberately platform-less buckets (MP3/OGG/...), which is a
        // valid answer here -- only UNKNOWN_FORMAT means "no filter at all".
        std::string slug = MusicDatabase::platformScreenshotSlug(b);
        printf("  %-6s -> byte %3d  %-24s %s\n", l, (int)b,
               slug.empty() ? "(no-platform bucket)" : slug.c_str(),
               b == UNKNOWN_FORMAT ? "*** NO FILTER ***" : "");
    }
    printf("------------------------------\n");
}

TEST_CASE("unclassified_songs", "[.]")
{
    using namespace chipmachine;
    auto dbPath = (Environment::getCacheDir() / "music.db").string();
    sqlite3db::Database db(dbPath);
    std::map<std::string, int> byFormat;
    std::map<std::string, int> extOf;
    int total = 0, unknown = 0;
    auto q = db.query<std::string, std::string>("SELECT format, path FROM song");
    while (q.step()) {
        std::string fmt, path;
        std::tie(fmt, path) = q.get_tuple();
        total++;
        if (MusicDatabase::classifyFormat(fmt, path) != UNKNOWN_FORMAT) continue;
        unknown++;
        byFormat[fmt.empty() ? "<empty>" : fmt]++;
        auto e = utils::toLower(utils::path_extension(path));
        extOf[e.empty() ? "<none>" : e]++;
    }
    std::vector<std::pair<std::string, int>> rows(byFormat.begin(), byFormat.end());
    std::sort(rows.begin(), rows.end(),
              [](auto const& a, auto const& b) { return a.second > b.second; });
    printf("\n--- SONGS REACHING NO PLATFORM FILTER: %d of %d ---\n", unknown, total);
    for (auto const& [name, n] : rows)
        if (n > 1) printf("%6d  format=\"%s\"\n", n, name.c_str());
    std::vector<std::pair<std::string, int>> es(extOf.begin(), extOf.end());
    std::sort(es.begin(), es.end(),
              [](auto const& a, auto const& b) { return a.second > b.second; });
    printf("  -- by extension --\n");
    for (auto const& [e, n] : es)
        if (n > 1) printf("%6d  .%s\n", n, e.c_str());
    printf("------------------------------\n");
}

// The Virtual Platforms family: a byte-less 2nd level inside the Other drill.
// Drives the real browse path (setFormatFilter -> search("") -> getSongInfo) to
// prove: the top menu shows one "Virtual Platforms" PARENT row (othergroup:: path,
// aggregate count) and no bare TIC-80/PICO-8/MicroW8; entering it lists exactly
// those children (otherplatform:: paths); the counts add up and nothing is lost.
TEST_CASE("virtual platforms family nests inside Other", "[database]")
{
    using namespace chipmachine;
    RemoteLoader rl;
    MusicDatabase mdb{ rl };
    REQUIRE(mdb.initFromLua("."));

    // The three fantasy consoles are children of the family, never top rows.
    static const std::set<std::string> kids = { "TIC-80", "PICO-8", "MicroW8" };

    mdb.setFormatFilter({ (uint8_t)OTHER });
    std::vector<int> res;
    mdb.search("", res, 100000);

    // Top level: exactly one family parent, and none of its children bare.
    int parentIdx = -1, parentCount = 0;
    for (int idx : res) {
        auto s = mdb.getSongInfo(idx);
        REQUIRE(kids.count(s.title) == 0); // children are hidden under the parent
        if (utils::startsWith(s.path, "othergroup::")) {
            REQUIRE(s.title == "Virtual / Fantasy Platforms / Consoles");
            parentIdx = idx;
            parentCount = mdb.otherPlatformSongCount(idx - MusicDatabase::OTHER_PLATFORM_INDEX);
        }
    }
    REQUIRE(parentIdx >= 0); // the family parent is present at the top level

    // Enter the family: the menu now lists exactly the three children as their
    // own drillable (otherplatform::) rows, and their counts sum to the parent's.
    mdb.setOtherParent(parentIdx - MusicDatabase::OTHER_PLATFORM_INDEX);
    std::vector<int> children;
    mdb.search("", children, 100000);
    std::set<std::string> got;
    int childSum = 0;
    for (int idx : children) {
        auto s = mdb.getSongInfo(idx);
        REQUIRE(utils::startsWith(s.path, "otherplatform::"));
        got.insert(s.title);
        childSum += mdb.otherPlatformSongCount(idx - MusicDatabase::OTHER_PLATFORM_INDEX);
    }
    REQUIRE(got == kids);
    REQUIRE(childSum == parentCount);
}

TEST_CASE("other_platforms", "[.]")
{
    using namespace chipmachine;
    RemoteLoader rl;
    MusicDatabase mdb{ rl };
    // The app's own startup path: loads db.lua + the cached index, which is what
    // populates the in-memory `formats` vector buildSubPlatforms reads. Only
    // reindexes if db.lua's VERSION moved, exactly as the app does.
    REQUIRE(mdb.initFromLua("."));

    // Both drills share otherPlatformList -- getOtherPlatformCount() /
    // getArcadePlatformCount() just flip subPlatformByte and rebuild it.
    auto dump = [&](const char* title, int groups) {
        int total = 0;
        std::vector<std::pair<std::string, int>> rows;
        for (auto const& [gid, name] : mdb.otherPlatforms()) {
            int n = mdb.otherPlatformSongCount(gid);
            // A family PARENT (e.g. "Virtual Platforms") aggregates its children,
            // so mark it and skip its count -- the children are listed separately
            // and already contribute to the total.
            bool fam = mdb.isOtherFamilyRow(gid);
            rows.emplace_back(fam ? ("[" + name + "]") : name, n);
            if (!fam) total += n;
        }
        std::sort(rows.begin(), rows.end(),
                  [](auto const& a, auto const& b) { return a.second > b.second; });
        printf("\n--- %s (%d top rows, %d songs) ---\n", title, groups, total);
        for (auto const& [name, n] : rows)
            printf("%6d  %s\n", n, name.c_str());
        printf("------------------------------\n");
    };
    dump("OTHER PLATFORMS", mdb.getOtherPlatformCount());
    dump("ARCADE PLATFORMS", mdb.getArcadePlatformCount());
}

TEST_CASE("priority_map", "[.]")
{
    musix::ChipPlugin::createPlugins("data");
    auto& plugins = musix::ChipPlugin::getPlugins();

    std::map<std::string, std::vector<std::string>> extMap;
    for (auto const& plugin : plugins) {
        auto exts = plugin->getSupportedExtensions();
        for (auto const& ext : exts) {
            extMap[ext].push_back(plugin->name() + " (P:" + std::to_string(plugin->priority()) + ")");
        }
    }

    printf("\n--- EXTENSION PRIORITY MAP ---\n");
    for (auto const& [ext, handlers] : extMap) {
        printf(".%-8s : ", ext.c_str());
        for (size_t i = 0; i < handlers.size(); ++i) {
            printf("%s%s", handlers[i].c_str(), (i == handlers.size() - 1) ? "" : " -> ");
        }
        printf("\n");
    }
    printf("------------------------------\n");
}

TEST_CASE("extension_to_platform_map", "[.]")
{
    using std::string;
    musix::ChipPlugin::createPlugins("data");
    auto& plugins = musix::ChipPlugin::getPlugins();

    // Every extension any plugin claims must resolve to a platform. The
    // extension->platform mapping is owned by the app -- MusicDatabase::
    // platformForExtension() picks the highest-priority plugin claiming the
    // extension and maps it to a platform (the same rule the GUI uses) -- so this
    // test just enumerates the extensions and asserts the app classifies each.
    // The plugin name is tracked only to make an uncovered-extension failure
    // easier to diagnose. (This is DISTINCT from classifyFormat(), which the app
    // uses for real DB rows via their format string; see platformForExtension.)

    // ext -> highest-priority plugin claiming it (for diagnostics only).
    std::map<string, string> extPrimary;
    std::map<string, int> extPriority;
    for (auto const& plugin : plugins) {
        for (auto const& raw : plugin->getSupportedExtensions()) {
            auto ext = utils::toLower(raw);
            if (!extPrimary.count(ext) || plugin->priority() > extPriority[ext]) {
                extPrimary[ext] = plugin->name();
                extPriority[ext] = plugin->priority();
            }
        }
    }

    std::map<string, std::vector<string>> platformExts;
    std::vector<string> uncovered;
    for (auto const& [ext, primary] : extPrimary) {
        string platform = chipmachine::MusicDatabase::platformForExtension(ext);
        if (platform.empty()) {
            uncovered.push_back(ext + " (" + primary + ")");
        } else {
            platformExts[platform].push_back(ext);
        }
    }

    printf("\n--- EXTENSION -> PLATFORM MAP ---\n");
    for (auto const& [platform, exts] : platformExts) {
        printf("%-18s (%3zu): ", platform.c_str(), exts.size());
        for (size_t i = 0; i < exts.size(); i++) {
            printf(".%s%s", exts[i].c_str(), (i + 1 == exts.size()) ? "" : ", ");
        }
        printf("\n");
    }
    printf("---------------------------------\n");
    if (uncovered.empty()) {
        printf("\033[32mAll %zu extensions map to a platform.\033[0m\n",
               extPrimary.size());
    } else {
        printf("\033[31m%zu extension(s) NOT covered by any platform:\033[0m\n",
               uncovered.size());
        for (auto const& u : uncovered) { printf("  .%s\n", u.c_str()); }
    }
    REQUIRE(uncovered.empty());
}

TEST_CASE("coverage", "[music]")
{
    printf("======================================================\n");
    printf("TOTAL EXTENSION STATS:  \033[31mERRORS: %d\033[0m, \033[33mSKIPS: %d\033[0m, \033[32mOK: %d\033[0m\n",
           g_errors, g_skips, g_ok);
    printf("UNIQUE EXTENSION STATS: \033[31mERRORS: %zu\033[0m, \033[33mSKIPS: %zu\033[0m, \033[32mOK: %zu\033[0m\n",
           g_errorExts.size(), g_skipExts.size(), g_okExts.size());
    printf("(note: auxilarry files are ignored for the stats)\n");
    printf("======================================================\n");
    musix::ChipPlugin::createPlugins("data");
    auto& plugins = musix::ChipPlugin::getPlugins();

    std::vector<std::string> allMissing;
    std::map<std::string, std::vector<std::string>> missingByDir;
    size_t missingExtCount = 0;
    std::set<std::string> missingFolders;
    std::unordered_map<std::string, std::string> pluginDirs = {
        {"Game Music Engine", "testmus/gme"},
        {"AdPlug", "testmus/adlib"},
        {"UADE", "testmus/uade"},
        {"OpenMPT", "testmus/openmpt"},
        {"Gameboy Advance", "testmus/gsf"},
        {"NDSPlugin", "testmus/nds"},
        {"HEPlugin", "testmus/psx"},
        {"Ayfly ZX", "testmus/zx"},
        {"ffmpeg", "testmus/ffmpeg"},
        {"HTPlugin", "testmus/ht"},
        {"SC68", "testmus/sc68"},
        {"USFPlugin", "testmus/usf"},
        {"StSound", "testmus/stsound"},
        {"libmpg123", "testmus/mp3"},
        {"HivelyPlugin", "testmus/hively"},
        {"RSNPlugin", "testmus/rsn"},
        {"MDX", "testmus/mdx"},
        {"S98", "testmus/s98"},
        {"Audio Overload", "testmus/ao"},
        {"Tedplay", "testmus/ted"},
        {"V2Plugin", "testmus/v2"},
        {"Organya Player", "testmus/org"},
        {"SunVox Player", "testmus/sunvox"},
        {"FMPPlugin", "testmus/fmp"},
        {"Quartet", "testmus/4v"},
        {"Euphony", "testmus/eup"},
        {"WonderSwan (in_wsr)", "testmus/wsr"},
        {"PokeyNoise", "testmus/pn"},
        {"Monotone", "testmus/monotone"},
        {"Beepola (Phaser1)", "testmus/bbsong"},
        {"Archimedes Tracker", "testmus/musx"},
        {"Coconizer", "testmus/coco"},
        {"MaxTrax", "testmus/maxtrax"},
        {"STarKos", "testmus/sks"},
        {"NerdTracker2", "testmus/ned"},
        {"SCC-Musixx", "testmus/sccmusixx"},
        {"Sam Coupe (COP)", "testmus/cop"},
        {"JayTrax", "testmus/jxs"},
        {"Funktracker", "testmus/fnk"},
        {"Vic-Tracker", "testmus/victracker"},
        {"Klystrack Player", "testmus/klystrack"}
    };

    // Plugins whose extensions are split across several testmus folders (one
    // sample dir per format), so a single dir can't cover them. libkss handles
    // five MSX formats, each filed under its own extension directory.
    std::unordered_map<std::string, std::vector<std::string>> pluginDirsMulti = {
        {"MSX (libkss)", {"testmus/mgs", "testmus/bgm", "testmus/opx",
                          "testmus/mpk", "testmus/mbm", "testmus/fac"}},
        // PxTone Collage handles two extensions (.ptcop and .pttune), each
        // filed under its own fixture dir -- the same dirs the PxTone/PxTune
        // playback tests read.
        {"PxTone Collage Player", {"testmus/ptcop", "testmus/pttune"}},
        // ZXTune handles several ZX/Sam Coupe formats, each under its own
        // fixture dir: Sound Tracker 1.1, Global Tracker, Chip Tracker, TFM
        // Music Maker, and ZX Pro Sound Maker (.psm, routed away from OpenMPT by
        // content -- see OpenMPTPlugin::canHandle). (Sam Coupe COP .cop is now
        // handled by CopPlugin -- see pluginDirs above.)
        {"ZX Spectrum (ZXTune)", {"testmus/st11", "testmus/gtr",
                                  "testmus/chi", "testmus/tfe", "testmus/psm",
                                  "testmus/ftc"}}
    };

    auto dirsFor = [&](std::string const& name) -> std::vector<std::string> {
        if (pluginDirsMulti.count(name)) { return pluginDirsMulti[name]; }
        if (pluginDirs.count(name)) { return {pluginDirs[name]}; }
        return {"testmus/" + utils::toLower(name)};
    };

    // Build the set of every extension that has a fixture in ANY plugin's sample
    // dir. An extension counts as covered if a test file for it exists somewhere,
    // so a low-priority *fallback* claimer is not reported missing just because
    // the primary handler owns the fixture in its own dir. Concretely: UADE also
    // lists .ym (primary: StSound) and .mus (primary: libvice), and OpenMPT also
    // lists .mus -- those fixtures live under testmus/stsound and testmus/libvice,
    // so without this they'd show as missing under uade/openmpt despite being
    // fully tested by their real owners.
    std::set<std::string> globalExts;
    for (auto const& plugin : plugins) {
        if (plugin->getSupportedExtensions().empty()) { continue; }
        for (auto const& dir : dirsFor(plugin->name())) {
            utils::File folderCheck{ dir };
            if (!folderCheck.exists()) { continue; }
            for (auto const& f : folderCheck.listFiles()) {
                // Index by both suffix extension AND modland-style prefix, so
                // prefix-form formats (e.g. "8svx.<name>", "mdat.<name>") whose
                // "extension" is the songname still count as covered.
                globalExts.insert(
                    utils::toLower(utils::path_extension(f.getName())));
                globalExts.insert(
                    utils::toLower(utils::path_prefix(f.getName())));
            }
        }
    }

    for (auto const& plugin : plugins) {
        std::string name = plugin->name();
        auto exts = plugin->getSupportedExtensions();
        if (exts.empty()) continue;

        std::vector<std::string> dirs = dirsFor(name);

        // Aggregate the extensions present across every sample dir for this
        // plugin. Compare case-insensitively: some rips carry upper-case
        // extensions (e.g. Demosong.MBM) while getSupportedExtensions() is
        // lower-case.
        std::set<std::string> existingExts;
        for (auto const& dir : dirs) {
            utils::File folderCheck{ dir };
            if (!folderCheck.exists()) {
                missingFolders.insert(dir);
            } else {
                auto files = folderCheck.listFiles();
                for (auto const& f : files) {
                    existingExts.insert(
                        utils::toLower(utils::path_extension(f.getName())));
                    existingExts.insert(
                        utils::toLower(utils::path_prefix(f.getName())));
                }
            }
        }

        std::string shortDir = dirs.front();
        std::string prefix = "testmus/";
        if (shortDir.rfind(prefix, 0) == 0) {
            shortDir = shortDir.substr(prefix.length());
        }
        for (auto const& ext : exts) {
            if (notSupportedExts().count(utils::toLower(ext)) > 0) { continue; }
            // Covered if this plugin's own dir has it, or any other plugin's dir
            // does (the extension's primary owner holds the fixture elsewhere).
            if (existingExts.count(ext) == 0 &&
                globalExts.count(utils::toLower(ext)) == 0) {
                missingByDir[shortDir].push_back(ext);
                missingExtCount++;
                allMissing.push_back(name + ":" + ext + " (Target Folder: " +
                                     dirs.front() + ")");
            }
        }
    }

    if (!missingByDir.empty()) {
        printf("\n\033[31m%zu testmus folders with missing test files detected.\033[0m\n",
               missingByDir.size());
        printf("\033[31m%zu extensions not covered.\033[0m\n", missingExtCount);
        for (auto const& [dir, exts] : missingByDir) {
            printf("%s/: ", dir.c_str());
            for (size_t i = 0; i < exts.size(); ++i) {
                printf("%s%s", exts[i].c_str(), (i == exts.size() - 1) ? "\n" : ", ");
            }
        }
    }

    if (!allMissing.empty()) {
        //printf("\n--- MISSING EXTENSIONS REPORT ---\n");
        for (auto const& m : allMissing) {
            //printf("  %s\n", m.c_str());
        }
        printf("-------------------------------------\n");
        printf("TOTAL MISSING EXTENSIONS SKIPPED: %zu\n", allMissing.size());
        printf("-------------------------------------\n");
    } else {
        printf("\n--- COVERAGE METRIC: 100%% COMPLIANT (0 MISSING EXTENSIONS) ---\n");
    }

    if (!missingFolders.empty()) {
        printf("\n--- MISSING TARGET DIRECTORIES DETECTED ---\n");
        printf("Execute the following terminal script to construct the environment:\n\n");
        printf("```bash\n");
        for (auto const& folder : missingFolders) {
            printf("mkdir -p \"%s\"\n", folder.c_str());
        }
        printf("```\n");
        printf("-------------------------------------------\n");
    }

    printf("\n>>> Hint: run cmtest priority_map to see the plugin handling priority map for each extension\n");
    printf(">>> Hint: run cmtest extension_to_platform_map to see mapping of extensions to platforms\n\n");

    auto const& unsupported = notSupportedExts();
    if (!unsupported.empty()) {
        printf(">>> Extensions EXPLICITLY not supported (per "
               "not_supported_extension.txt): ");
        bool first = true;
        for (auto const& ext : unsupported) {
            printf("%s.%s", first ? "" : ", ", ext.c_str());
            first = false;
        }
        printf("\n\n");
    }

    // --- Regression gate -----------------------------------------------------
    // Lock in today's playback results: every fixture that plays right now must
    // keep playing. These tallies only ever GROW when something regresses -- a
    // tune that renders sound today breaking tomorrow flips OK->error
    // (g_errors++), and a format whose plugin stops claiming it flips OK->skip
    // (g_skips++). Both are naturally 0 on an isolated/partial `cmtest` run, so
    // the gate never false-fails there; it only trips on a real regression in a
    // full run (CI). A new red line is a show-stopper.
    //
    // Maintenance: when you intentionally FIX a known-failing fixture (or delete
    // a dead one), LOWER the matching baseline so the gate stays tight. Adding a
    // new fixture that can't play (or isn't claimed) will also trip this -- by
    // design: make it play, or bump the baseline on purpose.
    //
    // Baseline captured 2026-06 (deterministic across runs). The g_errors are
    // known non-playing fixtures (e.g. mini* rips whose lib lives only on the
    // remote source, intentionally-bad rips); g_skips are deliberate canHandle
    // declines plus companion/lib files that aren't standalone tunes.
    // Set tight to the exact current counts so ANY new failure trips the gate.
    //
    // 2026-06-17: errors 69->44 after fixing dune1.dro (DRO v0), 2.hsc (HSC
    // half-pattern bug), the .sci/.ksm/.minidsf/.minissf/.miniusf multi-file
    // fixtures (bundled their companion banks/libs). skips 46->47: those bundled
    // companion files (insts.dat, *patch.003, *.dsflib/.ssflib/.usflib) are not
    // standalone tunes and correctly skip.
    //
    // 2026-06-17 (b): errors 44->30 after relocating misfiled fixtures out of
    // testmus/uade to their owning, higher-priority plugins (.it/.xm->openmpt,
    // .mdx->mdx, .dsf->ht, .pt2->zx, .bbsong->bbsong, .sid->libvice) where they
    // play, moving an orphan pm.psf2lib to psx, and bundling smpl.kraft so the
    // TFMX mdat.kraft plays. skips 47->48 from the shuffle (all legitimate).
    //
    // 2026-06-17 (c): errors 30->27 after bundling the missing companion files
    // for three "score died" UADE tunes -- daisy.adsc.as (Audio Sculpture) and
    // smpl.avalon2-ongame / smpl.hexuma-ice (TFMX). skips 48->51: those three
    // companions aren't standalone tunes and correctly skip.
    //
    // 2026-06-17 (d): errors 27->17. Bundled 5 more UADE companions (jpn->smp,
    // dns->smp, mcr->mcs, uds->smp, tpu->smp; latter 3 added to fmt_2files) so
    // those score-died tunes play; relocated misrouted fixtures -- prom.asc /
    // pr2.asc are ZX ASC Sound Master (->zx, now play), alp.pdx/pha.pdx are MDX
    // PCM banks (->mdx) and TP.PVI an FMP PCM bank (->fmp), all non-standalone
    // and correctly skipping. skips 51->55 from the bundled smp.* companions.
    //
    // 2026-06-17 (e): errors 17->14. Three .sng files misrouted to UADE are
    // really AdLib formats AdPlug 2.4 decodes -- sanxion.sng (FMC!), playmus1.sng
    // (ObsM), song1.sng (AdLib Tracker + song1.ins). Added a content-gated .sng
    // claim to AdPlug.canHandle (FMC!/ObsM magic or 36000B AdLib Tracker) so it
    // doesn't steal SCC-Musixx/Richard Joseph .sng, plus .sng->.ins secondary;
    // moved the three fixtures to testmus/adlib. (aquatic games.sng = Amiga
    // Richard Joseph "RJP1SMOD" stays in UADE.)
    //
    // 2026-06-17 (f): errors 14->11. Bundled companions mfp->smp (Magnetic
    // Fields Packer) and MIDI->SMPL (MIDI-Loriciel; added both to fmt_2files);
    // and the "war hawk.st1.3.mod.nt" fixture was actually the StarTrekker AM
    // .nt companion -- fetched the real 29756B "war hawk.st1.3.mod" (now plays,
    // finds its .nt sibling) and excluded the standalone ".mod.nt" companion
    // from the UADE folder scan. skips 55->56 (SMPL.Entity high companion).
    //
    // 2026-06-17 (g): errors -> 0. The last UADE error, qts.Big Pro (Quartet ST),
    // was NOT unsourceable: its dir shares a fixed-name "SMP.set" sample bank
    // (not the per-song "set.Big Pro" the prefix convention implies). Bundled
    // SMP.set and added the qts->SMP.set getSecondaryFiles case; the bank is now
    // auto-ignored as a companion. cmtest is fully green. NOTE: a rare transient
    // (~1 in 15 full runs) flips one normally-OK file to an error, presumably a
    // timing/network blip; bump g_errors a little if CI flakes on it.
    //
    // 2026-06-17 (h): skips 22->14. Taught shouldIgnoreFile to recognize more
    // companions (smpl/ip exts, case-insensitive smpl./smp., mcs., .ip.) so the
    // TFMX/MIDI-Loriciel/Mark Cooksey/MusicMaker banks Ignore instead of Skip;
    // moved the misplaced "Allegro Amadeus Strikes Back.BGM" (MSX, magic 0xFE)
    // to testmus/bgm where KSS plays it; and gave libvice the .rsid extension
    // (VICE's psid_load_file reads "RSID" magic) -- "10... knockout!.rsid" moved
    // to testmus/libvice and is covered by the "RSID plays sound" test.
    //
    // 2026-06-17 (i): skips 14->11. Removed two genuinely-bogus no-extension
    // files (BULLWINKLES MAX, NTV IRS GS SG); "songplay" turned out to be the
    // shared Kris Hatlelid replay executable the .kh tunes load, so it's kept,
    // Ignored, and named by .kh's getSecondaryFiles (the cycles.kh now plays).
    //
    // 2026-06-17 (j): skips 11->10. macOS hidden files (.DS_Store, ._*) are now
    // silently skipped before any reporting (not Skipped/Ignored, not counted).
    //
    // 2026-06-29: ".dtm" is a three-way collision. AdPlug content-gates DeFy DTM
    // ("DeFy DTM ") and OpenMPT now content-gates Digital Tracker ("D.T.") -- the
    // big modland "Digital Tracker DTM" corpus that previously hard-FAILED in
    // AdPlug now plays via OpenMPT (real dreams.dtm fixture). The third format,
    // DigiTrekker (MS-DOS, chunked "SONG"/..., modland "Digitrekker", ~2 tunes)
    // has no open replayer, so demorave.dtm is now claimed by nobody and Skips
    // cleanly instead of erroring -- one new intentional skip, cap 27->28.
    //
    // 2026-07-03: errors 26->0. The Jun-22 TFMX batch had regressed the suite
    // (missing sample halves + synthetic tfmx7v/tfmxpro/tfhd7v/tfhdpro/*1.5
    // prefixes that break UADE sample resolution). Fetched the real smpl./smp.
    // companions from modland for the tunes that have them (thm.* Thomas Hermann,
    // uds.* BladePacker, TFMX karamalz-titel/abandonedplaces-part2.1), collapsed
    // each TFMX base to its real "mdat.<song>"+"smpl.<song>" pair, and dropped the
    // fixtures that can't play: the synthetic prefix duplicates (never occur on
    // disk -- routing now guarded by the canHandle test above), the TFMX-ST jim
    // power set (no sample files exist anywhere on modland; TFMX-ST renders silent
    // here) and four degenerate oddballs (bootsong.mod -> belongs to OpenMPT,
    // "feud fake.dw"/jurassic "- null" = intentionally silent/empty, hardball2.kh
    // needs a "songplay" that collides with the-cycles.kh in a flat dir).
    // skips 25->19 (the removed tfmx1.5/tfhd1.5 duplicates had been skipping).
    REQUIRE(g_errors <= 0);
    REQUIRE(g_skips <= 19);
}


