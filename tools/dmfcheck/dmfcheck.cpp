// dmfcheck -- corpus validator for the clean-room DefleMask parser.
//
// DMF has no length fields, no section markers and no trailing sentinel, so a
// mis-sized version gate does not fail loudly: it silently shifts every
// subsequent field. The one property that catches this is total length -- a
// correct parse consumes the inflated buffer EXACTLY. This tool runs the parser
// in strict mode over a directory tree and reports, per DMF version, how many
// files land exactly on the end.
//
// That is also how the three version boundaries with no published spec (0x14,
// 0x17, and everything above 0x18) were pinned down -- see README.md.
//
//   dmfcheck <dir> [--system 0x02,0x42] [--verbose]

#include "../../external/musicplayer/src/plugins/dmfcrplugin/dmf_file.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <dirent.h>
#include <sys/stat.h>

namespace {

void walk(const std::string& dir, std::vector<std::string>& out)
{
    DIR* d = opendir(dir.c_str());
    if (d == nullptr) { return; }
    while (dirent* e = readdir(d)) {
        std::string n = e->d_name;
        if (n == "." || n == "..") { continue; }
        std::string p = dir + "/" + n;
        struct stat st;
        if (stat(p.c_str(), &st) != 0) { continue; }
        if (S_ISDIR(st.st_mode)) {
            walk(p, out);
        } else if (S_ISREG(st.st_mode)) {
            out.push_back(p);
        }
    }
    closedir(d);
}

std::vector<uint8_t> readFile(const std::string& p)
{
    std::ifstream in(p, std::ios::binary);
    if (!in) { return {}; }
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
}

struct Stat
{
    int total = 0;
    int ok = 0;
    int failed = 0;
    std::vector<std::string> failures;
};

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: dmfcheck <dir> [--system 0x02,0x42] [--verbose]\n");
        return 2;
    }
    std::string dir = argv[1];
    std::set<int> wantSystems;
    bool verbose = false;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "--system") == 0 && i + 1 < argc) {
            char* s = argv[++i];
            for (char* tok = strtok(s, ","); tok != nullptr; tok = strtok(nullptr, ",")) {
                wantSystems.insert((int)strtol(tok, nullptr, 0));
            }
        }
    }

    std::vector<std::string> files;
    walk(dir, files);

    std::map<int, Stat> byVersion;
    std::map<int, int> bySystem;
    std::map<std::string, int> errHist;
    int considered = 0, inflateFail = 0, skipped = 0;

    for (auto const& f : files) {
        auto raw = readFile(f);
        if (raw.size() < 4 || raw[0] != 0x78) { skipped++; continue; }

        std::vector<uint8_t> inflated;
        std::string err;
        if (!dmfcr::inflateDmf(raw.data(), raw.size(), inflated, err)) {
            inflateFail++;
            if (verbose) { printf("INFLATE-FAIL %s: %s\n", f.c_str(), err.c_str()); }
            continue;
        }
        if (inflated.size() < 18) { skipped++; continue; }

        int sys = inflated[17];
        if (!wantSystems.empty() && wantSystems.count(sys) == 0) { continue; }

        int ver = inflated[16];
        bySystem[sys]++;
        considered++;

        dmfcr::Module m;
        Stat& st = byVersion[ver];
        st.total++;
        if (dmfcr::parseDmf(inflated.data(), inflated.size(), m, err, true)) {
            st.ok++;
        } else {
            st.failed++;
            // Normalise the byte count out of the message so the histogram
            // groups "N trailing bytes" together.
            std::string key = err;
            size_t sp = key.find(" trailing bytes");
            if (sp != std::string::npos) {
                size_t n = strtoul(err.c_str(), nullptr, 10);
                key = (n == 1) ? "1 trailing byte" : "N trailing bytes";
            }
            errHist[key]++;
            if (st.failures.size() < 3) { st.failures.push_back(f + ": " + err); }
            if (verbose) { printf("PARSE-FAIL %s: %s\n", f.c_str(), err.c_str()); }
        }
    }

    printf("scanned %zu files, considered %d, inflate-fail %d, non-dmf %d\n\n",
           files.size(), considered, inflateFail, skipped);

    printf("-- strict parse by DMF version --\n");
    int gOk = 0, gTot = 0;
    for (auto const& kv : byVersion) {
        const Stat& s = kv.second;
        gOk += s.ok;
        gTot += s.total;
        printf("  ver %3d 0x%02X   %4d / %4d  %s\n", kv.first, kv.first, s.ok,
               s.total, s.ok == s.total ? "OK" : "<-- MISMATCH");
        for (auto const& f : s.failures) { printf("        %s\n", f.c_str()); }
    }
    printf("\n  TOTAL %d / %d exact\n", gOk, gTot);

    printf("\n-- failure reasons --\n");
    for (auto const& kv : errHist) {
        printf("  %-42s %d\n", kv.first.c_str(), kv.second);
    }

    printf("\n-- systems seen --\n");
    for (auto const& kv : bySystem) {
        printf("  0x%02X  %d\n", kv.first, kv.second);
    }

    return gOk == gTot ? 0 : 1;
}
