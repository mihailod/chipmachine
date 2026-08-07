// Runs a .prg through the clean-room TED machine and writes raw 16-bit mono
// 44.1k PCM to stdout, so the differential harness can point the same analysis
// at it and at the engine being replaced.
//
// With TED_DIAG set in the environment it writes a diagnosis to stderr instead
// of audio: instruction/interrupt/sound-write counts, the raster compare and
// interrupt mask, a sample of where the PC has been, and the bytes around
// wherever it ended up. A tune that renders silent is nearly always parked in a
// loop waiting for something, and those bytes say what.
#include "../ted_machine.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

extern "C" uint16_t tedcpu_get_pc(void);

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: ted_run <file.prg> [seconds]\n");
        return 2;
    }
    double secs = argc > 2 ? atof(argv[2]) : 5.0;

    FILE* f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "cannot open %s\n", argv[1]);
        return 2;
    }
    std::vector<uint8_t> data;
    uint8_t buf[65536];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        data.insert(data.end(), buf, buf + n);
    }
    fclose(f);

    musix::tedcr::TedMachine m(44100);
    if (!m.load(data.data(), data.size())) {
        fprintf(stderr, "not a 264-series prg\n");
        return 1;
    }

    bool diag = getenv("TED_DIAG") != nullptr;
    int total = static_cast<int>(secs * 44100);
    std::vector<uint16_t> pcs;
    std::vector<int16_t> out(4410 * 2);
    int done = 0;
    while (done < total) {
        int want = 4410;
        if (want > total - done) {
            want = total - done;
        }
        int got = m.generate(out.data(), want);
        if (got <= 0) {
            break;
        }
        if (diag) {
            pcs.push_back(tedcpu_get_pc());
        } else {
            for (int i = 0; i < got; i++) {
                out[i] = out[i * 2]; // collapse to mono for comparison
            }
            fwrite(out.data(), 2, got, stdout);
        }
        done += got;
    }

    if (diag) {
        const auto& st = m.stats();
        fprintf(stderr,
                "instr=%lld irqs=%lld soundwr=%lld rasterirq=%lld timerirq=%lld "
                "rastercmp=%d mask=$%02X\n",
                st.instructions, st.irqs, st.soundWrites, st.rasterIrqs, st.timerIrqs,
                m.rasterCompare(), m.irqMask());
        fprintf(stderr, "pc samples:");
        for (size_t i = 0; i < pcs.size() && i < 14; i++) {
            fprintf(stderr, " %04X", pcs[i]);
        }
        uint16_t pc = tedcpu_get_pc();
        fprintf(stderr, "\nbytes at %04X:", pc);
        for (int i = -8; i < 16; i++) {
            fprintf(stderr, " %02X", m.read(static_cast<uint16_t>(pc + i)));
        }
        fprintf(stderr, "\n");
    }
    return 0;
}
