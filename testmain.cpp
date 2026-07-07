#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <coreutils/environment.h>

int main(int argc, char* argv[])
{
    // Use the same cache/config dirs as the app (main.cpp sets this too), so
    // cmtest reads/writes the one shared ~/Library/Caches/chipmachine/music.db
    // instead of a stray ~/Library/Caches/music.db from an empty app name.
    // Must run before any getCacheDir()/getConfigDir() call, which cache their
    // result on first use.
    Environment::setAppName("chipmachine");
    return Catch::Session().run(argc, argv);
}
