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

#include <algorithm>
#include <array>
#include <numeric>
#include <string>
#include <unordered_map>
#include <set>

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
    AudioPlayerNull ap{};
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
    ap.seek(150);
    mpl->wait();
    info = mpl->getInfo();
    //LOGI("%s %s %d", info.title, info.path, state);
}

TEST_CASE("musicplayer", "")
{
    AudioPlayerNull ap{};
    const auto injector = di::make_injector(di::bind<utils::path>.to("."),
                                            di::bind<AudioPlayer>.to(ap));
    musix::ChipPlugin::createPlugins("data");
    chipmachine::MusicPlayer mp{ ap };
    bool ok = mp.playFile("music/Amiga/Nuke - Loader.mod");
    REQUIRE(ok);
    mp.update();
    std::vector<int16_t> data(8192);
    ap.get(data);
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

TEST_CASE("gme", "[music]") { testPlugin<musix::GMEPlugin>("testmus/gme/working", ""); }
TEST_CASE("adlib", "[music]") { testPlugin<musix::AdPlugin>("testmus/adlib", ".rol", "data"); }
TEST_CASE("uade", "[music]") { testPlugin<musix::UADEPlugin>("testmus/uade", "smp", "data"); }
TEST_CASE("openmpt", "[music]") { testPlugin<musix::OpenMPTPlugin>("testmus/openmpt", ""); }
TEST_CASE("gsf", "[music]") { testPlugin<musix::GSFPlugin>("testmus/gsf", "lib"); }
TEST_CASE("nds", "[music]") { testPlugin<musix::NDSPlugin>("testmus/nds", "lib"); }
TEST_CASE("psx", "[music]") { testPlugin<musix::HEPlugin>("testmus/psx", "lib", "data/hebios.bin"); }
TEST_CASE("zx", "[music]") { testPlugin<musix::AyflyPlugin>("testmus/zx", ".vt2"); }
TEST_CASE("ffmpeg", "[music]") { testPlugin<musix::FFMPEGPlugin>("testmus/ffmpeg", ""); }
TEST_CASE("ht", "[music]") { testPlugin<musix::HTPlugin>("testmus/ht", ""); }
TEST_CASE("sc68", "[music]") { testPlugin<musix::SC68Plugin>("testmus/sc68", "", "data"); }
TEST_CASE("usf", "[music]") { testPlugin<musix::USFPlugin>("testmus/usf", ""); }
TEST_CASE("stsound", "[music]") { testPlugin<musix::StSoundPlugin>("testmus/stsound", ""); }
TEST_CASE("mp3", "[music]") { testPlugin<musix::MP3Plugin>("testmus/mp3", ""); }
TEST_CASE("hively", "[music]") { testPlugin<musix::HivelyPlugin>("testmus/hively", ""); }
TEST_CASE("rsn", "[music]") { testPlugin<musix::RSNPlugin>("testmus/rsn", ""); }
TEST_CASE("mdx", "[music]") { testPlugin<musix::MDXPlugin>("testmus/mdx", ""); }
TEST_CASE("s98", "[music]") { testPlugin<musix::S98Plugin>("testmus/s98", ""); }
TEST_CASE("ao", "[music]") { testPlugin<musix::AOPlugin>("testmus/ao", ""); }
TEST_CASE("ted", "[music]") { testPlugin<musix::TEDPlugin>("testmus/ted", ""); }
TEST_CASE("v2", "[music]") { testPlugin<musix::V2Plugin>("testmus/v2", ""); }

TEST_CASE("coverage", "[music]")
{
    musix::ChipPlugin::createPlugins("data");
    auto& plugins = musix::ChipPlugin::getPlugins();
    
    std::vector<std::string> allMissing;
    std::set<std::string> missingFolders;
    
    std::unordered_map<std::string, std::string> pluginDirs = {
        {"gme", "testmus/gme/working"},
        {"adlib", "testmus/adlib"},
        {"uade", "testmus/uade"},
        {"OpenMPT", "testmus/openmpt"},
        {"gsf", "testmus/gsf"},
        {"nds", "testmus/nds"},
        {"psx", "testmus/psx"},
        {"zx", "testmus/zx"},
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
}
