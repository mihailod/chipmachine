#include "catch.hpp"

#include "src/MusicDatabase.h"
#include "src/MusicPlayer.h"
#include "src/MusicPlayerList.h"
#include "src/RemoteLoader.h"
#include "src/modutils.h"

#include "src/di.hpp"
namespace di = boost::di;

#include <audioplayer/audioplayer.h>
#include <coreutils/log.h>
#include <musicplayer/src/chipplugin.h>
#include <musicplayer/src/plugins/plugins.h>
#include <musicplayer/src/plugins/uadeplugin/UADEPlugin.h>
#include <musicplayer/src/plugins/ptkplugin/PTKPlugin.h>
#include <musicplayer/src/plugins/openmptplugin/OpenMPTPlugin.h>
#include <musicplayer/src/plugins/quartetplugin/QuartetPlugin.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <string>
#include <thread>
#include <unordered_map>
#include <set>
namespace fs = std::filesystem;

// Running tallies across all playback (testPlugin) runs, summarized in "coverage".
static int g_errors = 0; // red lines: FAILED / NO SOUND / EXCEPTION
static int g_skips = 0;   // gray lines: Skipping (plugin can't handle)
static int g_ok = 0;     // playback OK

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
    mpl->addSong("music/Amiga/Starbuck - Tennis.mod"s);
    mpl->addSong("music/Amiga/Dr.Awesome - Intromusic3.mod"s);
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
    bool ok = mp.playFile("music/Amiga/Nuke - Loader.mod");
    REQUIRE(ok);
    mp.update();
    std::vector<int16_t> data(8192);
    ap->get(data);
    auto sum = std::accumulate(data.begin(), data.end(), (int64_t)0);
    REQUIRE(sum != 0);
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

            // silently ignore extensions flagged impossible-to-support
            if (notSupportedExts().count(
                    utils::toLower(utils::path_extension(f.getName()))) > 0)
                continue;

            int64_t sum = 0;
            if (!plugin.canHandle(f.getName())) {
                printf("\033[90mSkipping %s\033[0m\n", f.getName().c_str());
                g_skips++;
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
                } else {
                    printf("playback %s\n", status);
                    g_ok++;
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
                    printf("\r\033[90mSkipping %s (%s)\033[0m\n",
                           f.getName().c_str(), e.what());
                    g_skips++;
                } else {
                    printf("\r\033[31mTrying %s ... playback EXCEPTION (%s)\033[0m\n",
                           f.getName().c_str(), e.what());
                    g_errors++;
                }
            }
        }
    } catch (std::exception& e) {
        printf("---- Plugin Instantiation Failed: %s ----\n", e.what());
    }
    return true;
}

TEST_CASE("GME", "[music]") { testPlugin<musix::GMEPlugin>("testmus/gme", "nowork"); }

