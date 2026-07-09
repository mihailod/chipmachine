#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <coreutils/environment.h>
#include <coreutils/searchpath.h>

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

// The tests read their fixtures (testmus/, data/) via paths relative to the
// current directory. Rather than force a specific launch dir, locate the source
// root the same way the app does (see src/main.cpp) — search getExeDir()-relative
// candidates for the directory that contains testmus/, then chdir into it. This
// lets cmtest run from anywhere; in particular `build % ./cmtest`, matching how
// the app is launched (`build % ./chipmachine`). testmus/ is the unambiguous
// marker of the source test root (it exists only in the source tree, never in a
// packaged bundle), so keying on it avoids landing in a data-only resource root.
static void enterTestRoot()
{
    auto search = makeSearchPath(
        {
            Environment::getExeDir() / ".." / "chipmachine",       // dev: build/ beside chipmachine/
            Environment::getExeDir() / ".." / ".." / "chipmachine",
            Environment::getExeDir() / "..",
            Environment::getExeDir() / ".." / "..",
            Environment::getAppDir(),
            utils::path(".")                                        // fallback: already in the root
        },
        true);

    auto root = findFile(search, "testmus");
    if (!root) {
        fprintf(stderr,
            "** cmtest: could not locate the test root (a directory containing\n"
            "   'testmus/'). Searched relative to the executable and the current\n"
            "   directory. Run cmtest from the workspace build dir\n"
            "   (workspace_root/build/cmtest) or from the chipmachine/ source dir.\n");
        exit(2);
    }

    auto work_dir = root->parent_path().lexically_normal();
    if (chdir(work_dir.string().c_str()) != 0) {
        fprintf(stderr, "** cmtest: chdir to '%s' failed\n", work_dir.string().c_str());
        exit(2);
    }
}

int main(int argc, char* argv[])
{
    // Make fixture-relative paths resolve regardless of where cmtest is launched.
    enterTestRoot();

    // Use the same cache/config dirs as the app (main.cpp sets this too), so
    // cmtest reads/writes the one shared ~/Library/Caches/chipmachine/music.db
    // instead of a stray ~/Library/Caches/music.db from an empty app name.
    // Must run before any getCacheDir()/getConfigDir() call, which cache their
    // result on first use.
    Environment::setAppName("chipmachine");
    return Catch::Session().run(argc, argv);
}
