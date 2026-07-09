#include "catch.hpp"

#include "chipplugin.h"
#include "plugins/plugins.h"

#include <array>
#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <filesystem>
#include <numeric>
#include <string>
namespace fs = std::filesystem;

static fs::path findProjectDir()
{
    auto current = fs::absolute(".");

    while (!current.empty()) {
        if (fs::exists(current / "testmus")) { return current; }
        current = current.parent_path();
    }
    return {};
}

inline fs::path projDir()
{
    static fs::path projectDir = findProjectDir();
    return projectDir;
}

static auto dataDir = projDir() / "data";

template <typename PLUGIN, typename... ARGS>
int testPlugin(std::string const& dir, std::string const& exclude,
               const ARGS&... args)
{
    auto realDir = projDir() / dir;

    std::vector<std::string> ex;
    if (!exclude.empty()) { ex = utils::split(exclude, ";"); }

    std::array<int16_t, 8192> buffer;
    PLUGIN plugin{args...};
    printf("---- %s ----\n", plugin.name().c_str());
    logging::setLevel(logging::Level::Warning);
    int total = 0;
    int working = 0;
    for (auto const& f : utils::listFiles(realDir, false, false)) {
        bool play = true;
        //printf("%s against '%s'\n", f.c_str(), exclude.c_str());
        for (auto const& e : ex) {
            if (f.string().find(e) != std::string::npos) { play = false; }
        }
        //printf("GO %s\n", play ? "PLAY" : "NO");

        if (!play) { continue; }

        int64_t sum = 0;
        //printf("Trying %s\n", f.string().c_str());
        musix::ChipPlayer* player = nullptr;
        try {
            player = plugin.fromFile(f.string());
        } catch (musix::player_exception& pe) {
            printf("Exception %s\n", pe.what());
        }
        if (player) {
            // puts("Player created");
            int count = 15;
            while (sum == 0 && count != 0) {
                int rc = player->getSamples(&buffer[0], buffer.size());
                // REQUIRE(rc > 0);
                if (rc > 0) {
                    sum = std::accumulate(
                        reinterpret_cast<uint16_t*>(&buffer[0]),
                        reinterpret_cast<uint16_t*>(&buffer[rc]),
                        static_cast<int64_t>(0));
                    // REQUIRE(sum != 0);
                    if (sum > 0) { break; }
                    count--;
                } else {
                    break;
                }
            }
            delete player;
        }

        bool madeSound = (sum > 0);

        if (madeSound) { working++; }
        total++;

        printf("#### Playing %s : %s\n", f.string().c_str(),
               player ? (madeSound ? "OK" : "NO SOUND") : "FAILED");
    }
    if (total == 0) { return 100; }
    int percent = working * 100 / total;
    printf("PERCENT %d\n\n", percent);
    return percent;
}

TEST_CASE("all", "[music]") {}

TEST_CASE("gme", "[music]")
{
    REQUIRE(testPlugin<musix::GMEPlugin>("testmus/gme/working", "") == 100);
    REQUIRE(testPlugin<musix::GMEPlugin>("testmus/gme/nowork", "") == 0);
}

TEST_CASE("adlib", "[music]")
{
    REQUIRE(testPlugin<musix::AdPlugin>("testmus/adlib/working", ".dat;.ins",
                                        dataDir) == 100);
    REQUIRE(testPlugin<musix::AdPlugin>("testmus/adlib/nowork", ".dat;.ins",
                                        dataDir) == 0);
}

TEST_CASE("uade", "[music]")
{
    testPlugin<musix::UADEPlugin>("testmus/uade", "smp", dataDir);
}

TEST_CASE("openmpt", "[music]")
{
    testPlugin<musix::OpenMPTPlugin>("testmus/openmpt", "");
}

TEST_CASE("gsf", "[music]")
{
    testPlugin<musix::GSFPlugin>("testmus/gsf", "lib");
}

TEST_CASE("nds", "[music]")
{
    testPlugin<musix::NDSPlugin>("testmus/nds", "lib");
}

