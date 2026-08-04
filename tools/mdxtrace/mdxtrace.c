/*
  mdxtrace -- dump the register/event trace an MDX driver produces.

  This is a DEVELOPMENT ORACLE, not shipping code. It links the GPL-2 mdxmini
  and exists so that a future non-GPL MDX engine can be proved equivalent by
  diffing traces rather than by ear. Running GPL code as a test oracle is fine;
  it is never distributed. See README.md.

  Usage: mdxtrace [options] <file.mdx>

    -f N     stop after N sequencer frames (default 3600)
    -r HZ    sample rate (default 44100)
    -c N     render block size in frames (default rate/100); the trace must
             not depend on this -- build.sh --check verifies it
    -d DIR   directory to search for the .pdx sample bank (default: the
             .mdx file's own directory)
    -o FILE  write trace to FILE (default stdout)
    -q       omit the header comment block (for byte-exact self-diffs)

  Output is line-oriented text, one event per line, in emission order:

    F <frame> <tempo>          sequencer tick boundary
    O <adr> <val>              YM2151 register write (hex, 2 digits each)
    P on <ch> <hash> <bytes>   PCM8 voice start; hash identifies the sample
    P tie <ch>                 PCM8 note-on absorbed as a tie (no restart)
    P off <ch>                 PCM8 voice stop
    P freq <ch> <hz>           PCM8 playback rate
    P vol <ch> <val>           PCM8 per-channel volume
    P mvol <val>               PCM8 master volume
    P pan <val>                PCM8 master pan

  Samples are identified by a content hash rather than a pointer because
  pointers vary between runs under ASLR and would defeat diffing.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "mdxmini.h"

/* ------------------------------------------------------------------ */
/* trace sink */

static FILE *out;
static long  frame_no = -1;   /* -1 until the first frame boundary */
static long  event_count;

/* Rendering happens in chunks, so the last chunk can carry the sequencer past
   the requested frame count -- by a chunk-dependent amount. Everything after
   the limit is suppressed so a trace is a pure function of (file, frames),
   independent of block size. */
static long trace_limit = -1;
static int  trace_stopped;

/* FNV-1a over at most CAP bytes -- enough to identify a sample, cheap enough
   to run on every note-on. */
#define HASH_CAP 4096

static unsigned int sample_hash(const void *p, int nbytes)
{
    const unsigned char *b = (const unsigned char *)p;
    unsigned int h = 2166136261u;
    int n = nbytes < HASH_CAP ? nbytes : HASH_CAP;
    int i;

    if (!b || n <= 0)
        return 0;

    for (i = 0; i < n; i++) {
        h ^= b[i];
        h *= 16777619u;
    }
    return h;
}

void mdxtrace_frame(int tempo)
{
    frame_no++;
    if (trace_limit >= 0 && frame_no > trace_limit) {
        trace_stopped = 1;
        return;
    }
    fprintf(out, "F %ld %d\n", frame_no, tempo);
    event_count++;
}

void mdxtrace_opm(int adr, int val)
{
    if (trace_stopped)
        return;
    fprintf(out, "O %02x %02x\n", adr & 0xff, val & 0xff);
    event_count++;
}

void mdxtrace_pcm_on(int ch, const void *data, int nbytes)
{
    if (trace_stopped)
        return;
    fprintf(out, "P on %d %08x %d\n", ch, sample_hash(data, nbytes), nbytes);
    event_count++;
}

void mdxtrace_pcm_tie(int ch)  { if (!trace_stopped) { fprintf(out, "P tie %d\n", ch); event_count++; } }
void mdxtrace_pcm_off(int ch)  { if (!trace_stopped) { fprintf(out, "P off %d\n", ch); event_count++; } }

void mdxtrace_pcm_freq(int ch, int hz)  { if (!trace_stopped) { fprintf(out, "P freq %d %d\n", ch, hz); event_count++; } }
void mdxtrace_pcm_vol(int ch, int val)  { if (!trace_stopped) { fprintf(out, "P vol %d %d\n", ch, val); event_count++; } }
void mdxtrace_pcm_mvol(int val)         { if (!trace_stopped) { fprintf(out, "P mvol %d\n", val);       event_count++; } }
void mdxtrace_pcm_pan(int val)          { if (!trace_stopped) { fprintf(out, "P pan %d\n", val);        event_count++; } }

/* ------------------------------------------------------------------ */

static void usage(void)
{
    fprintf(stderr,
        "usage: mdxtrace [-f frames] [-r rate] [-d pdxdir] [-o out] [-q] <file.mdx>\n");
    exit(2);
}

int main(int argc, char **argv)
{
    const char *path    = NULL;
    const char *outpath = NULL;
    const char *pdxdir  = NULL;
    int  max_frames = 3600;
    int  rate       = 44100;
    int  quiet      = 0;
    int  i;

    t_mdxmini song;
    char dirbuf[2048];
    short *buf;
    int  chunk = 0;          /* 0 = derive from rate */
    int  stalled = 0;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-f") && i + 1 < argc)      max_frames = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-r") && i + 1 < argc) rate    = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-c") && i + 1 < argc) chunk   = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-d") && i + 1 < argc) pdxdir  = argv[++i];
        else if (!strcmp(argv[i], "-o") && i + 1 < argc) outpath = argv[++i];
        else if (!strcmp(argv[i], "-q"))                 quiet   = 1;
        else if (argv[i][0] == '-')                      usage();
        else if (!path)                                  path    = argv[i];
        else                                             usage();
    }

    if (!path)
        usage();

    if (outpath) {
        out = fopen(outpath, "w");
        if (!out) {
            fprintf(stderr, "mdxtrace: cannot write %s\n", outpath);
            return 1;
        }
    } else {
        out = stdout;
    }

    if (!pdxdir) {
        snprintf(dirbuf, sizeof dirbuf, "%s", path);
        pdxdir = dirname(dirbuf);
    }

    /* The header must be written BEFORE mdx_open: driver initialisation
       already writes YM2151 registers, and those writes are part of the
       trace. */
    if (!quiet) {
        fprintf(out, "# mdxtrace 1\n");
        fprintf(out, "# engine mdxmini\n");
        fprintf(out, "# rate %d\n", rate);
        fprintf(out, "# frames %d\n", max_frames);
    }

    trace_limit = max_frames;

    memset(&song, 0, sizeof song);
    mdx_set_rate(rate);

    if (mdx_open(&song, path, pdxdir)) {
        fprintf(stderr, "mdxtrace: cannot open %s\n", path);
        return 1;
    }

    /* Render in small chunks so the frame hook fires at its natural rate; the
       audio itself is discarded -- only the event stream matters. Chunk size
       must not change the trace, and is checked by build.sh's chunk-invariance
       test. */
    if (chunk <= 0)
        chunk = rate / 100;
    if (chunk < 1)
        chunk = 1;

    buf = (short *)calloc((size_t)chunk * 2, sizeof(short));
    if (!buf) {
        fprintf(stderr, "mdxtrace: out of memory\n");
        return 1;
    }

    while (!trace_stopped) {
        long before = frame_no;

        mdx_calc_sample(&song, buf, chunk);

        /* A tune that has ended stops advancing frames; don't spin forever. */
        if (frame_no == before && frame_no >= 0) {
            if (++stalled > 1000)
                break;
        } else {
            stalled = 0;
        }
    }

    free(buf);
    mdx_close(&song);

    if (!quiet)
        fprintf(out, "# events %ld\n", event_count);

    if (out != stdout)
        fclose(out);

    return 0;
}
