#pragma once

#include <coreutils/log.h>
#include <coreutils/split.h>
#include <coreutils/utils.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

template <typename T> T read(std::ifstream& s)
{
    T t{};
    s.read(reinterpret_cast<char*>(&t), sizeof(T));
    return t;
}

class PSFFile
{
public:
    explicit PSFFile(fs::path const& name)
    {
        std::ifstream f(name, std::ios::in | std::ios::binary);
        auto fileSize = fs::file_size(name);
        std::array<char, 6> header;
        f.read(header.data(), 4);

        if (memcmp(header.data(), "PSF", 3) == 0) {

            LOGD("PSF VERSION {}", header[3]);

            auto resLen = read<uint32_t>(f);
            auto comprLen = read<uint32_t>(f);

            auto tagOffset = resLen + comprLen + 16;
            if (tagOffset > fileSize - 5) {
                return;
            }

            f.seekg(tagOffset);
            f.read(header.data(), 5);
            header[5] = 0;

            if (memcmp(header.data(), "[TAG]", 5) == 0) {
                auto tagSize = fileSize - comprLen - resLen - 21;
                std::vector<char> data(tagSize);
                f.read(&data[0], tagSize);
                tagData = std::string(&data[0], tagSize);

                auto lines = utils::split(tagData, "\n");
                for (const auto& l : lines) {
                    auto parts = utils::split(l, '=');
                    if (parts.size() == 2) {
                        _tags[utils::toLower(parts[0])] = parts[1];
                    }
                }
            }
        }
        f.close();
    }

    bool valid() { return !tagData.empty(); }

    double songLength()
    {
        auto it = _tags.find("length");

        if (it == _tags.end()) {
            return -1;
        }
        auto slen = it->second;
        std::vector<std::string> p = utils::split(slen, ":");
        double seconds = -1;
        if (p.size() == 2) {
            seconds = stod(p[0]) * 60.0 + stod(p[1]);
        } else {
            seconds = stod(slen);
        }
        return seconds;
    }

    std::string getTagData() const { return tagData; }

    std::unordered_map<std::string, std::string>& tags() { return _tags; }

private:
    std::string tagData;
    std::unordered_map<std::string, std::string> _tags;
};

// Companion libraries shared by "mini" PSF rips (mini-gsf/2sf/psf/qsf/dsf/usf...):
// the small per-track file references one or more program libraries via the PSF
// "_lib", "_lib2", ... tags, and every PSF player opens them by name from the
// file's own directory. Plugins return these from getSecondaryFiles() so the
// host fetches them alongside the stub -- without it, streaming on a clean
// machine downloads only the stub and the loader fails. Returns {} for a
// self-contained file (no _lib) or an unreadable path (PSFFile may throw).
inline std::vector<std::string> psfLibFiles(const std::string& name)
{
    std::vector<std::string> libs;
    try {
        PSFFile psf{name};
        if (!psf.valid()) { return {}; }
        auto& tags = psf.tags();
        auto add = [&](const std::string& key) {
            auto it = tags.find(key);
            if (it == tags.end()) { return; }
            std::string lib = it->second;
            // PSF tag lines may be CRLF-terminated; host fetches by exact name.
            while (!lib.empty() && (lib.back() == '\r' || lib.back() == ' ')) {
                lib.pop_back();
            }
            if (!lib.empty()) { libs.push_back(lib); }
        };
        add("_lib");
        for (int i = 2;; ++i) {
            std::string key = "_lib" + std::to_string(i);
            if (tags.find(key) == tags.end()) { break; }
            add(key);
        }
    } catch (std::exception const&) {
        return {};
    }
    return libs;
}

