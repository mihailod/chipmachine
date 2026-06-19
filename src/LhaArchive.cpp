#include "LhaArchive.h"

#include <coreutils/log.h>

#include <cstring>
#include <filesystem>

extern "C" {
#include <lhasa.h>
}

namespace chipmachine {

std::vector<std::string> extractLha(std::string const& lhaPath,
                                    std::string const& destDir)
{
    std::vector<std::string> names;

    // lha_input_stream_from opens the file itself and closes it on free.
    LHAInputStream* stream =
        lha_input_stream_from(const_cast<char*>(lhaPath.c_str()));
    if (stream == nullptr) {
        LOGW("Could not open LHA archive %s", lhaPath);
        return names;
    }

    LHAReader* reader = lha_reader_new(stream);
    if (reader == nullptr) {
        lha_input_stream_free(stream);
        return names;
    }

    std::error_code ec;
    std::filesystem::create_directories(destDir, ec);

    LHAFileHeader* header;
    while ((header = lha_reader_next_file(reader)) != nullptr) {
        // Skip directory entries (they carry no file data to extract).
        if (strcmp(header->compress_method, LHA_COMPRESS_TYPE_DIR) == 0)
            continue;

        // Work out the member's full stored name. Normally lhasa splits a file
        // into `path` (the containing directory, ending in '/') plus `filename`
        // (the leaf). But some MS-DOS / Human68k LHA variants (level-0 headers,
        // as used by parts of UnExoticA) instead put the COMPLETE file path in
        // `path` and leave `filename` holding garbage -- so trust `path` unless
        // it is a bare directory.
        std::string full;
        if (header->path != nullptr && header->path[0] != '\0') {
            full = header->path;
            if (full.back() == '/') {
                // Standard "directory/" + leaf form.
                if (header->filename != nullptr) full += header->filename;
            }
            // else: `path` already is the complete file path; ignore filename.
        } else if (header->filename != nullptr) {
            full = header->filename;
        } else {
            continue; // nothing usable
        }

        // Flatten to the basename (members can sit in an archive subdirectory):
        // co-locating everything in one dir lets the player find in-archive
        // companions (sample banks, instruments) next to the song. Handle both
        // '/' and '\\' separators.
        auto cut = full.find_last_of("/\\");
        std::string base =
            (cut == std::string::npos) ? full : full.substr(cut + 1);
        if (base.empty()) continue;

        std::string target = destDir + "/" + base;
        if (lha_reader_extract(reader, const_cast<char*>(target.c_str()),
                               nullptr, nullptr) == 0) {
            LOGW("Failed to extract %s from %s", base, lhaPath);
            continue;
        }
        names.emplace_back(base);
    }

    lha_reader_free(reader);
    lha_input_stream_free(stream);
    return names;
}

} // namespace chipmachine
