/*
  cmdxtrace -- emit the same trace format as mdxtrace, but from the cmdx
  (clean-room) engine, so the two can be diffed with mdxdiff.py.

  Deliberately does no audio synthesis: register writes are the observable
  behaviour under test, and they are produced before any chip is involved.

    cmdxtrace [-f frames] [-q] <file.mdx>
*/

#include "cmdx_player.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>

static FILE* out = stdout;
static long events = 0;

static void onReg(void* /*ctx*/, int reg, int val)
{
    fprintf(out, "O %02x %02x\n", reg & 0xFF, val & 0xFF);
    events++;
}

int main(int argc, char** argv)
{
    const char* path = nullptr;
    int frames = 3600;
    int quiet = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-f") && i + 1 < argc) frames = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-q")) quiet = 1;
        else if (argv[i][0] == '-') { fprintf(stderr, "usage: cmdxtrace [-f n] [-q] <file.mdx>\n"); return 2; }
        else path = argv[i];
    }
    if (!path) { fprintf(stderr, "usage: cmdxtrace [-f n] [-q] <file.mdx>\n"); return 2; }

    FILE* f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "cmdxtrace: cannot open %s\n", path); return 1; }
    std::vector<uint8_t> bytes;
    uint8_t buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) > 0)
        bytes.insert(bytes.end(), buf, buf + n);
    fclose(f);

    cmdx::File mdx;
    if (!mdx.load(bytes.data(), bytes.size())) {
        fprintf(stderr, "cmdxtrace: %s: %s\n", path, mdx.error().c_str());
        return 1;
    }

    /* Header first: the chip reset happens during init, and those writes are
       part of the trace -- same ordering rule as mdxtrace. */
    if (!quiet) {
        fprintf(out, "# mdxtrace 1\n");
        fprintf(out, "# engine cmdx\n");
        fprintf(out, "# rate 44100\n");
        fprintf(out, "# frames %d\n", frames);
    }

    cmdx::Player pl;
    pl.init(mdx, onReg, nullptr);

    for (int fr = 0; fr <= frames; fr++) {
        fprintf(out, "F %d %d\n", fr, pl.tempo());
        events++;
        if (!pl.tick())
            break;
    }

    if (!quiet)
        fprintf(out, "# events %ld\n", events);
    return 0;
}
