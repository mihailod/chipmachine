/*
 * gen_rhythm_synth.c — offline generator (NOT part of the build).
 *
 * Emits opna_rhythm_rom.cpp: the six OPNA hardware-rhythm voices
 * (BD/SD/TOP/HH/TOM/RIM) as 16-bit PCM at 55466 Hz, compiled into the fmgen
 * OPNA so that FMP and S98 playback have working drums with no runtime file.
 *
 * THESE SAMPLES ARE SYNTHESIZED FROM SCRATCH. They are NOT a decode of, and
 * contain no data derived from, Yamaha's YM2608 ADPCM-A rhythm ROM. That ROM
 * is copyrighted firmware and is deliberately not present in this repository.
 * Everything below is ordinary subtractive/additive drum synthesis written for
 * this project.
 *
 * They are therefore NOT bit-accurate to real OPNA rhythm and will not sound
 * identical to it. What is preserved, deliberately, is the part that affects
 * arrangement rather than timbre:
 *
 *   - the six voices and their order (BD, SD, TOP, HH, TOM, RIM), which is
 *     the order OPNA::rhythm[] and the rhythm key-on register 0x10 use;
 *   - each voice's exact length in samples, so note spacing, overlap and
 *     decay tails land where a composer placed them;
 *   - each voice's peak and approximate RMS, so the drum bus sits at the same
 *     level against the FM and SSG parts and no per-song remixing is needed.
 *
 * A user who owns the real ROM can still get authentic drums: fmgen tries an
 * external 2608_BD.WAV .. 2608_RIM.WAV set first (OPNA::LoadRhythmSample) and
 * only falls back to these tables (OPNA::LoadEmbeddedRhythm).
 *
 * Reference levels, measured from real OPNA rhythm, that the shapes below are
 * tuned to hit:
 *
 *      voice  samples      ms   peak     rms
 *      BD        2682    48.4  32608   11536
 *      SD        3834    69.1  30896    5397
 *      TOP      35706   643.7  31904    1744
 *      HH        2298    41.4  26400    7446
 *      TOM       7668   138.2  28784    6139
 *      RIM       1524    27.5  30656    6317
 *
 * Regenerate with:
 *   cc -O2 gen_rhythm_synth.c -lm -o /tmp/gen_rhythm_synth
 *   /tmp/gen_rhythm_synth opna_rhythm_rom.cpp
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define RATE 55466

#define N_BD   2682
#define N_SD   3834
#define N_TOP 35706
#define N_HH   2298
#define N_TOM  7668
#define N_RIM  1524

/* ------------------------------------------------------------------ *
 * Deterministic noise. rand() would make the output depend on libc, and
 * this file is checked in — regenerating it on another machine must
 * produce identical bytes. xorshift32 with a fixed seed per voice.
 * ------------------------------------------------------------------ */
static unsigned int rng_state;
static void  rng_seed(unsigned int s) { rng_state = s ? s : 1u; }
static double rng_bipolar(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return (double)(int)rng_state / 2147483648.0;   /* about -1 .. +1 */
}

/* One-pole low-pass. f is the cutoff in Hz. */
typedef struct { double y, a; } lpf;
static void lpf_set(lpf* f, double hz)
{
    double x = exp(-2.0 * M_PI * hz / RATE);
    f->a = 1.0 - x;
    f->y = 0.0;
}
static double lpf_run(lpf* f, double in) { f->y += f->a * (in - f->y); return f->y; }

/* One-pole high-pass, built as input minus its low-passed part. */
typedef struct { lpf lp; } hpf;
static void hpf_set(hpf* f, double hz) { lpf_set(&f->lp, hz); }
static double hpf_run(hpf* f, double in) { return in - lpf_run(&f->lp, in); }

/* Exponential decay to `floor_at_end` of the initial value over `len`. */
static double decay(int i, int len, double end)
{
    return exp(log(end) * (double)i / (double)len);
}

/* Scale a voice to a target peak and return the RMS that resulted. */
static double normalize(double* buf, int len, double target_peak)
{
    int i;
    double peak = 0.0, sum = 0.0;
    for (i = 0; i < len; i++)
        if (fabs(buf[i]) > peak) peak = fabs(buf[i]);
    if (peak < 1e-12) peak = 1.0;
    for (i = 0; i < len; i++) buf[i] *= target_peak / peak;
    for (i = 0; i < len; i++) sum += buf[i] * buf[i];
    return sqrt(sum / len);
}

