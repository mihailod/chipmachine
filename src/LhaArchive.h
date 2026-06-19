#ifndef LHA_ARCHIVE_H
#define LHA_ARCHIVE_H

#include <string>
#include <vector>

namespace chipmachine {

// Extract every file member of the LHA/LZH archive at `lhaPath` into `destDir`.
// The single wrapper directory that UnExoticA archives put everything under
// (named after the game) is stripped, but any deeper subdirectory structure is
// preserved (e.g. the Sonix driver's "instruments/"), with directories created
// as needed. Returns the wrapper-relative names of the extracted files, or an
// empty vector on failure. Existing files are overwritten. Used by the UnExoticA
// collection, whose tunes are distributed inside per-product .lha archives.
std::vector<std::string> extractLha(std::string const& lhaPath,
                                    std::string const& destDir);

} // namespace chipmachine

#endif // LHA_ARCHIVE_H
