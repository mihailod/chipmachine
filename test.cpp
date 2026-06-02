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
                printf("Skipping %s\n", f.getName().c_str());
                continue;
            }
            printf("Trying %s\n", f.getName().c_str());
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
                printf("#### Playing %s : %s\n", f.getName().c_str(),
                       player ? (sum == 0 ? "NO SOUND" : "OK") : "FAILED");
            } catch (std::exception& e) {
                printf("#### Playing %s : EXCEPTION (%s)\n", f.getName().c_str(), e.what());
            }
        }
    } catch (std::exception& e) {
        printf("---- Plugin Instantiation Failed: %s ----\n", e.what());
    }
    return true;
}

TEST_CASE("GME", "[music]") { testPlugin<musix::GMEPlugin>("testmus/gme/working", ""); }
TEST_CASE("AdPlug", "[music]") { testPlugin<musix::AdPlugin>("testmus/adlib", ".rol", "data"); }
TEST_CASE("UADE", "[music]") { testPlugin<musix::UADEPlugin>("testmus/uade", "smp", "data"); }

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
TEST_CASE("AO", "[music]") { testPlugin<musix::AOPlugin>("testmus/ao", ""); }
TEST_CASE("Ted", "[music]") { testPlugin<musix::TEDPlugin>("testmus/ted", ""); }
TEST_CASE("V2", "[music]") { testPlugin<musix::V2Plugin>("testmus/v2", ""); }

TEST_CASE("priority_map", "")
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
    musix::ChipPlugin::createPlugins("data");
    auto& plugins = musix::ChipPlugin::getPlugins();
    
    std::vector<std::string> allMissing;
    std::set<std::string> missingFolders;
    std::unordered_map<std::string, std::string> pluginDirs = {
        {"Game Music Engine", "testmus/gme/working"},
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
        {"V2Plugin", "testmus/v2"}
    };

    for (auto const& plugin : plugins) {
        std::string name = plugin->name();
        auto exts = plugin->getSupportedExtensions();
        if (exts.empty()) continue;

        std::string dir = "";
        if (pluginDirs.count(name)) {
            dir = pluginDirs[name];
        } else {
            dir = "testmus/" + utils::toLower(name);
        }

        utils::File folderCheck{ dir };
        std::set<std::string> existingExts;

        if (!folderCheck.exists()) {
            missingFolders.insert(dir);
        } else {
            auto files = folderCheck.listFiles();
            for (auto const& f : files) {
                existingExts.insert(utils::path_extension(f.getName()));
            }
        }

        for (auto const& ext : exts) {
            if (existingExts.count(ext) == 0) {
                printf(".%s testing skipped, add file to folder %s\n", ext.c_str(), dir.c_str());
                allMissing.push_back(name + ":" + ext + " (Target Folder: " + dir + ")");
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
