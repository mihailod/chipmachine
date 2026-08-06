/*
 * gen_2608rom_synth.c — offline generator (NOT part of the build).
 *
 * Emits emu/cores/fmopn_2608rom.h: the YM2608's six ADPCM-A rhythm voices
 * (BD/SD/TOP/HH/TOM/RIM) as 8192 bytes of ADPCM-A, in the layout libvgm's
 * YM2608_ADPCM_ROM_addr table expects.
 *
 * THIS DATA IS SYNTHESIZED FROM SCRATCH. It is NOT Yamaha's YM2608 rhythm ROM
 * and contains none of it. That ROM is copyrighted firmware and is deliberately
 * not present in this repository. Everything below is ordinary drum synthesis
 * plus an ADPCM-A encoder written for this project.
 *
 * It is therefore NOT bit-accurate to real OPNA rhythm and will not sound
 * identical to it. What is preserved is what affects arrangement rather than
 * timbre: the six voices and their order, each voice's exact byte range (so the
 * address table needs no change), and each voice's peak and AC RMS (so the drum
 * bus sits at the same level and no per-song remixing is needed).
 *
 * This is the companion to s98plugin's fmgen/gen_rhythm_synth.c, which does the
 * same job for the other consumer of the same ROM. The voice designs are the
 * same; the differences are that this one works at the chip's own ADPCM-A rate
 * with the ROM's sample counts, and encodes rather than emitting PCM.
 *
 * There is deliberately NO fmopn_2608rom.h.orig kept beside the generated file,
 * unlike the opnintf.c patch: keeping one would mean keeping the Yamaha ROM.
 *
 * ---------------------------------------------------------------------------
 * Reference levels
 *
 * Measured by decoding the real ROM through libvgm's own decoder, in 12-bit
 * accumulator units:
 *
 *      voice  bytes  samples      ms   peak    AC rms
 *      BD       448      896    48.4   2042       723
 *      SD       640     1280    69.1   1935       339
 *      TOP     5952    11904   643.0   1814       112
 *      HH       384      768    41.5   1764       497
 *      TOM      640     1280    69.1   1799       383
 *      RIM      128      256    13.8   1924       396
 *
 * "AC rms" is RMS about the mean, not raw RMS. That distinction matters for one
 * voice: the real TOP decodes with a DC offset of +477, about 23% of full
 * scale, held for its whole 643 ms. Two nibbles early in the stream push the
 * 12-bit accumulator past its range, and because it WRAPS rather than
 * saturating (see ADPCMA_calc_chan in fmopn.c) the displacement is permanent.
 * Its raw RMS is 490 but almost all of that is the offset; the actual signal is
 * 112. The voices generated here never drive the accumulator out of range, so
 * they carry no such offset -- which is a small improvement, not a regression.
 *
 * Regenerate with:
 *   cc -O2 gen_2608rom_synth.c -lm -o /tmp/gen_2608rom_synth
 *   /tmp/gen_2608rom_synth ../../../../zxtune/3rdparty/vgm/emu/cores/fmopn_2608rom.h
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ADPCM-A playback rate on the YM2608. Derived from the ROM itself: the bass
 * drum occupies 896 samples and is a 48.4 ms sound. */
#define RATE 18512

#define N_BD    896
#define N_SD   1280
#define N_TOP 11904
#define N_HH    768
#define N_TOM  1280
#define N_RIM   256

#define TOTAL_BYTES 0x2000

/* ------------------------------------------------------------------ *
 * Deterministic noise -- rand() would make the output depend on libc, and
 * this file's product is checked in.
 * ------------------------------------------------------------------ */
static unsigned int rng_state;
static void  rng_seed(unsigned int s) { rng_state = s ? s : 1u; }
static double rng_bipolar(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return (double)(int)rng_state / 2147483648.0;
}

typedef struct { double y, a; } lpf;
static void lpf_set(lpf* f, double hz)
{ f->a = 1.0 - exp(-2.0 * M_PI * hz / RATE); f->y = 0.0; }
static double lpf_run(lpf* f, double in) { f->y += f->a * (in - f->y); return f->y; }

typedef struct { lpf lp; } hpf;
static void hpf_set(hpf* f, double hz) { lpf_set(&f->lp, hz); }
static double hpf_run(hpf* f, double in) { return in - lpf_run(&f->lp, in); }

