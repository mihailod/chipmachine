#include "ChipInterface.h"
#include "MusicPlayer.h"
#ifndef TEXTMODE_ONLY
#    include "ChipMachine.h"
#    include <grappix/grappix.h>
#endif
#include <bbsutils/ansiconsole.h>
#include <bbsutils/petsciiconsole.h>
#include <bbsutils/telnetserver.h>
#include <coreutils/environment.h>
#include <coreutils/format.h>
#include <coreutils/searchpath.h>
#include <coreutils/var.h>

#include <audioplayer/audioplayer.h>
#include <musicplayer/src/plugins/plugins.h>
#include <musicplayer/src/chipplugin.h>
#include "../sol2/sol.hpp"


void initYoutube(sol::state&);

#include <psf/PSFFile.h>

#ifndef _WIN32
#    include <bbsutils/console.h>
#    define ENABLE_CONSOLE
#endif
#include "CLI11.hpp"

#include "di.hpp"
namespace di = boost::di;

#include <filesystem>
#include <optional>
#include <vector>

#include "version.h"

extern "C" void InitializeUpdateVerificationSubsystem();

namespace chipmachine {
void runConsole(std::shared_ptr<bbs::Console> console, ChipInterface& ci);
}

int main(int argc, char* argv[])
{
    Environment::setAppName("chipmachine");

#ifdef CM_DEBUG
    logging::setLevel(logging::Level::Debug);
#else
    logging::setLevel(logging::Level::Warning);
#endif

    srand(time(NULL));

    struct
    {
        std::vector<SongInfo> songs;
        int w = 960;
        int h = 540;
        int port = 12345;
        bool full_screen = false;
        bool telnet_server = false;
        bool only_headless = false;
        bool force_reindex = false;
        bool delete_web_cache = false;
        bool no_images = false;
        std::string play_what;
#ifdef TEXTMODE_ONLY
        bool text_mode = true;
#else
        bool text_mode = false;
#endif
    } options;

    static CLI::App opts{ PROGRAM_NAME " " VERSION_STR };

#ifndef TEXTMODE_ONLY
    opts.add_option("--width", options.w, "Width of window");
    opts.add_option("--height", options.h, "Height of window");
    opts.add_flag("-f,--fullscreen", options.full_screen, "Run in fullscreen");
#endif
    opts.add_flag("-X,--textmode", options.text_mode, "Run in textmode");
    opts.add_flag_function("-d",
                           [&](size_t count) {
                               options.full_screen = false;
                               logging::setLevel(logging::Debug);
                           },
                           "Debug output");

    opts.add_flag("--forcedbreindex", options.force_reindex, "Force database rebuild");
    opts.add_flag("--deletewebcache", options.delete_web_cache, "Delete web cache");
    opts.add_flag("--donotloadimages", options.no_images,
                  "Never download screenshots (e.g. when the image host is down)");
    opts.add_option("-T,--telnet", options.telnet_server,
                    "Start telnet server");
    opts.add_option("-p,--port", options.port, "Port for telnet server", true);
    opts.add_flag("-K", options.only_headless,
                  "Only play if no keyboard is connected");
    opts.add_option("--play", options.play_what,
                    "Shuffle a named collection (also 'all' or 'favorites')");
    opts.add_option("files", options.songs, "Songs to play");

    CLI11_PARSE(opts, argc, argv)

#ifndef TEXTMODE_ONLY
    chipmachine::ChipMachine::noImages = options.no_images;
#endif

    if (options.delete_web_cache) {
        utils::print_fmt("Clearing Web Cache...\n");
        auto cacheDir = Environment::getCacheDir();
        auto webFilesDir = cacheDir / "_webfiles";
        LOGD("Deleting web cache directory: %s", webFilesDir.string());
        std::error_code ec;
        std::filesystem::remove_all(webFilesDir.string(), ec);
    }

    InitializeUpdateVerificationSubsystem();

    // -----------------------------------------------------------------------
    // Search path for the resource root (the directory that contains data/,
    // lua/, and music/).
    //
    // Candidate order:
    //   1. Contents/Resources/          — production .app bundle (Apple only)
    //   2. exe/../chipmachine/          — dev: binary in workspace_root/build/
    //   3. exe/../../chipmachine/       — dev: binary nested one level deeper
    //   4. exe/../                      — dev: binary directly in project root
    //   5. exe/../../                   — dev: alternative nesting
    //   6. AppDir                       — Linux AppImage / fallback
    //
    // findFile() walks each candidate and returns the first directory that
    // contains a file or folder named "data" — the presence of data/ is the
    // reliable indicator that we found the correct resource root.
    //
    // IMPORTANT: The bundle candidate (Resources/) must remain FIRST so that
    // a packaged .app never accidentally falls through to a stale dev tree
    // that happens to exist on the same machine.
    // -----------------------------------------------------------------------
    auto search_path = makeSearchPath(
        {
#ifdef __APPLE__
            // Bundle mode: MacOS/chipmachine → ../Resources
            // This is the authoritative production path. It resolves correctly
            // regardless of where the user places the .app on their system,
            // and is fully sandbox-compatible (no home-directory access needed).
            Environment::getExeDir() / ".." / "Resources",
#endif
            Environment::getExeDir() / ".." / "chipmachine",
            Environment::getExeDir() / ".." / ".." / "chipmachine",
            Environment::getExeDir() / "..",
            Environment::getExeDir() / ".." / "..",
            Environment::getAppDir()
        },
        true);
    LOGD("PATH:%s", search_path);

    auto data_dir = findFile(search_path, "data");

    if (!data_dir) {
        fprintf(stderr,
            "** Error: Could not find data files.\n"
#ifdef __APPLE__
            "   Searched for 'data/' inside:\n"
            "     - Contents/Resources/  (bundle mode)\n"
            "     - ../chipmachine/      (dev mode from build/)\n"
            "   If running a packaged .app, re-run package_app.sh to rebuild.\n"
            "   If running in dev mode, ensure the build directory is inside\n"
            "   the workspace root (workspace_root/build/).\n"
#endif
        );
        exit(-1);
    }

    // work_dir is the resource root — the parent of data/, lua/, and music/.
    // All downstream asset paths are constructed relative to this.
    auto work_dir = data_dir->parent_path();

    // Emit a diagnostic early if music/Console is missing from the resolved
    // root. This surfaces packaging regressions immediately at launch rather
    // than as a silent "no songs found" state deep in the music database.
    {
        utils::path music_console = work_dir / "music" / "Console";
        if (!utils::exists(music_console)) {
            fprintf(stderr,
                "[chipmachine] WARNING: music/Console not found at: %s\n"
                "   Built-in .nsfe tracks will be unavailable.\n"
#ifdef __APPLE__
                "   Bundle build: re-run package_app.sh (Section 4b copies the tracks).\n"
                "   Dev build:    run `cmake --build` to sync tracks via POST_BUILD,\n"
                "                 or ensure chipmachine/music/Console/ exists in the source tree.\n"
#endif
                , music_console.string().c_str());
        }
    }

    utils::path binDir = (work_dir / "bin");
    utils::path exeDir = Environment::getExeDir();
    std::string currentPath = getenv("PATH");
    // yt-dlp ships as a PyInstaller *onedir* bundle (bin/ytdlp/yt-dlp + its
    // _internal/ dir) so it cold-starts in ~0.1s. Its containing directory must
    // be on PATH for the bare `yt-dlp` invocation to resolve. We prefer the
    // bundled tools (known-good, fast, reproducible for every user) over
    // whatever is on the system PATH, which stays as a last-resort fallback.
    //
    // NOTE: do NOT bundle the yt-dlp *onefile* here — it re-extracts its whole
    // runtime to a temp dir on every run (~8s), which made each YouTube resolve
    // take ~10s.
    utils::path exeYtdlpDir = exeDir / "ytdlp"; // bundle layout (Contents/MacOS/ytdlp)
    utils::path binYtdlpDir = binDir / "ytdlp"; // dev layout (chipmachine/bin/ytdlp)
    std::string newPath =
        exeDir.string() + ":" + exeYtdlpDir.string() + ":" +
        binDir.string() + ":" + binYtdlpDir.string() + ":" + currentPath;
    setenv("PATH", newPath.c_str(), 1);

    utils::path certPath = (work_dir / "cert.pem");
    if (utils::exists(certPath)) {
        setenv("SSL_CERT_FILE", certPath.string().c_str(), 0); // Don't overwrite if set
    }

    musix::ChipPlugin::createPlugins(work_dir / "data");

    auto lua = std::make_shared<sol::state>();
    lua->open_libraries(sol::lib::base, sol::lib::package, sol::lib::string);
    lua->set_function("print", [](sol::variadic_args va) {
        std::string s;
        for (auto const& arg : va) {
            if (!s.empty()) s += "\t";
            s += arg.as<std::string>();
        }
        LOGD("[LUA] %s", s.c_str());
    });
    
    lua->set_function("cm_execute",
                      [](std::string const& cmd) -> std::string {
                          return utils::execPipe(cmd);
                      });


    lua->script_file((work_dir / "lua" / "init.lua").string());
    initYoutube(*lua);

    auto audio_player = std::make_shared<AudioPlayer>(44100);
    auto injector =
        di::make_injector(di::bind<AudioPlayer>.to(audio_player),
                          di::bind<chipmachine::MusicDatabase>.in(di::singleton),
                          di::bind<chipmachine::MusicPlayerList>.in(di::singleton),
                          di::bind<RemoteLoader>.in(di::singleton),
                          di::bind<utils::path>.to(work_dir),
                          di::bind<sol::state>.to(lua));

    LOGD("WorkDir:%s", work_dir);

    auto& music_db = injector.create<chipmachine::MusicDatabase&>();
    if (options.force_reindex) {
        music_db.forceRebuild();
    }

    if (!options.songs.empty()) {
        int pos = 0;
#ifdef ENABLE_CONSOLE
        auto* console = bbs::Console::createLocalConsole();
#endif
        static auto music_player =
            injector.create<std::unique_ptr<chipmachine::MusicPlayer>>();

        while (true) {
            if (pos >= options.songs.size()) return 0;
            music_player->playFile(options.songs[pos++].path);
            SongInfo info = music_player->getPlayingInfo();
            utils::print_fmt(
                "Playing: %s\n",
                !info.title.empty()
                    ? info.title
                    : utils::path_filename(options.songs[pos - 1].path));
            int tune = 0;
            while (music_player->playing()) {
                music_player->update();
#ifdef ENABLE_CONSOLE
                if (console) {
                    auto key = console->getKey(100);
                    if (key != bbs::Console::KEY_TIMEOUT) {
                        switch (key) {
                        case bbs::Console::KEY_RIGHT:
                            music_player->seek(tune++);
                            break;
                        case bbs::Console::KEY_ENTER:
                            music_player->stop();
                            break;
                        }
                    }
                }
#endif
            }
        }
        return 0;
    }

    if (options.text_mode || options.telnet_server) {

        static auto chip_interface =
            injector.create<std::unique_ptr<chipmachine::ChipInterface>>();
        if (options.text_mode) {
#ifndef _WIN32
            logging::setLevel(logging::Error);
            auto console = std::shared_ptr<bbs::Console>(
                bbs::Console::createLocalConsole());
            chipmachine::runConsole(console, *chip_interface);
            if (options.telnet_server)
                std::thread conThread(chipmachine::runConsole, console,
                                      std::ref(*chip_interface));
            else
                chipmachine::runConsole(console, *chip_interface);
#else
            puts("Textmode not supported on Windows");
            exit(0);
#endif
        }
        if (options.telnet_server) {
            auto telnet = std::make_shared<bbs::TelnetServer>(options.port);
            telnet->setOnConnect([&](bbs::TelnetServer::Session& session) {
                try {
                    std::shared_ptr<bbs::Console> console;
                    session.echo(false);
                    auto term_type = session.getTermType();
                    LOGD("New telnet connection, TERMTYPE '%s'", term_type);

                    if (term_type.length() > 0) {
                        console = std::make_shared<bbs::AnsiConsole>(session);
                    } else {
                        console =
                            std::make_shared<bbs::PetsciiConsole>(session);
                    }
                    runConsole(console, *chip_interface);
                } catch (bbs::TelnetServer::disconnect_excpetion& e) {
                    LOGD("Got disconnect");
                }
            });
            telnet->run();
        }
        return 0;
    }
#ifndef TEXTMODE_ONLY
    grappix::screen.setTitle(PROGRAM_NAME " " VERSION_STR);
    if (options.full_screen)
        grappix::screen.open(true);
    else
        grappix::screen.open(options.w, options.h, false);

    auto chip_machine =
        injector.create<std::unique_ptr<chipmachine::ChipMachine>>();

    if (!options.play_what.empty() &&
        (!options.only_headless || !grappix::screen.haveKeyboard()))
        chip_machine->playNamed(options.play_what);

    grappix::screen.render_loop(
        [&chip_machine](uint32_t delta) {
            chip_machine->update();
            chip_machine->render(delta);
        },
        20);
#endif

    LOGD("Controlled exit");

    return 0;
}

