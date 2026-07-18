#ifndef SONGINFO_H
#define SONGINFO_H

#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <set>
#include <string>
#include <unordered_map>

struct SongInfo
{
    SongInfo(const std::string& path = "", const std::string& game = "",
             const std::string& title = "", const std::string& composer = "",
             const std::string& format = "", const std::string& info = "", const std::string& ext = "") 
        : ext(ext), path(path), game(game), title(title), composer(composer),
          format(format), metadata{ info, "" }
    {
        auto pos = path.find_last_of(';');
        if (pos != std::string::npos) {
            auto s = path.substr(pos + 1);
            // Only a SHORT, ALL-DIGIT suffix is a subtune selector (e.g.
            // "file.sgc;3"). A trailing ';' (empty suffix) or a non-numeric tail
            // is part of the path/URL itself -- some scene.org demozoo URLs end
            // with a stray ';' -- and must NOT be fed to stoi(), which throws
            // "stoi: no conversion". That uncaught exception previously aborted a
            // whole collection's indexing partway (e.g. Demozoo stopped at the
            // first such row, leaving ~3k of 42k songs indexed).
            if (!s.empty() && s.size() < 3 &&
                s.find_first_not_of("0123456789") == std::string::npos) {
                starttune = stoi(s);
                this->path = path.substr(0, pos);
            }
        }
    }

    enum
    {
        INFO,
        SCREENSHOT
    };

    bool operator==(const SongInfo& other) const
    {
        return path == other.path && starttune == other.starttune;
    }

    std::string path;
    std::string game;
    std::string title;
    std::string composer;
    std::string format;
    std::vector<std::string> metadata;
    std::string ext;

    int numtunes = 0;
    int starttune = -1;
};

// Canonical "no real composer name recorded" test. Matched EXACTLY (trimmed,
// case-insensitive), NEVER by substring -- "Hyperunknown"/"The Unknown"/"UnknownPotato"
// are real handles a '%unknown%' test would swallow. The big buckets are ""
// (~173k songs), "?" (~19k) and "<?>" (~3k) -- ~a quarter of the DB, not an edge case.
// Single source of truth for: the search dedup weak-identity guard
// (MusicDatabase::search), the shuffle seed (ChipMachine::isUnknownComposer delegates
// here), and displayComposer() below.
inline bool isUnknownComposer(const std::string& composer)
{
    static const std::set<std::string> unknowns = {
        "",         "?",                "??",               "???",
        "<?>",      "-",                "--",               "unknown",
        "unknown composer", "unknown composers",
        "unknown artist",   "unknown artists",
        "_unknown", "anonymous",        "none",
    };
    return unknowns.count(utils::toLower(utils::lrstrip(composer))) > 0;
}

// Display label when the composer's real name isn't recorded. The stored value
// ("?"/"<?>"/"unknown"/...) is cryptic and reads as an error in the UI; substitute
// one self-describing string -- the word "Composer" keeps the standalone info-panel
// line legible. DISPLAY ONLY: never write this back into SongInfo.composer, which the
// search / dedup / matching paths still key on the raw value.
inline std::string displayComposer(const std::string& composer)
{
    return isUnknownComposer(composer) ? "Uncredited Composer" : composer;
}

#endif // SONGINFO_H