static double decay(int i, int len, double end)
{ return exp(log(end) * (double)i / (double)len); }

/* Remove any DC, scale to the target peak, return the resulting AC RMS. */
static double normalize(double* buf, int len, double target_peak)
{
    int i;
    double mean = 0.0, peak = 0.0, sum = 0.0;
    for (i = 0; i < len; i++) mean += buf[i];
    mean /= len;
    for (i = 0; i < len; i++) { buf[i] -= mean; if (fabs(buf[i]) > peak) peak = fabs(buf[i]); }
    if (peak < 1e-12) peak = 1.0;
    for (i = 0; i < len; i++) buf[i] *= target_peak / peak;
    for (i = 0; i < len; i++) sum += buf[i] * buf[i];
    return sqrt(sum / len);
}

/* ADPCM-A cannot start abruptly. The decoder begins with the accumulator at 0
 * and the step index at 0, where the largest representable jump is 30 units;
 * the index only climbs 144 per maximum-magnitude nibble, so it takes a handful
 * of samples before the codec can reach full scale at all. A voice that opens
 * at peak amplitude is therefore unencodable, and the encoder spends its first
 * samples slewing and overshooting.
 *
 * ATTACK_SAMPLES of ramp fixes that. At 18512 Hz this is about 1.3 ms -- short
 * enough not to soften any of these transients audibly, long enough for the
 * step index to reach the top. The real ROM's voices face the same constraint
 * and open the same way. */
#define ATTACK_SAMPLES 24

static void apply_attack(double* buf, int len)
{
    int i, n = (len < ATTACK_SAMPLES) ? len : ATTACK_SAMPLES;
    for (i = 0; i < n; i++) buf[i] *= (double)i / (double)n;
}

/* Fit the one envelope parameter so the voice lands on the reference RMS.
 * Same approach as fmgen/gen_rhythm_synth.c; see that file for the rationale. */
typedef void (*voice_fn)(double* out, double dend);

static void fit(voice_fn fn, double* buf, int len,
                double peak, double rms_target, const char* name)
{
    double lo = 1e-9, hi = 0.995, mid = 0.0, got = 0.0;
    int it;
    for (it = 0; it < 60; it++) {
        mid = sqrt(lo * hi);
        fn(buf, mid);
        apply_attack(buf, len);
        got = normalize(buf, len, peak);
        if (got < rms_target) lo = mid; else hi = mid;
    }
    fprintf(stderr, "  %-4s len=%-6d peak=%-6.0f rms=%-6.0f (target %-6.0f, %+.1f%%)\n",
            name, len, peak, got, rms_target, 100.0 * (got - rms_target) / rms_target);
}

/* ------------------------------------------------------------------ *
 * The six voices. Same designs as fmgen/gen_rhythm_synth.c.
 * ------------------------------------------------------------------ */

static void synth_bd(double* o, double dend)
{
    int i; double ph = 0.0; lpf click;
    rng_seed(0x8D10B1u); lpf_set(&click, 2400.0);
    for (i = 0; i < N_BD; i++) {
        double t = (double)i / N_BD;
        double f = 125.0 * exp(-3.1 * t) + 42.0;
        double att = 1.0 - exp(-(double)i / (0.0065 * RATE));
        double body = sin(ph) * decay(i, N_BD, dend) * att;
        double punch = lpf_run(&click, rng_bipolar()) * exp(-38.0 * t) * 0.10;
        ph += 2.0 * M_PI * f / RATE;
        o[i] = body * 0.92 + punch;
    }
}

static void synth_sd(double* o, double dend)
{
    int i; double p1 = 0.0, p2 = 0.0; lpf ntop; hpf nbot;
    rng_seed(0x5EA71Du); lpf_set(&ntop, 3800.0); hpf_set(&nbot, 700.0);
    for (i = 0; i < N_SD; i++) {
        double t = (double)i / N_SD;
        double shell = (sin(p1) * 0.6 + sin(p2) * 0.4) * exp(-13.0 * t);
        double n = hpf_run(&nbot, lpf_run(&ntop, rng_bipolar()));
        p1 += 2.0 * M_PI * 186.0 / RATE;
        p2 += 2.0 * M_PI * 331.0 / RATE;
        o[i] = shell * 0.55 + n * decay(i, N_SD, dend) * 0.85;
    }
}

