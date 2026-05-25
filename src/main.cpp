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

// Python C-API Header for native execution
#include <Python.h>

void initYoutube(sol::state&);

#include <psf/PSFFile.h>

#ifndef _WIN32
#    include <bbsutils/console.h>
#    define ENABLE_CONSOLE
#endif
#include "CLI11.hpp"

#include "di.hpp"
namespace di = boost::di;

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

    opts.add_option("-T,--telnet", options.telnet_server,
                    "Start telnet server");
    opts.add_option("-p,--port", options.port, "Port for telnet server", true);
    opts.add_flag("-K", options.only_headless,
                  "Only play if no keyboard is connected");
    opts.add_option("--play", options.play_what,
                    "Shuffle a named collection (also 'all' or 'favorites')");
    opts.add_option("files", options.songs, "Songs to play");

    CLI11_PARSE(opts, argc, argv)

    InitializeUpdateVerificationSubsystem();

    auto search_path = makeSearchPath(
        {
#ifdef __APPLE__
            Environment::getExeDir() / ".." / "Resources",
#else
            Environment::getExeDir(),
#endif
            Environment::getExeDir() / ".." / "chipmachine",
            Environment::getExeDir() / ".." / ".." / "chipmachine",
            Environment::getExeDir() / "..",
            Environment::getExeDir() / ".." / "..", Environment::getAppDir() },
        true);
    LOGD("PATH:%s", search_path);

    auto data_dir = findFile(search_path, "data");

    if (!data_dir) {
        fprintf(stderr, "** Error: Could not find data files\n");
        exit(-1);
    }

    auto work_dir = data_dir->parent_path();

    utils::path binDir = (work_dir / "bin");
    utils::path exeDir = Environment::getExeDir();
    std::string currentPath = getenv("PATH");
    std::string newPath =
        exeDir.string() + ":" + binDir.string() + ":" + currentPath;
    setenv("PATH", newPath.c_str(), 1);
    //printf("NEW PATH SET TO: %s\n", newPath.c_str());
    //fflush(stdout);

    utils::path certPath = (work_dir / "cert.pem");
    if (utils::exists(certPath)) {
        setenv("SSL_CERT_FILE", certPath.string().c_str(), 0); // Don't overwrite if set
        //printf("SSL_CERT_FILE SET TO: %s\n", certPath.string().c_str());
        //fflush(stdout);
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

    // Native in-memory embedded Python runtime bridge - Portable & Bundle Safe
    lua->set_function("cm_python_extract", [work_dir](std::string const& url) -> std::string {
        utils::path runtime_base = work_dir / "data" / "python_runtime";
        std::string py_home_path = runtime_base.string();
        std::string script_dir = (work_dir / "data").string();
        std::string site_packages_dir = (runtime_base / "site-packages").string();

        if (!Py_IsInitialized()) {
            std::wstring w_py_home(py_home_path.begin(), py_home_path.end());
            
            // Reconstruct absolute internal standard library locations dynamically relative to target workspace bundle
            std::string full_python_path = 
                (runtime_base / "lib" / "python3.14").string() + ":" +
                (runtime_base / "lib" / "python3.14" / "lib-dynload").string();
            std::wstring w_py_path(full_python_path.begin(), full_python_path.end());

            PyConfig config;
            PyConfig_InitIsolatedConfig(&config);

            // Maintain strict hermetic isolation inside bundles to bypass host machine environmental pollutions
            config.isolated = 1; 
            config.use_environment = 0;

            PyStatus status = PyConfig_SetString(&config, &config.home, w_py_home.c_str());
            if (PyStatus_Exception(status)) { PyConfig_Clear(&config); return ""; }

            status = PyConfig_SetString(&config, &config.pythonpath_env, w_py_path.c_str());
            if (PyStatus_Exception(status)) { PyConfig_Clear(&config); return ""; }

            status = Py_InitializeFromConfig(&config);
            PyConfig_Clear(&config);
            if (PyStatus_Exception(status)) {
                fprintf(stderr, "[CRITICAL] Fatal: Embedded Python subsystem failed to boot. Standard library missing from runtime home layout.\n");
                fflush(stderr);
                return "";
            }

            // Explicitly force injection of both the script directory and third-party site-packages to sys.path
            std::string python_injection_cmd = 
                "import sys\n"
                "if '" + script_dir + "' not in sys.path: sys.path.append('" + script_dir + "')\n"
                "if '" + site_packages_dir + "' not in sys.path: sys.path.append('" + site_packages_dir + "')\n";
            PyRun_SimpleString(python_injection_cmd.c_str());
        }

        // FORCE A FRESH RELOAD VIA PYTHON EXECUTION TO PREVENT CACHE CORRUPTION ON SUBSEQUENT PLAYS
        std::string reload_cmd = 
            "import importlib\n"
            "import extractor\n"
            "importlib.reload(extractor)\n";
        PyRun_SimpleString(reload_cmd.c_str());

        PyObject* pName = PyUnicode_DecodeFSDefault("extractor");
        if (PyErr_Occurred()) { PyErr_Print(); return ""; }

        PyObject* pModule = PyImport_Import(pName);
        Py_DECREF(pName);

        if (!pModule) {
            if (PyErr_Occurred()) { PyErr_Print(); }
            return "";
        }

        PyObject* pFunc = PyObject_GetAttrString(pModule, "get_audio_url");
        if (!pFunc || !PyCallable_Check(pFunc)) {
            if (PyErr_Occurred()) { PyErr_Print(); }
            Py_XDECREF(pFunc);
            Py_DECREF(pModule);
            return "";
        }

        PyObject* pUrlStr = PyUnicode_FromString(url.c_str());
        if (!pUrlStr) {
            if (PyErr_Occurred()) { PyErr_Print(); }
            Py_DECREF(pFunc);
            Py_DECREF(pModule);
            return "";
        }

        PyObject* pArgs = PyTuple_Pack(1, pUrlStr);
        PyObject* pValue = PyObject_CallObject(pFunc, pArgs);
        Py_DECREF(pArgs);
        Py_DECREF(pUrlStr);
        Py_DECREF(pFunc);
        Py_DECREF(pModule);

        std::string stream_url = "";
        if (pValue != nullptr) {
            stream_url = PyUnicode_AsUTF8(pValue);
            Py_DECREF(pValue);
        } else {
            if (PyErr_Occurred()) { PyErr_Print(); }
        }

        return stream_url;
    });

    lua->script_file((work_dir / "lua" / "init.lua").string());
    initYoutube(*lua);

    AudioPlayer audio_player{ 44100 };
    const auto injector =
        di::make_injector(di::bind<AudioPlayer>.to(audio_player),
                          di::bind<utils::path>.to(work_dir),
                          di::bind<sol::state>.to(lua));
    LOGD("WorkDir:%s", work_dir);

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
