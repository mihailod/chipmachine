#include "LhaArchive.h"

#include <coreutils/log.h>

#include <algorithm>
#include <cstring>
#include <filesystem>

extern "C" {
#include <lhasa.h>
}

namespace chipmachine {

namespace {

// Work out a member's full stored name. Normally lhasa splits a file into
// `path` (the containing directory, ending in '/') plus `filename` (the leaf).
// But some MS-DOS / Human68k LHA variants (level-0 headers, as used by parts of
// UnExoticA) instead put the COMPLETE file path in `path` and leave `filename`
// holding garbage -- so trust `path` unless it is a bare directory. Returns ""
// if nothing usable. Normalises '\\' to '/'.
std::string memberFullName(LHAFileHeader* header)
{
    std::string full;
    if (header->path != nullptr && header->path[0] != '\0') {
        full = header->path;
        if (full.back() == '/') {
            if (header->filename != nullptr) full += header->filename;
        }
        // else: `path` already is the complete file path; ignore filename.
    } else if (header->filename != nullptr) {
        full = header->filename;
    }
    std::replace(full.begin(), full.end(), '\\', '/');
    return full;
}

// The leading directory shared by every member, ending in '/' (or "" if there
// is none). UnExoticA archives wrap their whole payload in a single dir named
// after the game (e.g. "Spirit_of_Excalibur/"); the database paths are relative
// to *inside* that wrapper, so it must be stripped. Everything below the wrapper
// (a "instruments/" subdir, sample banks, ...) is kept, because replayers like
// the Sonix driver load companions from a subdirectory of the song's own dir.
std::string commonDirPrefix(std::vector<std::string> const& names)
{
    if (names.empty()) return "";
    std::string prefix = names.front();
    for (auto const& n : names) {
        size_t i = 0;
        while (i < prefix.size() && i < n.size() && prefix[i] == n[i])
            i++;
        prefix.resize(i);
        if (prefix.empty()) break;
    }
    // Trim back to the last directory boundary so we never strip a partial name.
    auto cut = prefix.find_last_of('/');
    return (cut == std::string::npos) ? "" : prefix.substr(0, cut + 1);
}

} // namespace

std::vector<std::string> extractLha(std::string const& lhaPath,
                                    std::string const& destDir)
{
    std::vector<std::string> names;

    // Pass 1: read the member names (forward-only reader, no extraction yet) so
    // we can work out the shared wrapper directory to strip.
    std::vector<std::string> fulls;
    {
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
        LHAFileHeader* header;
        while ((header = lha_reader_next_file(reader)) != nullptr) {
            if (strcmp(header->compress_method, LHA_COMPRESS_TYPE_DIR) == 0)
                continue;
            std::string full = memberFullName(header);
            if (!full.empty()) fulls.push_back(full);
        }
        lha_reader_free(reader);
        lha_input_stream_free(stream);
    }

    std::string wrapper = commonDirPrefix(fulls);

    std::error_code ec;
    std::filesystem::create_directories(destDir, ec);

    // Pass 2: re-open and extract, stripping the wrapper but preserving any
    // deeper subdirectory structure (e.g. "instruments/").
    LHAInputStream* stream =
        lha_input_stream_from(const_cast<char*>(lhaPath.c_str()));
    if (stream == nullptr) return names;
    LHAReader* reader = lha_reader_new(stream);
    if (reader == nullptr) {
        lha_input_stream_free(stream);
        return names;
    }

    LHAFileHeader* header;
    while ((header = lha_reader_next_file(reader)) != nullptr) {
        if (strcmp(header->compress_method, LHA_COMPRESS_TYPE_DIR) == 0)
            continue;

        std::string full = memberFullName(header);
        if (full.empty()) continue;

        // Member path relative to the wrapper. Guard against an entry that does
        // not start with the computed wrapper (mixed archives): fall back to the
        // basename so it still lands inside destDir.
        std::string rel;
        if (full.size() > wrapper.size() &&
            full.compare(0, wrapper.size(), wrapper) == 0)
            rel = full.substr(wrapper.size());
        else {
            auto cut = full.find_last_of('/');
            rel = (cut == std::string::npos) ? full : full.substr(cut + 1);
        }
        if (rel.empty()) continue;

        std::string target = destDir + "/" + rel;
        auto slash = target.find_last_of('/');
        if (slash != std::string::npos)
            std::filesystem::create_directories(target.substr(0, slash), ec);

        if (lha_reader_extract(reader, const_cast<char*>(target.c_str()),
                               nullptr, nullptr) == 0) {
            LOGW("Failed to extract %s from %s", rel, lhaPath);
            continue;
        }
        names.emplace_back(rel);
    }

    lha_reader_free(reader);
    lha_input_stream_free(stream);
    return names;
}

} // namespace chipmachine