/* NOTE ON FREQUENCIES. This generator runs at 18512 Hz, where Nyquist is
 * 9256 Hz -- a third of what fmgen/gen_rhythm_synth.c has at 55466 Hz. The
 * oscillator and filter frequencies below are therefore LOWER than that file's,
 * and deliberately so. Content close to Nyquist alternates sign almost every
 * sample, which is precisely what a differential codec cannot track: carrying
 * the 55466 Hz tuning over unchanged put content right against Nyquist, where
 * a signal alternates sign almost every sample. They sit around 5-6 kHz here
 * instead of 6-7.5 kHz.
 *
 * This buys headroom, not accuracy. The residual encode error on the
 * noise-based voices (SD, TOP, HH) is roughly 15% either way: that is ADPCM-A's
 * 4-bit quantization floor on noise, which no amount of retuning removes. The
 * tonal voices (BD, TOM) encode at under 2%, and the error on the noisy ones is
 * itself noise added to noise, so it reads as slight extra grain rather than as
 * a defect.
 */
static double metal(double* ph, const double* f, int n)
{
    int k; double s = 0.0;
    for (k = 0; k < n; k++) {
        s += (sin(ph[k]) >= 0.0) ? 1.0 : -1.0;
        ph[k] += 2.0 * M_PI * f[k] / RATE;
    }
    return s / n;
}

/* Hard strike over a slow body -- see gen_rhythm_synth.c. */
static void synth_top(double* o, double dend)
{
    static const double f[6] = { 1279.0, 1693.0, 2069.0, 2571.0, 3119.0, 3797.0 };
    double ph[6] = {0,0,0,0,0,0}; hpf hp; lpf tone; int i;
    rng_seed(0x70B1C1u); hpf_set(&hp, 1900.0); lpf_set(&tone, 5200.0);
    for (i = 0; i < N_TOP; i++) {
        double t = (double)i / N_TOP;
        double body = lpf_run(&tone, hpf_run(&hp, metal(ph, f, 6) + rng_bipolar() * 0.22));
        o[i] = body * (decay(i, N_TOP, dend) + 3.0 * exp(-130.0 * t));
    }
}

static void synth_hh(double* o, double dend)
{
    static const double f[6] = { 1546.0, 2009.0, 2436.0, 2977.0, 3623.0, 4416.0 };
    double ph[6] = {0,0,0,0,0,0}; hpf hp; int i;
    rng_seed(0x44474Au); hpf_set(&hp, 3500.0);
    for (i = 0; i < N_HH; i++) {
        double t = (double)i / N_HH;
        double m = metal(ph, f, 6) + rng_bipolar() * 0.30;
        double s = tanh(3.2 * hpf_run(&hp, m)) / tanh(3.2);
        o[i] = s * (decay(i, N_HH, dend) + 0.10 * exp(-45.0 * t));
    }
}

static void synth_tom(double* o, double dend)
{
    int i; double ph = 0.0; lpf skin;
    rng_seed(0x70110Du); lpf_set(&skin, 3200.0);
    for (i = 0; i < N_TOM; i++) {
        double t = (double)i / N_TOM;
        double f = 196.0 * exp(-1.9 * t) + 104.0;
        double env = decay(i, N_TOM, dend) + 0.22 * decay(i, N_TOM, 0.30);
        double skn = lpf_run(&skin, rng_bipolar()) * exp(-26.0 * t) * 0.20;
        ph += 2.0 * M_PI * f / RATE;
        o[i] = sin(ph) * env * 0.90 + skn;
    }
}

static void synth_rim(double* o, double dend)
{
    int i; double p1 = 0.0, p2 = 0.0; hpf hp;
    rng_seed(0x71B5A0u); hpf_set(&hp, 400.0);
    for (i = 0; i < N_RIM; i++) {
        double t = (double)i / N_RIM;
        double ring = sin(p1) * 0.55 + sin(p2) * 0.45;
        double clk = rng_bipolar() * exp(-70.0 * t) * 0.5;
        p1 += 2.0 * M_PI * 447.0 / RATE;
        p2 += 2.0 * M_PI * 1450.0 / RATE;
        o[i] = hpf_run(&hp, ring * decay(i, N_RIM, dend) + clk);
    }
}

