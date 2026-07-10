#pragma once

#include <algorithm>
#include <cstring>
#include <string>
#include <tuple>

/** Get basename, but also handles urlencoded path names */
inline std::string getBaseName(std::string const& filename)
{
    auto fnstart = std::string::npos;
    while (true) {
        fnstart = filename.find_last_of("%/\\", fnstart);
        if ((fnstart == std::string::npos) || (filename[fnstart] != '%')) break;
        if ((filename[fnstart + 1] == '2') && (filename[fnstart + 2] == 'f')) {
            fnstart += 2;
            break;
        }
        // A '%' that isn't "%2f" (e.g. any byte of a percent-encoded UTF-8 name
        // like "%E5%A4%A7...MDX"): keep scanning earlier for a real separator.
        // But if this '%' is at position 0 there is nothing earlier -- decrement
        // would wrap the unsigned index to npos and find_last_of() would restart
        // from the end, spinning forever. Stop: the whole string is the basename.
        if (fnstart == 0) {
            fnstart = std::string::npos;
            break;
        }
        fnstart--;
    }

    return filename.substr(fnstart + 1);
}

inline std::tuple<std::string, std::string>
getTypeAndBase(std::string const& filename)
{
    // Modland prefix-form names ("<type>.<song>"). MUST stay sorted: looked up
    // with std::lower_bound below. "pn" is PokeyNoise (e.g. "pn.jetsetwilly").
    constexpr char const* knownExts[] = {
        "ash", "jpn", "mdat", "mod", "pn", "smp", "smpl", "sng",
    };

    auto base = getBaseName(filename);

    auto firstDot = base.find_first_of('.');
    auto lastDot = base.find_last_of('.');
    if (firstDot != std::string::npos) {
        auto prefix = base.substr(0, firstDot);
        auto suffix = base.substr(lastDot + 1);

        auto it = std::lower_bound(std::begin(knownExts), std::end(knownExts),
                                   prefix.c_str(),
                                   [](char const* a, char const* b) -> bool {
                                       return strcmp(a, b) < 0;
                                   });
        if (it != std::end(knownExts) && strcmp(*it, prefix.c_str()) == 0)
            return std::make_tuple(prefix, base.substr(firstDot + 1));
        return std::make_tuple(suffix, base.substr(0, lastDot));
    }
    return std::make_tuple("", base);
}

inline std::string getTypeFromName(std::string const& filename)
{
    return std::get<0>(getTypeAndBase(filename));
}