// Regression test for SGC (Sega Master System / Game Gear / ColecoVision)
// support. The vendored Game_Music_Emu had the SGC emulator stripped out (the
// USE_GME_SGC scaffolding was left behind but the Sgc_* sources were missing).
// It was added back -- Sgc_Emu/Impl/Core/Cpu plus the Z80_Cpu core, Gme_Loader
// and Sms_Fm_Apu it depends on -- and "sgc" registered in GMEPlugin. This plays
// a real .sgc PSG tune and checks for audio. (The YM2413/OPLL FM chip is a stub
// in this copy, so SMS FM is reported unsupported; Game Gear tunes like this one
// are PSG-only and play fully.) Fails if SGC routing or the emulator regresses.
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
TEST_CASE("UADE", "[music]") { testPlugin<musix::UADEPlugin>("testmus/uade", ".smpl,smp.", "data"); }
TEST_CASE("PxTone", "[music]") { testPlugin<musix::PxTonePlugin>("testmus/ptcop", ""); }
TEST_CASE("PxTune", "[music]") { testPlugin<musix::PxTonePlugin>("testmus/pttune", ""); }
TEST_CASE("PTK", "[music]") { testPlugin<musix::PTKPlugin>("testmus/ptk", ""); }
TEST_CASE("NTK", "[music]") { testPlugin<musix::PTKPlugin>("testmus/ntk", ""); }
TEST_CASE("Org", "[music]") { testPlugin<musix::OrgPlugin>("testmus/org", ""); }
TEST_CASE("SunVox", "[music]") { testPlugin<musix::SunVoxPlugin>("testmus/sunvox", ""); }
// Exclude the ".W" wavebank from the scan -- it's the song's companion, not a
// playable fixture (canHandle rightly declines it); fromFile() picks it up next
// to the bare song.
TEST_CASE("SoundSmith", "[music]") { testPlugin<musix::SoundSmithPlugin>("testmus/soundsmith", ".W"); }
TEST_CASE("Musx", "[music]") { testPlugin<musix::MusxPlugin>("testmus/musx", ""); }
TEST_CASE("Coconizer", "[music]") { testPlugin<musix::CocoPlugin>("testmus/coco", ""); }
TEST_CASE("MaxTrax", "[music]") { testPlugin<musix::MaxTraxPlugin>("testmus/maxtrax", ""); }
TEST_CASE("STarKos", "[music]") { testPlugin<musix::SksPlugin>("testmus/sks", ""); }
TEST_CASE("NerdTracker2", "[music]") { testPlugin<musix::NEDPlugin>("testmus/ned", ""); }
TEST_CASE("PlayerPRO", "[music]") { testPlugin<musix::PlayerProPlugin>("testmus/playerpro", ""); }
TEST_CASE("JayTrax", "[music]") { testPlugin<musix::JxsPlugin>("testmus/jxs", ""); }
TEST_CASE("IXS", "[music]") { testPlugin<musix::IXSPlugin>("testmus/ixs", ""); }

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

TEST_CASE("OpenMPT", "[music]") { testPlugin<musix::OpenMPTPlugin>("testmus/openmpt", ""); }
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
    testPlugin<musix::ZXTunePlugin>("testmus/cop", "");  // Sam Coupe COP (SAA1099)
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
TEST_CASE("Quartet", "[music]") { testPlugin<musix::QuartetPlugin>("testmus/4v", ".set"); }

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

    // The .4v declares its .set voiceset companion as a secondary file.
    REQUIRE(plugin.getSecondaryFiles(fourv) ==
            std::vector<std::string>{ "Bangkok.set" });

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

TEST_CASE("coverage", "[music]")
{
    printf("===========================================\n");
    printf("\033[31mERRORS: %d\033[0m, \033[90mSKIPS: %d\033[0m, \033[32mOK: %d\033[0m\n",
           g_errors, g_skips, g_ok);
    printf("===========================================\n");
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
        {"JayTrax", "testmus/jxs"}
    };

    // Plugins whose extensions are split across several testmus folders (one
    // sample dir per format), so a single dir can't cover them. libkss handles
    // five MSX formats, each filed under its own extension directory.
    std::unordered_map<std::string, std::vector<std::string>> pluginDirsMulti = {
        {"MSX (libkss)", {"testmus/mgs", "testmus/bgm", "testmus/opx",
                          "testmus/mpk", "testmus/mbm"}},
        // PxTone Collage handles two extensions (.ptcop and .pttune), each
        // filed under its own fixture dir -- the same dirs the PxTone/PxTune
        // playback tests read.
        {"PxTone Collage Player", {"testmus/ptcop", "testmus/pttune"}},
        // ZXTune handles several ZX/Sam Coupe formats, each under its own
        // fixture dir: Sound Tracker 1.1, Sam Coupe COP, Global Tracker, Chip
        // Tracker, TFM Music Maker, and ZX Pro Sound Maker (.psm, routed away
        // from OpenMPT by content -- see OpenMPTPlugin::canHandle).
        {"ZX Spectrum (ZXTune)", {"testmus/st11", "testmus/cop", "testmus/gtr",
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
                globalExts.insert(
                    utils::toLower(utils::path_extension(f.getName())));
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
        printf("--------------------------------------\n");
        printf("TOTAL MISSING EXTENSIONS SKIPPED: %zu\n", allMissing.size());
        printf("--------------------------------------\n");
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

    printf("\n>>> Hint: run cmtest priority_map to see the plugin handling priority map for each extension\n\n");

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
    REQUIRE(g_errors <= 27);
    REQUIRE(g_skips <= 51);
}