/* ------------------------------------------------------------------ *
 * ADPCM-A codec.
 *
 * Mirrors ADPCMA_calc_chan() and Init_ADPCMATable() in
 * emu/cores/fmopn.c exactly, including the 12-bit accumulator WRAP (the chip
 * wraps, it does not saturate). The encoder is closed-loop: it runs this same
 * decoder and picks, for each sample, the nibble whose decoded result lands
 * nearest the target. That makes encoder and decoder track in lockstep, so what
 * comes out of libvgm is what was synthesized.
 * ------------------------------------------------------------------ */
static const int steps[49] = {
     16,  17,   19,   21,   23,   25,   28,
     31,  34,   37,   41,   45,   50,   55,
     60,  66,   73,   80,   88,   97,  107,
    118, 130,  143,  157,  173,  190,  209,
    230, 253,  279,  307,  337,  371,  408,
    449, 494,  544,  598,  658,  724,  796,
    876, 963, 1060, 1166, 1282, 1411, 1552
};
static const int step_inc[8] = { -1*16, -1*16, -1*16, -1*16, 2*16, 5*16, 7*16, 9*16 };
static int jedi_table[49*16];

static void init_adpcma_table(void)
{
    int step, nib;
    for (step = 0; step < 49; step++)
        for (nib = 0; nib < 16; nib++) {
            int value = (2*(nib & 0x07) + 1) * steps[step] / 8;
            jedi_table[step*16 + nib] = (nib & 0x08) ? -value : value;
        }
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

/* One decode step, shared by encoder and verifier. */
static int adpcma_step(int* acc, int* stepidx, int nib)
{
    int a = (*acc + jedi_table[*stepidx + nib]) & 0xfff;
    if (a & 0x800) a |= ~0xfff;
    *acc = a;
    *stepidx = clampi(*stepidx + step_inc[nib & 7], 0, 48*16);
    return a;
}

/* Encode `len` samples into `len/2` bytes. Returns how many times the
 * accumulator left 12-bit range, which should be zero. */
static int adpcma_encode(const double* in, int len, unsigned char* out)
{
    int acc = 0, stepidx = 0, i, wraps = 0;
    for (i = 0; i < len; i++) {
        int target = (int)lround(in[i]);
        int best = -1; long bestErr = -1; int nib;
        for (nib = 0; nib < 16; nib++) {
            int raw = acc + jedi_table[stepidx + nib];
            int a, s, got; long err;
            /* Never choose a nibble that would take the accumulator out of
             * 12-bit range. It WRAPS rather than saturating, which in a
             * differential codec displaces every following sample permanently
             * -- that is exactly the +477 DC offset the real cymbal carries. */
            if (raw > 2047 || raw < -2048) continue;
            a = acc; s = stepidx;
            got = adpcma_step(&a, &s, nib);
            err = labs((long)got - target);
            if (bestErr < 0 || err < bestErr) { bestErr = err; best = nib; }
        }
        if (best < 0) {                    /* cannot happen: nibble 0 and 8 are
                                            * the smallest steps either way */
            best = 0; wraps++;
        }
        adpcma_step(&acc, &stepidx, best);
        if (i & 1) out[i >> 1] |= (unsigned char)best;
        else       out[i >> 1]  = (unsigned char)(best << 4);
    }
    return wraps;
}

/* Decode back and report the error, so a bad encode cannot pass unnoticed. */
static void verify(const unsigned char* data, int bytes, const double* ref,
                   const char* name)
{
    int acc = 0, stepidx = 0, i, n = bytes * 2;
    double sum = 0.0, refsum = 0.0, peak = 0.0;
    for (i = 0; i < n; i++) {
        int nib = (i & 1) ? (data[i >> 1] & 0x0f) : ((data[i >> 1] >> 4) & 0x0f);
        int got = adpcma_step(&acc, &stepidx, nib);
        double e = (double)got - ref[i];
        sum += e * e; refsum += ref[i] * ref[i];
        if (abs(got) > peak) peak = abs(got);
    }
    fprintf(stderr, "  %-4s decoded peak=%-6.0f  encode error %.1f%% of signal\n",
            name, peak, 100.0 * sqrt(sum / n) / sqrt(refsum / n));
}

/* ------------------------------------------------------------------ */

struct voice {
    const char*  name;
    voice_fn     fn;
    int          len, start, end;
    double       peak, rms;
    double*      buf;
};

int main(int argc, char** argv)
{
    static double bd[N_BD], sd[N_SD], top[N_TOP], hh[N_HH], tom[N_TOM], rim[N_RIM];
    static unsigned char rom[TOTAL_BYTES];
    /* Byte ranges are libvgm's YM2608_ADPCM_ROM_addr, reproduced exactly. */
    struct voice v[6] = {
        { "bd",  synth_bd,  N_BD,  0x0000, 0x01bf, 2042.0, 723.0, bd  },
        { "sd",  synth_sd,  N_SD,  0x01c0, 0x043f, 1935.0, 339.0, sd  },
        { "top", synth_top, N_TOP, 0x0440, 0x1b7f, 1814.0, 112.0, top },
        { "hh",  synth_hh,  N_HH,  0x1b80, 0x1cff, 1764.0, 497.0, hh  },
        { "tom", synth_tom, N_TOM, 0x1d00, 0x1f7f, 1799.0, 383.0, tom },
        { "rim", synth_rim, N_RIM, 0x1f80, 0x1fff, 1924.0, 396.0, rim },
    };
    FILE* fp;
    int i, totalWraps = 0;

    /* The same ROM is consumed in two places and they declare it differently:
     * libvgm's fmopn_2608rom.h is `static const` (included by one .c file),
     * Furnace's rss.h is plain `const` (included by one .cpp). Pass "nonstatic"
     * for the latter. Furnace uses the identical address layout -- see
     * set_start_end() in its ymfm_opn.cpp -- so one generator serves both. */
    const char* decl = "static const";
    if (argc > 2 && !strcmp(argv[2], "nonstatic")) decl = "const";

    if (argc < 2) {
        fprintf(stderr, "usage: %s <out.h> [nonstatic]\n", argv[0]);
        return 1;
    }
    init_adpcma_table();

    fprintf(stderr, "synthesizing YM2608 ADPCM-A rhythm voices at %d Hz:\n", RATE);
    for (i = 0; i < 6; i++) {
        int bytes = v[i].end - v[i].start + 1;
        if (bytes * 2 != v[i].len) {
            fprintf(stderr, "  %s: length %d does not fill %d bytes\n",
                    v[i].name, v[i].len, bytes);
            return 1;
        }
        fit(v[i].fn, v[i].buf, v[i].len, v[i].peak, v[i].rms, v[i].name);
    }

    fprintf(stderr, "encoding:\n");
    for (i = 0; i < 6; i++) {
        int bytes = v[i].end - v[i].start + 1;
        int w = adpcma_encode(v[i].buf, v[i].len, rom + v[i].start);
        totalWraps += w;
        verify(rom + v[i].start, bytes, v[i].buf, v[i].name);
        if (w) fprintf(stderr, "  %-4s WARNING: %d accumulator wraps\n", v[i].name, w);
    }
    fprintf(stderr, "accumulator wraps: %d (want 0)\n", totalWraps);

    fp = fopen(argv[1], "w");
    if (!fp) { perror(argv[1]); return 1; }
    fprintf(fp,
        "/*\n"
        "    Synthesized YM2608 ADPCM-A rhythm voices.\n"
        "\n"
        "    THIS IS NOT YAMAHA'S RHYTHM ROM. It contains no data from it. The six\n"
        "    voices were generated from scratch by\n"
        "    musicplayer/src/plugins/libvgmplugin/gen_2608rom_synth.c, which also\n"
        "    carries the rationale and the reference measurements. They match the\n"
        "    real voices in count, order, byte layout and level, but not in timbre.\n"
        "\n"
        "    DO NOT EDIT BY HAND - regenerate with that program.\n"
        "*/\n\n"
        "%s unsigned char YM2608_ADPCM_ROM[0x%04X] = {\n", decl, TOTAL_BYTES);
    for (i = 0; i < 6; i++)
        fprintf(fp, "/* %-10s 0x%04X - 0x%04X */\n", v[i].name, v[i].start, v[i].end);
    fputc('\n', fp);
    for (i = 0; i < TOTAL_BYTES; i++) {
        fprintf(fp, "0x%02X,", rom[i]);
        if ((i & 15) == 15) fputc('\n', fp);
    }
    fprintf(fp, "\n};\n");
    fclose(fp);
    fprintf(stderr, "wrote %s\n", argv[1]);
    return totalWraps ? 1 : 0;
}
