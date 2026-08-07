// Drives ted_sound.c and dumps raw 16-bit mono 44.1k PCM on stdout, so the same
// measurement scripts used on the tedplay engine can be pointed at the new core.
//
//   ted_test tone   <N> <ctrl> <secs>
//   ted_test tone2  <N1> <N2> <ctrl> <secs>
//   ted_test noise  <N> <secs>
//   ted_test da     <ctrl> <halfperiod_cycles> <secs>   volume toggled 8<->0
//   ted_test seq                                        prints the noise bits
#include "../ted_sound.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SR 44100
#define CLK TEDSND_CLOCK_PAL

static void emit(int cycles)
{
    int16_t buf[4096];
    while (cycles > 0) {
        int chunk = cycles;
        int dt = chunk;
        int n = tedsnd_render(buf, 4096, 1, &dt);
        if (n <= 0) break;
        fwrite(buf, 2, n, stdout);
        cycles -= (chunk - dt);
    }
}

int main(int argc, char** argv)
{
    if (argc < 2) return 2;
    tedsnd_init(CLK, SR);

    if (!strcmp(argv[1], "tone")) {
        int n = atoi(argv[2]), ctrl = (int)strtol(argv[3], 0, 16);
        double secs = atof(argv[4]);
        tedsnd_store(0x0E, n & 0xff);
        tedsnd_store(0x12, (n >> 8) & 3);
        tedsnd_store(0x11, ctrl);
        emit((int)(secs * CLK));
    } else if (!strcmp(argv[1], "tone2")) {
        int n1 = atoi(argv[2]), n2 = atoi(argv[3]);
        int ctrl = (int)strtol(argv[4], 0, 16);
        double secs = atof(argv[5]);
        tedsnd_store(0x0E, n1 & 0xff);
        tedsnd_store(0x12, (n1 >> 8) & 3);
        tedsnd_store(0x0F, n2 & 0xff);
        tedsnd_store(0x10, (n2 >> 8) & 3);
        tedsnd_store(0x11, ctrl);
        emit((int)(secs * CLK));
    } else if (!strcmp(argv[1], "noise")) {
        int n = atoi(argv[2]);
        double secs = atof(argv[3]);
        tedsnd_store(0x0F, n & 0xff);
        tedsnd_store(0x10, (n >> 8) & 3);
        tedsnd_store(0x11, 0x48);
        emit((int)(secs * CLK));
    } else if (!strcmp(argv[1], "da")) {
        int ctrl = (int)strtol(argv[2], 0, 16);
        int half = atoi(argv[3]);
        double secs = atof(argv[4]);
        // both counters parked on the locked value
        tedsnd_store(0x0E, 0xFE); tedsnd_store(0x12, 0x03);
        tedsnd_store(0x0F, 0xFE); tedsnd_store(0x10, 0x03);
        int total = (int)(secs * CLK), t = 0, phase = 0;
        while (t < total) {
            tedsnd_store(0x11, (ctrl & 0xF0) | (phase ? 8 : 0));
            emit(half);
            t += half; phase ^= 1;
        }
    } else if (!strcmp(argv[1], "seq")) {
        // the noise register's own sequence, straight out of the core
        unsigned char s = 0x00;
        int i;
        for (i = 0; i < 600; i++) {
            putchar('0' + (s & 1));
            unsigned int fb = 1u ^ ((s >> 7) & 1u) ^ ((s >> 5) & 1u) ^
                              ((s >> 4) & 1u) ^ ((s >> 1) & 1u);
            s = (unsigned char)((s << 1) | fb);
        }
        putchar('\n');
    }
    return 0;
}