/* ------------------------------------------------------------------ *
 * Envelope fitting.
 *
 * Each voice is written as a function of one parameter, `dend`: the
 * fraction of its initial amplitude the main envelope has fallen to by the
 * end of the voice. That parameter alone controls how much of the voice's
 * length carries energy, so after peak normalization it maps monotonically
 * onto RMS -- small dend gives a spike that dies immediately (low RMS),
 * large dend gives a voice that stays loud throughout (high RMS).
 *
 * So rather than hand-tuning six envelopes until they happen to sit at the
 * right level, bisect on dend until each voice's RMS matches the measured
 * reference. Timbre comes from the oscillator and filter design above;
 * only the amplitude contour is fitted.
 * ------------------------------------------------------------------ */
typedef void (*voice_fn)(double* out, double dend);

static void fit(voice_fn fn, double* buf, int len,
                double peak, double rms_target, const char* name)
{
    double lo = 1e-9, hi = 0.995, mid = 0.0, got = 0.0;
    int it;
    for (it = 0; it < 60; it++) {
        mid = sqrt(lo * hi);              /* geometric: dend spans decades */
        fn(buf, mid);
        got = normalize(buf, len, peak);
        if (got < rms_target) lo = mid; else hi = mid;
    }
    fprintf(stderr, "  %-4s len=%-6d peak=%-6.0f rms=%-6.0f (target %-6.0f, %+.1f%%)  dend=%.3g\n",
            name, len, peak, got, rms_target, 100.0 * (got - rms_target) / rms_target, mid);
}

/* ------------------------------------------------------------------ *
 * The six voices.
 * ------------------------------------------------------------------ */

/* Bass drum: sine with a fast downward pitch sweep, plus a short click at
 * the attack. High RMS relative to peak -- it stays loud for most of its
 * 48 ms rather than spiking and vanishing.
 *
 * The real voice swells rather than starting at full amplitude: its loudest
 * eighth is the second one, not the first. A ~6 ms attack ramp reproduces
 * that, and without it the fitter has to compensate with an unnaturally fast
 * decay to keep the total energy right. */
static void synth_bd(double* o, double dend)
{
    int i;
    double ph = 0.0;
    lpf click;
    rng_seed(0x8D10B1u);
    lpf_set(&click, 3000.0);
    for (i = 0; i < N_BD; i++) {
        double t     = (double)i / N_BD;
        double f     = 125.0 * exp(-3.1 * t) + 42.0;         /* 167 Hz -> 42 Hz */
        double att   = 1.0 - exp(-(double)i / (0.0065 * RATE));
        double env   = decay(i, N_BD, dend) * att;
        double body  = sin(ph) * env;
        double punch = lpf_run(&click, rng_bipolar()) * exp(-38.0 * t) * 0.10;
        ph += 2.0 * M_PI * f / RATE;
        o[i] = body * 0.92 + punch;
    }
}

/* Snare: two detuned shell tones under a band-limited noise burst. Noise
 * dominates, so RMS sits well below the peak. */
static void synth_sd(double* o, double dend)
{
    int i;
    double p1 = 0.0, p2 = 0.0;
    lpf ntop;
    hpf nbot;
    rng_seed(0x5EA71Du);
    lpf_set(&ntop, 7200.0);
    hpf_set(&nbot, 900.0);
    for (i = 0; i < N_SD; i++) {
        double t     = (double)i / N_SD;
        double shell = (sin(p1) * 0.6 + sin(p2) * 0.4) * exp(-13.0 * t);
        double n     = hpf_run(&nbot, lpf_run(&ntop, rng_bipolar()));
        double nenv  = decay(i, N_SD, dend);
        p1 += 2.0 * M_PI * 186.0 / RATE;
        p2 += 2.0 * M_PI * 331.0 / RATE;
        o[i] = shell * 0.55 + n * nenv * 0.85;
    }
}

/* Metallic source shared by cymbal and hi-hat: six inharmonic square
 * oscillators, the classic analog metal cluster. Frequencies are mutually
 * irrational-ish so the sum never repeats over the voice's length. */
