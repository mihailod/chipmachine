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
#include <musicplayer/src/plugins/openmptplugin/OpenMPTPlugin.h>
#include <musicplayer/src/plugins/quartetplugin/QuartetPlugin.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <string>
#include <unordered_map>
#include <set>
namespace fs = std::filesystem;

// Running tallies across all playback (testPlugin) runs, summarized in "coverage".
static int g_errors = 0; // red lines: FAILED / NO SOUND / EXCEPTION
static int g_skips = 0;   // gray lines: Skipping (plugin can't handle)
static int g_ok = 0;     // playback OK

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
            if (exclude != "" && f.getName().find(exclude) != std::string::npos)
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
                printf("\r\033[31mTrying %s ... playback EXCEPTION (%s)\033[0m\n",
                       f.getName().c_str(), e.what());
                g_errors++;
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
TEST_CASE("AdPlug", "[music]") { testPlugin<musix::AdPlugin>("testmus/adlib", ".rol", "data"); }

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
TEST_CASE("UADE", "[music]") { testPlugin<musix::UADEPlugin>("testmus/uade", "smp", "data"); }
TEST_CASE("PxTone", "[music]") { testPlugin<musix::PxTonePlugin>("testmus/ptcop", ""); }
TEST_CASE("PxTune", "[music]") { testPlugin<musix::PxTonePlugin>("testmus/pttune", ""); }
TEST_CASE("Org", "[music]") { testPlugin<musix::OrgPlugin>("testmus/org", ""); }
TEST_CASE("SunVox", "[music]") { testPlugin<musix::SunVoxPlugin>("testmus/sunvox", ""); }

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

TEST_CASE("OpenMPT", "[music]") { testPlugin<musix::OpenMPTPlugin>("testmus/openmpt", ""); }
TEST_CASE("GSF", "[music]") { testPlugin<musix::GSFPlugin>("testmus/gsf", "lib"); }
TEST_CASE("NDS", "[music]") { testPlugin<musix::NDSPlugin>("testmus/nds", "lib"); }
TEST_CASE("HE", "[music]") { testPlugin<musix::HEPlugin>("testmus/psx", "lib", "data/hebios.bin"); }
TEST_CASE("Ayfly", "[music]") { testPlugin<musix::AyflyPlugin>("testmus/zx", ".vt2"); }
TEST_CASE("FFMPEG", "[music]") { testPlugin<musix::FFMPEGPlugin>("testmus/ffmpeg", ""); }
TEST_CASE("HT", "[music]") { testPlugin<musix::HTPlugin>("testmus/ht", ""); }
TEST_CASE("SC68", "[music]") { testPlugin<musix::SC68Plugin>("testmus/sc68", "", "data"); }
TEST_CASE("USF", "[music]") { testPlugin<musix::USFPlugin>("testmus/usf", ""); }
TEST_CASE("StSound", "[music]") { testPlugin<musix::StSoundPlugin>("testmus/stsound", ""); }
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
TEST_CASE("Ted", "[music]") { testPlugin<musix::TEDPlugin>("testmus/ted", ""); }
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
        {"PxTone Collage Player", "testmus/pxtone"},
        {"Organya Player", "testmus/org"},
        {"SunVox Player", "testmus/sunvox"},
        {"FMPPlugin", "testmus/fmp"},
        {"Quartet", "testmus/4v"},
        {"Euphony", "testmus/eup"},
        {"WonderSwan (in_wsr)", "testmus/wsr"}
    };

    // Plugins whose extensions are split across several testmus folders (one
    // sample dir per format), so a single dir can't cover them. libkss handles
    // five MSX formats, each filed under its own extension directory.
    std::unordered_map<std::string, std::vector<std::string>> pluginDirsMulti = {
        {"MSX (libkss)", {"testmus/mgs", "testmus/bgm", "testmus/opx",
                          "testmus/mpk", "testmus/mbm"}}
    };

    for (auto const& plugin : plugins) {
        std::string name = plugin->name();
        auto exts = plugin->getSupportedExtensions();
        if (exts.empty()) continue;

        std::vector<std::string> dirs;
        if (pluginDirsMulti.count(name)) {
            dirs = pluginDirsMulti[name];
        } else if (pluginDirs.count(name)) {
            dirs = {pluginDirs[name]};
        } else {
            dirs = {"testmus/" + utils::toLower(name)};
        }

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
            if (existingExts.count(ext) == 0) {
                missingByDir[shortDir].push_back(ext);
                missingExtCount++;
                allMissing.push_back(name + ":" + ext + " (Target Folder: " +
                                     dirs.front() + ")");
            }
        }
    }

    if (!missingByDir.empty()) {
        printf("\n\033[31m%zu missing folders with test files in testmus detected.\033[0m\n",
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
}
