#include "ChipInterface.h"
#include "MusicPlayer.h"
#ifndef TEXTMODE_ONLY
#    include "ChipMachine.h"
#    include <grappix/grappix.h>
#    ifdef __APPLE__
#        include "macnative/FileOpenHandler.h"
#    endif
#endif
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

#include <cctype>
#include <csignal>
#include <filesystem>
#include <optional>
#include <set>
#include <vector>

#include "version.h"

extern "C" void InitializeUpdateVerificationSubsystem();

namespace chipmachine {
void runConsole(std::shared_ptr<bbs::Console> console, ChipInterface& ci);
}

int main(int argc, char* argv[])
{
    // Ignore SIGPIPE process-wide. We pipe audio through ffmpeg subprocesses
    // (FFMPEGPlayer); when a stream is torn down on a song switch, the feeder
    // thread can write to ffmpeg's stdin just as ffmpeg exits, and a write to a
    // pipe with no reader raises SIGPIPE -- whose default action silently kills
    // the whole app (no crash report). Ignoring it makes that write return EPIPE
    // instead, which feedLoop() already handles by stopping.
    std::signal(SIGPIPE, SIG_IGN);

    // Cache/config live under ~/Library/{Caches,Application Support}/<appName>.
    // The MAS variant uses a distinct name so that, on a dev machine where both
    // variants run un-sandboxed, the App Store build (which drops YouTube rows at
    // index time) does not clobber the full build's music.db and vice-versa. In
    // production the App Sandbox already isolates each by its container.
#ifdef CM_MAS
    Environment::setAppName("chipmachine-mas");
#else
    Environment::setAppName("chipmachine");
#endif

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
        bool full_screen = false;
        bool only_headless = false;
        bool force_reindex = false;
        bool delete_web_cache = false;
        bool no_images = false;
        bool debug = false;
        bool dump_extensions = false;
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
                               options.debug = true;
                               logging::setLevel(logging::Debug);
                           },
                           "Debug output");

    opts.add_flag("--forcedbreindex", options.force_reindex, "Force database rebuild");
    opts.add_flag("--deletewebcache", options.delete_web_cache, "Delete web cache");
    opts.add_flag("--donotloadimages", options.no_images,
                  "Never download screenshots (e.g. when the image host is down)");
    opts.add_flag("-K", options.only_headless,
                  "Only play if no keyboard is connected");
    opts.add_option("--play", options.play_what,
                    "Shuffle a named collection (also 'all' or 'favorites')");
    opts.add_flag("--dump-extensions", options.dump_extensions,
                  "Print every file extension the loaded plugins can play "
                  "(one per line) and exit. Feeds the macOS file-association "
                  "document-type list in package_app.sh; the single source of "
                  "truth is the plugins themselves, so this stays in sync.");
    opts.add_option("files", options.songs, "Songs to play");

    CLI11_PARSE(opts, argc, argv)

#ifndef TEXTMODE_ONLY
    chipmachine::ChipMachine::noImages = options.no_images;
    chipmachine::ChipMachine::debugMode = options.debug;
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
    // Normalize away any "build/.." style prefix so logs and downstream path
    // comparisons show the real root (e.g. .../chipmachine/data, not
    // .../build/../chipmachine/data). Purely lexical -- no behavior change.
    auto work_dir = data_dir->parent_path().lexically_normal();

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

    // Same check for the HVTC store (Commodore 16/116/+4 TED .prg tracks), which
    // was pivoted from the online plus4world mirror to a shipped local folder.
    {
        utils::path music_hvtc = work_dir / "music" / "hvtc";
        if (!utils::exists(music_hvtc)) {
            fprintf(stderr,
                "[chipmachine] WARNING: music/hvtc not found at: %s\n"
                "   Built-in HVTC (.prg) tracks will be unavailable.\n"
#ifdef __APPLE__
                "   Bundle build: re-run package_app.sh (Section 4b copies the tracks).\n"
                "   Dev build:    run `cmake --build` to sync tracks via POST_BUILD,\n"
                "                 or ensure chipmachine/music/hvtc/ exists in the source tree.\n"
#endif
                , music_hvtc.string().c_str());
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

    // --dump-extensions: emit the union of every extension the loaded plugins
    // advertise, deduped and sorted, one per line, then exit. This is the
    // single source of truth for the macOS file-association list baked into the
    // .app's Info.plist by package_app.sh (via extensions.txt). Kept here --
    // after createPlugins() but before the heavy DB/Lua/audio init below -- so
    // it runs fast and touches nothing else.
    if (options.dump_extensions) {
        std::set<std::string> exts;
        for (auto const& pl : musix::ChipPlugin::getPlugins()) {
            for (auto const& e : pl->getSupportedExtensions()) {
                std::string low;
                low.reserve(e.size());
                for (char c : e) low += static_cast<char>(::tolower(c));
                if (!low.empty()) exts.insert(low);
            }
        }
        for (auto const& e : exts) utils::print_fmt("%s\n", e);
        return 0;
    }

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
#ifndef CM_MAS
    // The Mac App Store build ships no YouTube plugin: yt-dlp is a spawned
    // executable (App Store guideline 2.5.2) and has no in-process, MAS-legal
    // form. Without this plugin, no youtube.com URL is claimed by any decoder,
    // and MusicDatabase drops the YouTube-only catalog rows at index time.
    initYoutube(*lua);
#endif

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

    if (options.text_mode) {

        static auto chip_interface =
            injector.create<std::unique_ptr<chipmachine::ChipInterface>>();
#ifndef _WIN32
        logging::setLevel(logging::Error);
        auto console = std::shared_ptr<bbs::Console>(
            bbs::Console::createLocalConsole());
        chipmachine::runConsole(console, *chip_interface);
#else
        puts("Textmode not supported on Windows");
        exit(0);
#endif
        return 0;
    }
#ifndef TEXTMODE_ONLY
#ifdef __APPLE__
    // Install the Finder "Open With" / double-click handler BEFORE screen.open()
    // -- and therefore before glfwInit(). GLFW runs [NSApp run] inside glfwInit()
    // to finish launching, which is exactly where AppKit dispatches the
    // cold-launch open-document event. installFileOpenHandler() teaches GLFW's
    // app delegate class to answer -application:openURLs: so that dispatch is
    // delivered to us instead of hitting AppKit's "cannot open this format"
    // fallback. ChipMachine::update() drains the queued paths and plays them.
    chipmachine::installFileOpenHandler();

    // Re-acquire sandbox access to any external files a previous session opened
    // and the user saved to Favorites/a playlist. Self-gates at runtime: a no-op
    // in the non-sandboxed plus build. Must run before those lists are played.
    chipmachine::restoreSecurityScopedFiles();
#endif

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

