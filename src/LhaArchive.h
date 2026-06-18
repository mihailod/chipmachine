#ifndef LHA_ARCHIVE_H
#define LHA_ARCHIVE_H

#include <string>
#include <vector>

namespace chipmachine {

// Extract every file member of the LHA/LZH archive at `lhaPath` into `destDir`
// (flattened to basenames, directories created as needed). Returns the list of
// extracted file names, or an empty vector on failure. Existing files are
// overwritten. Used by the UnExoticA collection, whose tunes are distributed
// inside per-product .lha archives.
std::vector<std::string> extractLha(std::string const& lhaPath,
                                    std::string const& destDir);

} // namespace chipmachine

#endif // LHA_ARCHIVE_H