static double metal(double* ph, const double* f, int n)
{
    int k;
    double s = 0.0;
    for (k = 0; k < n; k++) {
        s += (sin(ph[k]) >= 0.0) ? 1.0 : -1.0;
        ph[k] += 2.0 * M_PI * f[k] / RATE;
    }
    return s / n;
}

/* Top cymbal: a hard strike followed by a long shimmer over 644 ms.
 *
 * Measured from the real voice, the strike is about 4x the level of the body
 * that follows it -- peak 31904 in the first 54 ms, then a sustained peak
 * near 7600 decaying to ~800 over the remaining 590 ms. A single exponential
 * cannot be both, so the envelope is a short 5 ms spike sitting on top of a
 * slow body decay, and it is the body that the fitter tunes. */
static void synth_top(double* o, double dend)
{
    static const double f[6] = { 2158.0, 2857.0, 3491.0, 4337.0, 5261.0, 6407.0 };
    double ph[6] = { 0, 0, 0, 0, 0, 0 };
    hpf hp;
    lpf tone;
    int i;
    rng_seed(0x70B1C1u);
    hpf_set(&hp, 3400.0);
    lpf_set(&tone, 11000.0);
    for (i = 0; i < N_TOP; i++) {
        double t    = (double)i / N_TOP;
        double m    = metal(ph, f, 6);
        double air  = rng_bipolar() * 0.22;
        double body = lpf_run(&tone, hpf_run(&hp, m + air));
        double env  = decay(i, N_TOP, dend) + 3.0 * exp(-130.0 * t);
        o[i] = body * env;
    }
}

/* Closed hi-hat: same metal cluster, harder high-pass, gone in 41 ms.
 *
 * The real voice is dense -- crest factor only about 2.2 (peak 26400 against
 * an early RMS near 12000), i.e. much closer to a square than to a spiky
 * oscillator sum. Saturating the source before the envelope reproduces that
 * density; without it the voice cannot reach the reference RMS at the
 * reference peak no matter how the envelope is shaped. */
static void synth_hh(double* o, double dend)
{
    static const double f[6] = { 2609.0, 3391.0, 4111.0, 5023.0, 6113.0, 7451.0 };
    double ph[6] = { 0, 0, 0, 0, 0, 0 };
    hpf hp;
    int i;
    rng_seed(0x44474Au);
    hpf_set(&hp, 6200.0);
    for (i = 0; i < N_HH; i++) {
        double t = (double)i / N_HH;
        double m = metal(ph, f, 6) + rng_bipolar() * 0.30;
        double s = tanh(3.2 * hpf_run(&hp, m)) / tanh(3.2);
        o[i] = s * (decay(i, N_HH, dend) + 0.10 * exp(-45.0 * t));
    }
}

/* Tom: like the bass drum but higher, slower sweep and a longer tail.
 *
 * A single exponential cannot hold the reference shape here -- the real voice
 * still carries a sixth of its opening level in its last eighth, 138 ms in,
 * which a one-stage decay can only match by being too quiet in the middle.
 * So the envelope is a fitted fast decay plus a fixed slow tail, the same
 * two-stage idea used for the cymbal but with the weighting reversed. */
static void synth_tom(double* o, double dend)
{
    int i;
    double ph = 0.0;
    lpf skin;
    rng_seed(0x70110Du);
    lpf_set(&skin, 5000.0);
    for (i = 0; i < N_TOM; i++) {
        double t    = (double)i / N_TOM;
        double f    = 196.0 * exp(-1.9 * t) + 104.0;         /* 300 Hz -> 104 Hz */
        double env  = decay(i, N_TOM, dend) + 0.22 * decay(i, N_TOM, 0.30);
        double body = sin(ph) * env;
        double skn  = lpf_run(&skin, rng_bipolar()) * exp(-26.0 * t) * 0.20;
        ph += 2.0 * M_PI * f / RATE;
        o[i] = body * 0.90 + skn;
    }
}

/* Rim shot: 27 ms, almost all transient — a hard click with a short
 * resonant ring above it. */
