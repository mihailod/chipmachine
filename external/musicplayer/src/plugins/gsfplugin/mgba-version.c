/* Not part of mGBA upstream -- chipmachine's replacement for the version.c that
 * mGBA's own CMake generates from src/core/version.c.in by shelling out to git.
 * We vendor a snapshot rather than a working clone, so there is no git metadata
 * to interrogate; these constants record the imported commit instead.
 *
 * Nothing in the audio path reads them: core/serialize.c stamps savestates with
 * projectName/projectVersion and feature/commandline.c prints them for --version,
 * neither of which a GSF decoder reaches. They exist so the library links.
 *
 * Keep in step with the commit recorded for `mgba` in external/VENDORED.md.
 */
#include <mgba/core/version.h>

MGBA_EXPORT const char* const gitCommit =
    "669817d03e4858e65d0b992bcd96d3009236cc1e";
MGBA_EXPORT const char* const gitCommitShort = "669817d";
MGBA_EXPORT const char* const gitBranch = "master";
MGBA_EXPORT const int gitRevision = 0;
MGBA_EXPORT const char* const binaryName = "mgba";
MGBA_EXPORT const char* const projectName = "mGBA";
MGBA_EXPORT const char* const projectVersion = "0.11-dev-669817d";
