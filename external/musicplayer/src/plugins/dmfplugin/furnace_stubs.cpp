// Stubs for Furnace symbols that normally live in the GUI/CLI main.cpp we do
// not vendor. DivConfig::save()/loadFromFile() call reportError() on a config
// I/O error -- a path never reached during in-memory .dmf playback (we drive
// DivEngine directly and never touch the on-disk config), so a stderr note is
// all that is needed.
#include "ta-utils.h"
#include <cstdio>

void reportError(String what)
{
    fprintf(stderr, "[dmfplugin] furnace: %s\n", what.c_str());
}