static void synth_rim(double* o, double dend)
{
    int i;
    double p1 = 0.0, p2 = 0.0;
    hpf hp;
    rng_seed(0x71B5A0u);
    hpf_set(&hp, 400.0);
    for (i = 0; i < N_RIM; i++) {
        double t    = (double)i / N_RIM;
        double ring = sin(p1) * 0.55 + sin(p2) * 0.45;
        double clk  = rng_bipolar() * exp(-70.0 * t) * 0.5;
        p1 += 2.0 * M_PI * 447.0 / RATE;
        p2 += 2.0 * M_PI * 1712.0 / RATE;
        o[i] = hpf_run(&hp, ring * decay(i, N_RIM, dend) + clk);
    }
}

/* ------------------------------------------------------------------ */

static void emit(FILE* fp, const char* name, const double* buf, int len)
{
    int i;
    fprintf(fp, "static const short opna_rhythm_pcm_%s[%d] = {\n", name, len);
    for (i = 0; i < len; i++) {
        long v = lround(buf[i]);
        if (v >  32767) v =  32767;
        if (v < -32768) v = -32768;
        fprintf(fp, "%ld,", v);
        if ((i & 15) == 15) fputc('\n', fp);
    }
    if (len & 15) fputc('\n', fp);
    fprintf(fp, "};\n\n");
}

int main(int argc, char** argv)
{
    static double bd[N_BD], sd[N_SD], top[N_TOP], hh[N_HH], tom[N_TOM], rim[N_RIM];
    FILE* fp;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <out.cpp>\n", argv[0]);
        return 1;
    }

    fprintf(stderr, "synthesizing OPNA rhythm voices at %d Hz:\n", RATE);
    fit(synth_bd,  bd,  N_BD,  32608.0, 11536.0, "bd");
    fit(synth_sd,  sd,  N_SD,  30896.0,  5397.0, "sd");
    fit(synth_top, top, N_TOP, 31904.0,  1744.0, "top");
    fit(synth_hh,  hh,  N_HH,  26400.0,  7446.0, "hh");
    fit(synth_tom, tom, N_TOM, 28784.0,  6139.0, "tom");
    fit(synth_rim, rim, N_RIM, 30656.0,  6317.0, "rim");

    fp = fopen(argv[1], "w");
    if (!fp) { perror(argv[1]); return 1; }

    fprintf(fp,
        "// Auto-generated by gen_rhythm_synth.c. DO NOT EDIT BY HAND.\n"
        "// Synthesized 16-bit PCM for the 6 OPNA rhythm voices, native rate %d Hz.\n"
        "//\n"
        "// These are NOT a decode of Yamaha's YM2608 ADPCM-A rhythm ROM and contain\n"
        "// no data derived from it. They are drum sounds synthesized from scratch for\n"
        "// this project, matched to the real voices only in count, order, length and\n"
        "// level -- not in timbre. See gen_rhythm_synth.c for the full rationale.\n"
        "//\n"
        "// A user with the real ROM can still get authentic drums by supplying an\n"
        "// external 2608_BD.WAV .. 2608_RIM.WAV set; OPNA::LoadRhythmSample is tried\n"
        "// first and these tables are only the fallback.\n\n", RATE);

    emit(fp, "bd",  bd,  N_BD);
    emit(fp, "sd",  sd,  N_SD);
    emit(fp, "top", top, N_TOP);
    emit(fp, "hh",  hh,  N_HH);
    emit(fp, "tom", tom, N_TOM);
    emit(fp, "rim", rim, N_RIM);

    fprintf(fp,
        "namespace FM {\n"
        "extern const int opna_rhythm_present = 1;\n"
        "extern const int opna_rhythm_rate = %d;\n"
        "extern const int opna_rhythm_len[6] = {%d,%d,%d,%d,%d,%d};\n"
        "extern const short* const opna_rhythm_pcm[6] = {\n"
        "    opna_rhythm_pcm_bd, opna_rhythm_pcm_sd, opna_rhythm_pcm_top,\n"
        "    opna_rhythm_pcm_hh, opna_rhythm_pcm_tom, opna_rhythm_pcm_rim\n"
        "};\n"
        "} // namespace FM\n",
        RATE, N_BD, N_SD, N_TOP, N_HH, N_TOM, N_RIM);

    fclose(fp);
    fprintf(stderr, "wrote %s\n", argv[1]);
    return 0;
}