TEST_CASE("psx", "[music]")
{
    testPlugin<musix::HEPlugin>("testmus/psx", "lib", dataDir / "hebios.bin");
}

TEST_CASE("zx", "[music]")
{
    testPlugin<musix::AyflyPlugin>("testmus/zx", "");
}

// Build a minimal, self-contained WonderSwan rip: a 64 KB zero ROM capped with
// the 32-byte "WSRF" footer the loader requires (magic at offset 0, first
// subsong index at offset 5). It carries no real driver, so it stays silent,
// but it lets us exercise the whole detect -> load -> render path against the
// real in_wsr core without committing a copyrighted game rip. (A zero ROM is
// safe to run: every out-of-cart read returns 0xFF and the CPU only ever writes
// into the emulator's own 64 KB RAM banks, while Update_WSR is cycle-bounded.)
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

static void writeFile(const fs::path& p, const std::vector<uint8_t>& data)
{
    FILE* f = fopen(p.string().c_str(), "wb");
    REQUIRE(f != nullptr);
    if (!data.empty()) { fwrite(data.data(), 1, data.size(), f); }
    fclose(f);
}

TEST_CASE("wsr", "[music]")
{
    auto tmp = fs::temp_directory_path();
    auto good = tmp / "musix_wsr_good.wsr";
    auto nofooter = tmp / "musix_wsr_nofooter.wsr";
    auto wrongext = tmp / "musix_wsr_wrongext.bin";

    writeFile(good, makeSyntheticWSR());
    writeFile(nofooter, std::vector<uint8_t>(0x10000, 0)); // no WSRF magic
    writeFile(wrongext, makeSyntheticWSR());               // valid bytes, .bin

    musix::WSRPlugin plugin;

    // Detection: only a .wsr file carrying the WSRF footer is accepted.
    REQUIRE(plugin.canHandle(good.string()));
    REQUIRE_FALSE(plugin.canHandle(nofooter.string()));
    REQUIRE_FALSE(plugin.canHandle(wrongext.string()));
    REQUIRE(plugin.getSupportedExtensions().count("wsr") == 1);

    // Load + render: the synthetic rip must construct and produce the requested
    // number of interleaved stereo samples without throwing or crashing.
    musix::ChipPlayer* player = plugin.fromFile(good.string());
    REQUIRE(player != nullptr);
    std::array<int16_t, 8192> buffer{};
    int rc = player->getSamples(buffer.data(), buffer.size());
    REQUIRE(rc == static_cast<int>(buffer.size()));
    delete player;

    // A footerless file must be rejected at construction time.
    REQUIRE_THROWS_AS(plugin.fromFile(nofooter.string()),
                      musix::player_exception);

    fs::remove(good);
    fs::remove(nofooter);
    fs::remove(wrongext);

    // If real rips have been dropped into testmus/wsr, play them too (this part
    // is informational: the directory is empty in a clean checkout).
    if (fs::exists(projDir() / "testmus/wsr")) {
        testPlugin<musix::WSRPlugin>("testmus/wsr", ".md");
    }
}

TEST_CASE("pokeynoise", "[music]")
{
    musix::PokeyNoisePlugin plugin;
    REQUIRE(plugin.getSupportedExtensions().count("pn") == 1);

    // Detection is content-based (FF FF E0 02 E1 02) and matches both modland's
    // `pn.<song>` prefix form and the rarer `<song>.pn` suffix form.
    auto pnDir = projDir() / "testmus/pn";
    if (fs::exists(pnDir / "pn.draconus")) {
        REQUIRE(plugin.canHandle((pnDir / "pn.draconus").string()));
    }
    // A name that matches but lacks the magic must be rejected.
    auto bogus = fs::temp_directory_path() / "musix_pn_bogus.pn";
    writeFile(bogus, std::vector<uint8_t>(64, 0));
    REQUIRE_FALSE(plugin.canHandle(bogus.string()));
    fs::remove(bogus);

    // Every real sample in testmus/pn must play with sound.
    REQUIRE(testPlugin<musix::PokeyNoisePlugin>("testmus/pn", "") == 100);
}
