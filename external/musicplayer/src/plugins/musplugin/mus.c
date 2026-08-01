// Compute!'s Sidplayer (.mus / .str) sequencer -- CLEAN ROOM.
//
// Written from two independent, freely-distributed format write-ups shipped in
// the Compute's Gazette SID Collection (CGSC/00_Documents/):
//   * MUS_format_A.txt -- Peter Weighill, which states outright that it was
//     produced by hex-editing files made with a MUSic editor, "without any
//     reference to the book written by Craig Chamberlin".
//   * MUS_format_B.txt -- Dick Thornton, giving the bit-level layouts.
// No code was consulted from libsidplayfp / sidplayfp / JSIDPlay2 (all GPL) or
// from any disassembly of the original 1980s Sidplayer routine.
//
// A .mus is note-sequence DATA, not machine code, so there is no 6502 driver
// here. We decode the documented two-byte command stream, keep per-voice state,
// and write SID registers once per frame; the chip emulation is cSID's.

#include "mus.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Note/pitch
// ---------------------------------------------------------------------------
// MUS_format_B, "BYTE 2 - FREQUENCY (PITCH) AND RESTS":
//   bits 7-6 accidental, bits 5-3 octave (111=octave 0 .. 000=octave 7),
//   bits 2-0 note (000=Rest, 001=C, 010=D, 011=E, 100=F, 101=G, 110=A, 111=B).
// Constants MEASURED against VICE's own SID register stream (see the plugin
// README): the note table is NTSC-derived and TRUNCATED, note lengths are
// EXACTLY the raw formula (the tick itself is slow, see MUS_TICK_IRQ_CYCLES),
// and the default tempo byte is 144.
#define MUS_NTSC_CLK        1022727.0
// 144, i.e. 14400/144 = 100 quarter notes per minute. Measured, not assumed:
// most tunes issue a TEM before their first timed event so the default never
// shows, but "Africa" (DC Starr) opens with two quarter rests and only then sets
// the tempo, and VICE gives those rests 72 ticks -- 144/4 each. 128 would make
// them 64 and put the whole tune 8 ticks early forever.
#define MUS_DEFAULT_TEMPO   144

// Per-tick IRQ overhead, in CPU cycles. The player's tick does NOT arrive every
// `latch` cycles: measuring the EFFECTIVE period (total span / tick count) in
// VICE's register stream gives a constant ~580 cycles more than the programmed
// latch, on every tune regardless of JIF:
//     no JIF (16467) -> 17048   (+581)
//     JIF +75 (21267) -> 21817  (+550)
//     JIF -72 (11859) -> 12460  (+601)
// An earlier round mistook this for a 1.049 multiplier on NOTE LENGTH, which had
// to be re-fitted per tune (1.027 for one, 1.049 for another) because it was
// modelling a per-TICK cost as a per-NOTE one. Note lengths are exactly the
// formula; the tick is simply slower than its latch.
#define MUS_TICK_IRQ_CYCLES 580

// The player does not sequence on its first IRQ. Measured against VICE, the
// first note-on of every tune lands exactly 2 ticks after ours did: the real
// player spends those on its init pass (power-on volume 15 -> 8, filter regs
// cleared) before touching the score. Two ticks is only 36ms of absolute
// offset, but a VDP/VRT vibrato moves EVERY tick, so being two ticks early
// leaves the shape perfect and the phase wrong -- "La Donna e Mobile" matched
// VICE's vibrato sample for sample and still scored 30% on the low frequency
// byte.
#define MUS_START_TICKS 2

static const int kSemitone[8] = { 0, 0, 2, 4, 5, 7, 9, 11 }; // index by note bits

// Equal temperament from C0 = 16.3516 Hz (so A4 lands on 440 Hz), converted to a
// SID frequency register value: reg = Hz * 2^24 / clock.
//
// The clock here is the NTSC C64's, NOT the PAL clock the emulation runs at.
// Compute! was a US publication and the player ships a fixed NTSC-derived note
// table, so on a PAL machine every note comes out slightly flat -- authentic
// behaviour, and exactly what tracing VICE shows: its register values are ours
// divided by 1022727/985248 = 1.0380, a constant ratio across every note.
static int sid_freq_for(int octave, int semitone_in_octave, int cents_detune)
{
    double semi = octave * 12.0 + semitone_in_octave + cents_detune / 100.0;
    double hz = 16.3515978313 * pow(2.0, semi / 12.0);
    double reg = hz * 16777216.0 / MUS_NTSC_CLK;
    if (reg < 0) { reg = 0; }
    if (reg > 65535.0) { reg = 65535.0; }
    // TRUNCATE, do not round. Reconstructing the player's table from VICE's
    // note-on registers over 23 traced tunes (46 distinct notes, semitone 23-74)
    // and testing all three roundings: floor reproduces 44/46 entries exactly
    // (96%), rounding only 16/46 (35%), ceil none. e.g. A3 = 3608.99 -> 3608,
    // middle C = 4291.99 -> 4291. The two stragglers are detuned/vibrato'd
    // samples that survived filtering, not counter-evidence.
    return (int)reg;
}

// ---------------------------------------------------------------------------
// Voice state
// ---------------------------------------------------------------------------
typedef struct
{
    const unsigned char* d;
    int len;
    int pos;
    int halted;

    double wait;     // ticks left in the current note/rest (FRACTIONAL)
    double gate_off; // ticks before the gate is released (0 = already off)
    int tied;      // current note ties into the next -> do not release

    int wave;      // SID control-register waveform bits (0x10..0x80)
    int gate;      // current gate bit
    int ring, sync;
    int atk, dcy, sus, rls;
    int pw;        // 12-bit pulse width, as swept
    int pw_base;   // the value P-W programmed; the sweep restarts here each note
    int freq;      // current SID frequency register value
    int transpose; // half-steps
    int detune;    // raw DTN units, -2048..2047
    int filt_through;
    int just_started; // note began THIS tick: modulation holds for one tick
    int resting;      // current event is a REST: modulation is frozen
    int utl;          // UTL: explicit length in TICKS for a code-1 event
    int aut;         // AUT: filter-auto offset
    int aut_set;     // ...and whether the tune enabled it
    int rls_point;   // PNT: when in the note the gate is released
    int hold_time;   // HLD

    // Continuous modulation, applied once per frame (see apply_modulation).
    int pw_sweep;   // P-S: signed step added to the pulse width every frame
    int vib_depth;  // VDP: frequency vibrato depth
    int vib_rate;   // VRT: frequency vibrato rate
    int pvib_depth; // PVD: pulse-width vibrato depth
    int pvib_rate;  // PVR: pulse-width vibrato rate
    int vib_pos, vib_dir;   // triangle counter, in steps of vib_depth
    int pvib_pos, pvib_dir;
    int base_freq;  // pitch before vibrato, so vibrato does not accumulate
    int por_rate;   // POR: glide speed in frequency units per tick, 0 = off
    int por_target; // pitch we are gliding TOWARDS (== base_freq once arrived)

    // REPEAT: HEAD n ... TAIL -- HEAD stores the count and the loop point.
    int rep_count;
    int rep_pos;

    // PHRASE: DEFINE n ... END, replayed later by CALL n. The TABLE is on
    // MusPlayer and shared by every voice of the file -- see there.
    int defining;      // phrase being defined, or -1
    int call_return;   // stream position to resume after a CALL, or -1
    const unsigned char* call_d;   // and the stream it belonged to
    int call_len;
} MusVoice;

struct MusPlayer
{
    unsigned char* data;
    unsigned char* strdata;   // .str allocation BASE (voices point into it)
    int size;

    MusVoice v[6];   // 0-2 = .mus voices 1-3, 3-5 = .str voices 4-6
    int voices;      // 3 or 6

    int tempo;       // TEM byte, 0 means 256 (shared: one musical clock)
    // Volume and the filter are PER CHIP. A Stereo Sidplayer .str drives a
    // second SID with its own $D418/$D415-17, and VICE keeps the two sets
    // independent -- sharing one set let the .str's commands stomp the .mus's
    // (and vice versa), which showed up as voices on the second chip going
    // silent mid-tune.
    int volume[2];      // 0-15
    int filt_mode[2];   // SID filter mode bits (0x10 low / 0x20 band / 0x40 high)
    int filt_cutoff[2]; // 0-255
    int filt_res[2];    // 0-15
    int filt_sweep[2];  // F-S: signed RATE -- one cutoff step per |F-S| ticks
    int filt_sweep_cnt[2];
    int filt_cutoff_set[2];
    // PHRASE table, SHARED BY ALL VOICES of a file (one set per chip: the .mus
    // and the .str are separate files with their own numbering).
    //
    // Phrases are NOT per-voice. Africa's .str has voice 1 DEFINE phrase 15 --
    // the ATK/DCY/SUS/RLS/WAV/P-W setup block -- and voices 2 and 3 CALL 15 to
    // inherit it. With a per-voice table those calls found nothing, so two of
    // the three voices on the second SID never got an envelope or timbre at all.
    // A phrase is (stream, offset): `pos` alone is an offset into ONE voice's
    // buffer, so a voice calling a phrase defined by a DIFFERENT voice must
    // switch to that voice's stream as well, then switch back on END.
    const unsigned char* phrase_d[2][24];
    int phrase_len[2][24];
    int phrase_start[2][24];

    int start_delay; // idle ticks before sequencing begins, see MUS_START_TICKS
    long long rendered; // frames emitted so far (drives the offline trace clock)
    int jiffy;       // JIF: signed adjustment to the player's tick period
    double tick_scale; // ticks-per-second / 60, see mus_retime()

    double frame_acc;      // fractional frames carried between render calls
    double samples_per_frame;
    int samplerate;
    int finished;
    char title[6][33];
    int title_lines;
};

// ---------------------------------------------------------------------------
// Header
// ---------------------------------------------------------------------------
// MUS_format_B: three 2-byte little-endian voice lengths, then the voice data
// back to back, then the text. MUS_format_A additionally describes a leading
// 2-byte PRG load address, which is present in files kept as raw PRG dumps and
// absent in others -- so try both and keep whichever makes the lengths add up.

// Voices 0-2 are the .mus (chip 0); voices 3-5 are the .str (chip 1).
#define MUS_CHIP_OF(voice_index) ((voice_index) >= 3 ? 1 : 0)

static void mus_retime(MusPlayer* p);

static int parse_header(const unsigned char* d, int size, int* off,
                        int lens[3])
{
    // A CGSC .mus is a C64 PRG, so the first two bytes are a LOAD ADDRESS and
    // the voice-length table starts at 2. Try that FIRST and fall back to 0 only
    // for images saved without one. The other order silently mis-parses any file
    // whose load address happens to look like a plausible length: "Muppet Show B"
    // loads at $0046, and 70/664/528 fits inside the file, so the whole voice
    // table was read two bytes early and the tune started 97 ticks too soon.
    static const int kBases[2] = { 2, 0 };
    for (int bi = 0; bi < 2; bi++) {
        int base = kBases[bi];
        if (size < base + 6) { continue; }
        int l0 = d[base] | (d[base + 1] << 8);
        int l1 = d[base + 2] | (d[base + 3] << 8);
        int l2 = d[base + 4] | (d[base + 5] << 8);
        long total = (long)base + 6 + l0 + l1 + l2;
        // Every voice ends in HLT (01 4F) so a real voice is >= 2 bytes, and the
        // three blocks must fit inside the file with room for the text.
        if (l0 < 2 || l1 < 2 || l2 < 2) { continue; }
        if (total > size) { continue; }
        lens[0] = l0;
        lens[1] = l1;
        lens[2] = l2;
        *off = base + 6;
        return 1;
    }
    return 0;
}

static void voice_init(MusVoice* v, const unsigned char* d, int len)
{
    memset(v, 0, sizeof(*v));
    v->d = d;
    v->len = len;
    // PULSE, not sawtooth. A tune need not issue any WAV command at all, and
    // VICE's first waveform write is 0x40 (pulse) on all 33 traced tunes --
    // unanimous. The old 0x20 guess made every WAV-less tune play the wrong
    // timbre, which is audible as unbalanced voice levels rather than as an
    // obviously wrong sound. Pulse width defaults to 2048 (50%), which VICE
    // also writes (24/24 tunes).
    v->wave = 0x40;
    v->atk = 2;   // AD = 0x20 and SR = 0xF5 are what VICE writes before any
    v->dcy = 0;   // tune command runs -- verified identical on linus and
    v->sus = 15;  // raistlin. The previous 0/9/9/0 guess was simply wrong.
    v->rls = 5;
    v->pw = v->pw_base = 2048;
    v->defining = -1;
    v->call_return = -1;
    v->rep_pos = -1;
    v->resting = 1;
    // The vibrato counter climbs from 0 to +VRT, reverses to -VRT and back, so
    // its period is 4*VRT ticks. It is initialised ONCE and then FREE-RUNS: it
    // is NOT re-phased at note-on. Traced on "Africa" (DC Starr, VDP=4 VRT=1),
    // whose successive note-ons land on base+4, base, base-4, base -- a counter
    // that never restarts. Resetting it pinned every note-on to base+4.
    v->vib_pos = 0;
    v->vib_dir = 1;
    v->pvib_pos = 0;
    v->pvib_dir = 1;
}

// ---------------------------------------------------------------------------
// Register-trace hook. Compiled OUT of the shipping plugin; the offline
// prototype builds with -DMUS_TRACE_HOOKS so its SID writes can be diffed
// against a VICE oracle trace. Kept HERE, in the shipped file, so there is
// only ever one copy of the sequencer to reason about.
// ---------------------------------------------------------------------------
#ifdef MUS_TRACE_HOOKS
#include <stdio.h>
static FILE* mus_trace_fp = NULL;
static double mus_trace_t = 0.0;
void mus_set_reg_trace(const char* path) { mus_trace_fp = fopen(path, "w"); }
void mus_trace_set_time(double t) { mus_trace_t = t; }
static FILE* mus_ev_fp = NULL;
void mus_set_event_trace(const char* path) { mus_ev_fp = fopen(path, "w"); }
static void mus_ev(int vi, int pos, unsigned char b1, unsigned char b2,
                   double fr)
{
    if (mus_ev_fp) {
        fprintf(mus_ev_fp, "%.6f v%d +%d %02x %02x %.3f\n", mus_trace_t, vi, pos,
                b1, b2, fr);
    }
}
#define csid_poke(a, v)                                                        \
    do {                                                                       \
        if (mus_trace_fp) {                                                    \
            fprintf(mus_trace_fp, "%.6f %02x %02x\n", mus_trace_t,             \
                    (unsigned)(a) & 0xFFu, (unsigned)(v) & 0xFFu);             \
        }                                                                      \
        csid_poke((a), (v));                                                   \
    } while (0)
#endif

MusPlayer* mus_create(const unsigned char* mus, int mus_size,
                      const unsigned char* str, int str_size, int samplerate)
{
    int off, lens[3];
    if (!parse_header(mus, mus_size, &off, lens)) { return NULL; }

    MusPlayer* p = (MusPlayer*)calloc(1, sizeof(MusPlayer));
    if (!p) { return NULL; }
    p->data = (unsigned char*)malloc(mus_size);
    memcpy(p->data, mus, mus_size);
    p->size = mus_size;

    // -1, NOT 0: a CALL to a phrase that was never DEFINEd must be ignored, not
    // jump to offset 0 and replay the entire voice.
    for (int c = 0; c < 2; c++) {
        for (int i = 0; i < 24; i++) {
            p->phrase_start[c][i] = -1;
            p->phrase_d[c][i] = NULL;
            p->phrase_len[c][i] = 0;
        }
    }
    int o = off;
    for (int i = 0; i < 3; i++) {
        voice_init(&p->v[i], p->data + o, lens[i]);
        o += lens[i];
    }
    p->voices = 3;

    // Text lines follow voice 3 (PETSCII, up to 5 lines).
    for (int line = 0; line < 5 && o < mus_size; line++) {
        int n = 0;
        while (o < mus_size && n < 32) {
            unsigned char c = p->data[o++];
            if (c == 0x0D || c == 0x00) { break; }
            // PETSCII upper-case maps onto ASCII letters for our purposes.
            if (c >= 0x41 && c <= 0x5A) { c = (unsigned char)(c + 32); }
            else if (c >= 0xC1 && c <= 0xDA) { c = (unsigned char)(c - 0xC1 + 'A'); }
            else if (c < 0x20 || c > 0x7E) { c = ' '; }
            p->title[line][n++] = (char)c;
        }
        p->title[line][n] = 0;
        if (n > 0) { p->title_lines = line + 1; }
    }

    // Stereo Sidplayer: the .str carries voices 4-6 for the second SID chip.
    if (str && str_size > 0) {
        int soff, slens[3];
        if (parse_header(str, str_size, &soff, slens)) {
            unsigned char* sd = (unsigned char*)malloc(str_size);
            memcpy(sd, str, str_size);
            p->strdata = sd;
            int so = soff;
            for (int i = 0; i < 3; i++) {
                voice_init(&p->v[3 + i], sd + so, slens[i]);
                so += slens[i];
            }
            p->voices = 6;
        }
    }

    p->tempo = MUS_DEFAULT_TEMPO;
    // The player's default master volume is 8, not full scale: every one of the
    // 13 traced tunes goes 15 (power-on) -> 8 at t=0.004s, and only then to a
    // tune-supplied VOL (12, 9, ...) if it has one. Tunes with no VOL command
    // at all -- Cheers, raistlin -- simply stay at 8.
    p->volume[0] = p->volume[1] = 8;
    p->filt_mode[0] = p->filt_mode[1] = 0;
    p->filt_cutoff[0] = p->filt_cutoff[1] = 0;
    p->filt_res[0] = p->filt_res[1] = 0;
    p->samplerate = samplerate;
    // The tempo formula in MUS_format_B (14400 / TEM = quarter notes per minute)
    // is only self-consistent on a 60 Hz jiffy clock, which is what the NTSC C64
    // this was written for used: TEM=256 -> 56.25 QPM -> a quarter note is
    // 3600/56.25 = 64 jiffies, matching TEM/4 exactly.
    p->jiffy = 0;
    p->tick_scale = 1.0;
    p->start_delay = MUS_START_TICKS;
    mus_retime(p);

    csid_chip_init(samplerate, p->voices > 3 ? 2 : 1, MUS_SID2_BASE);
    return p;
}

void mus_destroy(MusPlayer* p)
{
    if (!p) { return; }
    // Free the ALLOCATION BASE, never v[3].d -- the voice pointers are offsets
    // into the .str buffer, so freeing one of those is an invalid free (SIGABRT).
    free(p->strdata);
    free(p->data);
    free(p);
}

const char* mus_title_line(MusPlayer* p, int line)
{
    if (!p || line < 0 || line >= p->title_lines) { return NULL; }
    return p->title[line];
}

// ---------------------------------------------------------------------------
// Duration decoding -- MUS_format_B, "BYTE 1 - TIE AND DURATION"
// ---------------------------------------------------------------------------
// bits 1-0 == 00 marks a note pair. bit 6 = tie. bits 4-2 select the base
// length (010 whole, 011 half, 100 quarter, 101 eighth, 110 16th, 111 32nd,
// 000 64th). bit 7 and bit 5 together give the dot type: 01 dotted,
// 11 double-dotted, 10 triplet, 00 plain.
static double duration_frames(MusPlayer* p, MusVoice* v, unsigned char b1)
{
    int tempo = p->tempo ? p->tempo : 256;
    int code = (b1 >> 2) & 7;
    // Length code 1 does NOT name a note value: it means "the length was given
    // explicitly by the preceding UTL command", in TICKS. That is how the
    // MIDI-converted tunes encode everything -- "Misty" (Ken Lo) is a chain of
    // UTL/note pairs whose UTL bytes reproduce VICE's note spacing exactly
    // (29+1=30, 32+1=33, 32+17=49 ticks), and Dan Barrett's ".str" opens with
    // UTL 2 for a 2-tick rest. Without it those tunes ran ~8x too fast.
    if (code == 1 && v->utl > 0) { return (double)v->utl; }
    // Length code 0 splits on BIT 5. MUS_format_A gives the 64th as $20 -- bit 5
    // SET -- and a code-0 byte with bit 5 CLEAR is a ZERO-LENGTH event: it sets
    // the voice's pitch and retriggers it but costs no time, so the pair that
    // follows starts on the same tick. "Painless Dream" is built out of
    // `00 xx` + `14 xx` pairs and VICE spaces its onsets exactly one eighth
    // apart; charging the `00 xx` a single tick (the old frames<1 clamp) made
    // every gap one tick too long, on every note, for the whole tune.
    if (code == 0 && !(b1 & 0x20)) { return 0.0; }
    // Otherwise the 3-bit field is a table INDEX and the table wraps: 2..7 run
    // whole,half,quarter,8th,16th,32nd -- 64,32,16,8,4,2 in 64ths, halving at
    // every step -- so index 0 continues it as a 64th (1).
    int shift = (code == 0) ? 8 : (code == 1) ? 9 : code;
    // A whole note is `tempo` frames; each step halves it.
    double frames = ((double)tempo * 4) / (double)(1 << shift);

    // For a 64TH note, bit 5 is part of the LENGTH, not a dot. The two format
    // documents contradict each other here -- MUS_format_A lists 64TH as $20
    // (bit 5 set), MUS_format_B lists bit 5 as "dotted" -- and A is right: it
    // also says dots only apply from WHOLE to 32ND, so a dotted 64th is simply
    // unrepresentable. Reading $20 as a dotted 64th made every one of those
    // notes 3 ticks instead of 2, so a voice gained a tick per 64th and slid out
    // of sync with the others over ~20s. Bit 7 still marks a TRIPLET 64th ($A0).
    int dot = (code == 0) ? (((b1 >> 6) & 2) ? 2 : 0)
                          : (((b1 >> 6) & 2) | ((b1 >> 5) & 1));
    switch (dot) {
    case 1: frames = frames * 3.0 / 2.0; break; // dotted
    case 3: frames = frames * 7.0 / 4.0; break; // double dotted
    case 2: frames = frames * 2.0 / 3.0; break; // triplet
    default: break;
    }
    // Kept FRACTIONAL: rounding each note to whole ticks loses up to a tick per
    // note and, because each voice rounds independently, the three voices drift
    // apart over time -- audible as the channels sliding out of sync.
    if (frames < 1.0) { frames = 1.0; }
    return frames;
}

// ---------------------------------------------------------------------------
// Command execution
// ---------------------------------------------------------------------------
static void set_note(MusPlayer* p, MusVoice* v, int chip, unsigned char b1,
                     unsigned char b2)
{
    int note = b2 & 7;
    double frames = duration_frames(p, v, b1);
#ifdef MUS_TRACE_HOOKS
    mus_ev((int)(v - p->v), v->pos - 2, b1, b2, frames);
#endif
    v->tied = (b1 & 0x40) != 0;

    if (note == 0) { // Rest
        v->resting = 1;
        v->gate = 0;
        v->wait = frames;
        v->gate_off = 0.0;
        return;
    }

    // The width restarts from its P-W base at every note-on; the sweep then
    // walks it away again for the duration of the note.
    v->pw = v->pw_base;
    v->resting = 0;
    // The note-on tick itself writes the UNMODULATED value -- VICE emits pw
    // 1000,1004,1008,... and freq base,base+d,..., so the sweep and the vibrato
    // both start moving on the tick AFTER the note-on, not on it.
    v->just_started = 1;

    int octave = 7 - ((b2 >> 3) & 7);
    int semi = kSemitone[note];
    switch ((b2 >> 6) & 3) {
    case 3: semi -= 1; break; // flat
    case 2: break;            // natural
    case 1: semi += 1; break; // sharp
    case 0:
        // "Double sharp (GFDC) or Double flat (ABE)" -- the spelling depends on
        // the letter, so C/D/F/G raise and E/A/B lower.
        semi += (note == 1 || note == 2 || note == 4 || note == 5) ? 2 : -2;
        break;
    }
    semi += v->transpose;
    while (semi < 0) { semi += 12; octave--; }
    while (semi > 11) { semi -= 12; octave++; }
    if (octave < 0) { octave = 0; }
    if (octave > 8) { octave = 8; }

    // DTN is a raw offset on the frequency register, not a pitch interval.
    int semi_abs = octave * 12 + semi;
    int target = sid_freq_for(octave, semi, 0) + v->detune;
    if (target < 0) { target = 0; }
    if (target > 65535) { target = 65535; }
    v->por_target = target;
    // With portamento armed the voice keeps SOUNDING its previous pitch and
    // slides to the new one; otherwise it arrives instantly. A zero-length
    // `00 xx` event before the note is how a tune sets the glide's STARTING
    // pitch -- it costs no time but does establish base_freq.
    if (v->por_rate <= 0 || v->base_freq <= 0) { v->base_freq = target; }
    v->freq = v->base_freq;

    // AUT ("filter auto") makes the cutoff TRACK THE NOTE, recomputed at every
    // note-on -- which is why a tune can drive the filter with no F-C command at
    // all. Derived from VICE: over 14 note-ons of Africa (AUT=-15) the cutoff is
    // semitone * 5/3 + AUT to within a unit (k measured 1.6618-1.6743, median
    // 1.6684 = 5/3), and it RESETS at each note-on before F-S sweeps it again.
    // Without this the cutoff stayed 0, so any voice routed through the filter
    // was silenced outright -- audible as voices vanishing mid-tune.
    if (v->aut_set) {
        int cut = (semi_abs * 5 + 1) / 3 + v->aut;
        if (cut < 0) { cut = 0; }
        if (cut > 255) { cut = 255; }
        p->filt_cutoff[chip] = cut;
        p->filt_cutoff_set[chip] = 1;
        p->filt_sweep_cnt[chip] = 0;
    }
    v->wait = frames;
    // Retrigger unless this note was tied into from the previous one.
    v->gate = 1;
    // PNT ("release point") decides where inside the note the gate drops. The
    // documents give it only as a 0-255 value with no units, so the reading is
    // fitted: MUS_PNTMODE selects between absolute frames and a fraction of the
    // note, and 0 means hold for the whole note.
    // Release the gate one tick BEFORE the note ends. Without that gap the
    // gate is cleared at the end of a note and re-asserted by the next note in
    // the SAME tick, so it never actually reads 0, the SID envelope never sees
    // a rising edge, and it never retriggers -- notes blur into one long decay
    // and voices drift to wildly different levels. VICE holds the gate low for
    // exactly one tick between notes.
    //
    // This is NOT PNT: PNT does not shorten the gate any further (tracing VICE,
    // voice-3 gate agreement went 10%% -> 94%% once we stopped releasing early),
    // so PNT/HLD stay parsed but unused.
    v->gate_off = frames > 1.0 ? frames - 1.0 : frames;
}

// Returns 1 while the voice still has work, 0 once halted.
static void voice_step(MusPlayer* p, MusVoice* v, int chip)
{
    if (v->halted) { return; }
    if (v->wait > 0.0) {
        v->wait -= 1.0;
        if (v->gate_off > 0.0) {
            v->gate_off -= 1.0;
            // Release at the end of the note unless it ties into the next.
            if (v->gate_off <= 0.0 && !v->tied) { v->gate = 0; }
        }
        if (v->wait > 0.0) { return; }
    }

    // Consume commands until one costs time (a note or rest) or we halt.
    for (int guard = 0; guard < 4096; guard++) {
        if (v->pos + 1 >= v->len) {
            // Ran off the end without a HLT; treat as halted.
            if (v->call_return >= 0) {
                v->d = v->call_d;
                v->len = v->call_len;
                v->pos = v->call_return;
                v->call_return = -1;
                continue;
            }
            v->halted = 1;
            v->gate = 0;
            return;
        }
        unsigned char b1 = v->d[v->pos];
        unsigned char b2 = v->d[v->pos + 1];
        v->pos += 2;

        // While defining a phrase we still play it through, but remember where
        // it started so a later CALL can replay it.
        if ((b1 & 3) == 0) { // note or rest -- normally costs time
            set_note(p, v, chip, b1, b2);
            // A zero-length event does not end the tick; the next pair runs now.
            if (v->wait > 0.0) { return; }
            continue;
        }

        if (b1 == 0x01) {
            int hi = (b2 >> 4) & 0xF;
            // ATK/SUS are the one entry in this table with a THREE-bit tag:
            // MUS_format_B gives them as "?nnn n100", value in bits 6-3, bit 7
            // selecting SUS over ATK. Dispatching them on the low NIBBLE (like
            // every other entry) silently drops every odd value -- sustain 5 is
            // 0xAC, whose low nibble is C -- which lost half of all ATK/SUS
            // commands in every tune. Must be matched before the nibble switch.
            if ((b2 & 7) == 4) {
                if (b2 & 0x80) { v->sus = (b2 >> 3) & 0xF; }
                else { v->atk = (b2 >> 3) & 0xF; }
                continue;
            }
            switch (b2 & 0xF) {
            case 0x0: v->dcy = hi; break;                       // DCY
            case 0x2: // CAL 0-15
                if (v->call_return < 0 && hi < 24 && p->phrase_start[chip][hi] >= 0) {
                    v->call_return = v->pos;
                    v->call_d = v->d;
                    v->call_len = v->len;
                    v->d = p->phrase_d[chip][hi];
                    v->len = p->phrase_len[chip][hi];
                    v->pos = p->phrase_start[chip][hi];
                }
                break;
            case 0x6: // DEF 0-15
                if (hi < 24) {
                    v->defining = hi;
                    p->phrase_start[chip][hi] = v->pos;
                    p->phrase_d[chip][hi] = v->d;
                    p->phrase_len[chip][hi] = v->len;
                }
                break;
            case 0x7: // WAV (bit4=0) / F-M (bit4=1)
                if (b2 & 0x10) {
                    int m = (b2 >> 5) & 7;
                    p->filt_mode[chip] = ((m & 1) ? 0x10 : 0) | ((m & 2) ? 0x20 : 0) |
                                         ((m & 4) ? 0x40 : 0);
                } else {
                    int w = (b2 >> 5) & 7;
                    static const int wavebits[8] = { 0x80, 0x10, 0x20, 0x30,
                                                     0x40, 0x50, 0x60, 0x70 };
                    v->wave = wavebits[w];
                }
                break;
            case 0x8: v->rls = hi; break;                       // RLS
            case 0xA: p->filt_res[chip] = hi; break;                  // RES
            case 0xE: p->volume[chip] = hi; break;                    // VOL
            case 0x3:
                if (b2 & 0x80) {                                // DEF 16-23
                    int n = 16 + (hi - 8);
                    if (n >= 0 && n < 24) {
                        v->defining = n;
                        p->phrase_start[chip][n] = v->pos;
                        p->phrase_d[chip][n] = v->d;
                        p->phrase_len[chip][n] = v->len;
                    }
                } else {
                    switch (b2) {
                    case 0x03: // BMP -- VOLUME:BUMP UP, one step
                        if (p->volume[chip] < 15) { p->volume[chip]++; }
                        break;
                    case 0x13: v->filt_through = 0; break;      // FLT NO
                    case 0x23: v->ring = 0; break;              // RNG NO
                    case 0x33: v->sync = 0; break;              // SNC NO
                    default: break;
                    }
                }
                break;
            case 0xB:
                if (b2 & 0x80) {                                // CAL 16-23
                    int n = 16 + (hi - 8);
                    if (n >= 0 && n < 24 && v->call_return < 0 &&
                        p->phrase_start[chip][n] >= 0) {
                        v->call_return = v->pos;
                        v->call_d = v->d;
                        v->call_len = v->len;
                        v->d = p->phrase_d[chip][n];
                        v->len = p->phrase_len[chip][n];
                        v->pos = p->phrase_start[chip][n];
                    }
                } else {
                    switch (b2) {
                    case 0x0B: // BMP -- VOLUME:BUMP DOWN, one step
                        if (p->volume[chip] > 0) { p->volume[chip]--; }
                        break;
                    case 0x1B: v->filt_through = 1; break;      // FLT YES
                    case 0x2B: v->ring = 1; break;              // RNG YES
                    case 0x3B: v->sync = 1; break;              // SNC YES
                    default: break;
                    }
                }
                break;
            case 0xF:
                if (b2 == 0x4F) {                               // HLT
                    v->halted = 1;
                    v->gate = 0;
                    return;
                }
                if (b2 == 0x0F) {                               // TAL (repeat)
                    if (v->rep_count > 0 && v->rep_pos >= 0) {
                        v->rep_count--;
                        v->pos = v->rep_pos;
                    }
                } else if (b2 == 0x2F) {                        // END of phrase
                    if (v->defining >= 0) {
                        v->defining = -1;
                    } else if (v->call_return >= 0) {
                        v->d = v->call_d;
                        v->len = v->call_len;
                        v->pos = v->call_return;
                        v->call_return = -1;
                    }
                }
                break;
            default: break;
            }
            continue;
        }

        // Commands whose first byte is not 01.
        //
        // The low two bits of byte 1 partition the stream (MUS_format_B): 00 is
        // a note, 01 selects the big "first byte = hex 01" table above, 11 is
        // PORTAMENTO (whose top six bits are all value), and 10 is everything
        // else. Within that last group the LOW NIBBLE selects the family and
        // the HIGH nibble is either part of the opcode or part of the value --
        // so these must be matched on the FULL byte, never on b1 & 0x3F. Masking
        // conflates C6 (PVD) and 86 (VRT) with 06 (TEM), which silently rewrites
        // the tempo mid-tune.
        if ((b1 & 3) == 3) { // POR -- 14-bit portamento, byte1 bits 7-2 + byte2
            // A per-tick delta on the FREQUENCY REGISTER, not a pitch interval.
            // "Vicar1" carries POR 1000 and VICE glides 2145 -> 3145 -> target:
            // exactly +1000 a tick, clamped on arrival.
            v->por_rate = (((b1 >> 2) & 0x3F) << 8) | b2;
            continue;
        }
        switch (b1 & 0x0F) {
        case 0x02: // P-W -- 12-bit pulse width, byte1 bits 7-4 + byte2
            v->pw = v->pw_base = (((b1 >> 4) & 0xF) << 8) | b2;
            break;
        case 0x0A: // DTN -- 11-bit detune, byte1 bits 7-5 + byte2, bit4 = sign
        {
            int val = (((b1 >> 5) & 7) << 8) | b2;
            if (b1 & 0x10) { val -= 2048; }
            v->detune = val;
            break;
        }
        case 0x0E:
            // Low nibble E splits four ways on byte1 bits 5-4 (and bit 6).
            switch (b1 & 0x30) {
            case 0x00:
                if (b1 & 0x40) { v->hold_time = b2; }        // 4E HLD
                else { p->filt_cutoff[chip] = b2; p->filt_cutoff_set[chip] = 1; } // 0E F-C
                break;
            case 0x10: break;                               // MS# measure marker
            case 0x20: break;                               // 2E RTP / 6E SCA
            case 0x30: {                                    // JIF jiffy length
                // 10-bit signed: byte1 bits 7-6 are the low 2 bits, byte2 the
                // high 8 (so byte1 cycles 3E/7E/BE/FE per step of 1).
                int v = b2 * 4 + ((b1 >> 6) & 3);
                if (v >= 512) { v -= 1024; }
                p->jiffy = v;
                mus_retime(p);
                break;
            }
            }
            break;
        case 0x06:
            switch (b1) {
            case 0x06: p->tempo = b2; break;                // TEM
            case 0x16: v->utl = b2; break;                    // UTL
            case 0x26: v->rls_point = b2; break;              // PNT
            case 0x36: // HED -- repeat head
                // The count is how many times the block is played in TOTAL, not
                // how many EXTRA times: "Africa" (Dan Barrett) opens HED 2 ...
                // TAL and VICE plays that block twice, where treating it as two
                // repeats gave a third pass and threw every voice off the rails
                // ~11s in.
                v->rep_count = (b2 > 0) ? b2 - 1 : 0;
                v->rep_pos = v->pos;
                break;
            case 0x56: v->pw_sweep = (signed char)b2; break;   // P-S
            case 0x66: p->filt_sweep[chip] = (signed char)b2; p->filt_sweep_cnt[chip] = 0; break; // F-S
            case 0x96: v->aut = (signed char)b2; v->aut_set = 1; break;  // AUT
            case 0x76: v->vib_depth = b2 & 0x7F; break;        // VDP
            case 0x86: v->vib_rate = b2; break;                // VRT
            case 0xC6: v->pvib_depth = b2 & 0x7F; break;       // PVD
            case 0xD6: v->pvib_rate = b2 & 0x7F; break;        // PVR
            case 0xA6: {                                    // TPS -- transpose
                int neg = b2 & 1;
                int oct = (b2 >> 1) & 7;
                int half = (b2 >> 4) & 0xF;
                v->transpose = neg ? -(oct * 12 + (11 - half))
                                   : ((7 - oct) * 12 + half);
                break;
            }
            // 26 PNT, 46 FLG, 56 P-S, 66 F-S, 76 VDP, 86 VRT, 96 AUT,
            // B6 AUX, C6 PVD, D6 PVR, E6 MAX, F6 UTV -- vibrato/sweep/utility
            // effects not modelled yet; ignored rather than mis-dispatched.
            default: break;
            }
            break;
        default: break;
        }
    }
    // Guard tripped (pathological stream): stop this voice rather than spin.
    v->halted = 1;
    v->gate = 0;
}

// ---------------------------------------------------------------------------
// Per-frame SID register write-out
// ---------------------------------------------------------------------------
// The player is driven by a CIA timer, NOT a fixed 50/60 Hz frame -- confirmed by
// tracing VICE's own sid_store() writes: tunes with no JIF command tick every
// ~16467 cycles (59.83 Hz), and JIF shifts that by 64 cycles per unit. "linus and
// lucy" carries JIF=-72 and really runs at 83 Hz, so treating every tune as 60 Hz
// played it 1.39x too slow -- which no amount of tempo fitting could correct.
#define MUS_TICK_BASE_CYCLES 16467
#define MUS_TICK_PER_JIF     64
#define MUS_C64_PAL_CLK      985248.0
static void mus_retime(MusPlayer* p)
{
    double cycles = MUS_TICK_BASE_CYCLES + (double)p->jiffy * MUS_TICK_PER_JIF
                    + MUS_TICK_IRQ_CYCLES;
    if (cycles < 1000.0) { cycles = 1000.0; }
    p->samples_per_frame = p->samplerate * cycles / MUS_C64_PAL_CLK;
    // JIF buys finer timing RESOLUTION, it does not change the musical tempo:
    // TEM is quarter-notes-per-MINUTE (real time), so a note's length in ticks
    // has to scale with the tick rate. Without this, "linus and lucy" (JIF=-72,
    // 83 Hz) played 1.39x too fast and scored WORSE than ignoring JIF entirely.
    // What the faster tick actually buys is per-tick effects -- vibrato and the
    // pulse/filter sweeps -- running at their intended speed.
    p->tick_scale = (MUS_C64_PAL_CLK / cycles) / 60.0;
}

// Per-frame continuous modulation. The format documents these only as values,
// not as behaviour, so the shapes here are the straightforward reading: sweeps
// are a signed increment per frame, vibratos are a triangle LFO whose rate sets
// the phase increment. Depth/rate scaling was fitted against VICE renders.
static void apply_modulation(MusPlayer* p)
{
    for (int i = 0; i < p->voices; i++) {
        MusVoice* v = &p->v[i];
        // On the note-on tick the counters HOLD -- but the frequency still
        // carries their current offset. Skipping the whole body instead wrote a
        // bare base_freq there, one vibrato step away from what VICE holds.
        int advance = !v->just_started;
        v->just_started = 0;
        // P-S is a SIGNED PER-TICK DELTA on the pulse width, and it only runs
        // for as long as a NOTE is running -- including the one-tick gate-release
        // gap at its end -- and freezes only through a REST. That gate is the
        // whole story: it is why this
        // looked like an unresolvable conflict for so long. "Raistlin" voices 1
        // and 2 carry the SAME commands (P-W 120, P-S 7) and VICE sweeps only
        // voice 1 -- because voice 2 is resting, and a resting voice's width is
        // frozen. Applying the sweep unconditionally wrecked exactly those tunes
        // whose P-S voices spend most of their time un-gated.
        // Portamento: walk the sounding pitch toward the note's pitch. Runs on
        // the same schedule as the other modulation (held for the note-on tick,
        // frozen through a rest).
        if (advance && v->por_rate > 0 && !v->resting &&
            v->base_freq != v->por_target) {
            int d = v->por_target - v->base_freq;
            int step = (d > 0) ? v->por_rate : -v->por_rate;
            if ((d > 0 && step > d) || (d < 0 && step < d)) { step = d; }
            v->base_freq += step;
        }
        if (advance && v->pw_sweep && !v->resting) {
            v->pw += v->pw_sweep;
            if (v->pw < 0) { v->pw = 0; }
            if (v->pw > 4095) { v->pw = 4095; }
        }
        // Frequency vibrato, DERIVED from the oracle rather than guessed. Within
        // a single held note VICE's frequency register walks a triangle whose
        // step is exactly VDP and whose amplitude is VRT*VDP:
        //   VDP=12,VRT=2 -> steps of 12, swing 48, period 8 ticks
        //   VDP=5, VRT=1 -> steps of  5, swing 10, period 4 ticks
        // i.e. a counter that moves one VDP per tick and reverses at +/-VRT, so
        // the period is 4*VRT ticks. The note's table value is the CENTRE, and
        // the counter starts at 0 on note-on.
        // Like P-S, the vibrato counter only advances while the voice is
        // GATED; through a rest it FREEZES and the frequency register holds its
        // last value, then the triangle resumes from there at the next note-on.
        // "Africa" (DC Starr) shows this plainly: 4547,4551,4547,4543,... while
        // gated, a flat 4547 for the whole rest, then 4547,4543,... again. That
        // freeze is also why successive note-ons land on different points of the
        // triangle, which looked like a phase bug for a long time.
        if (v->vib_depth && v->vib_rate) {
            if (advance && !v->resting) {
                v->vib_pos += v->vib_dir;
                if (v->vib_pos >= v->vib_rate) { v->vib_pos = v->vib_rate; v->vib_dir = -1; }
                else if (v->vib_pos <= -v->vib_rate) { v->vib_pos = -v->vib_rate; v->vib_dir = 1; }
            }
            long f = (long)v->base_freq + (long)v->vib_pos * v->vib_depth;
            if (f < 0) { f = 0; }
            if (f > 65535) { f = 65535; }
            v->freq = (int)f;
        } else {
            v->freq = v->base_freq;
        }
        // Pulse-width vibrato: same triangle applied to the pulse width.
        if (advance && v->pvib_depth && v->pvib_rate && !v->resting) {
            v->pvib_pos += v->pvib_dir;
            if (v->pvib_pos >= v->pvib_rate) { v->pvib_pos = v->pvib_rate; v->pvib_dir = -1; }
            else if (v->pvib_pos <= -v->pvib_rate) { v->pvib_pos = -v->pvib_rate; v->pvib_dir = 1; }
        }
    }
    // F-S IS a cutoff sweep, but a RATE rather than a per-tick delta: on
    // "La Donna e Mobile" (F-S=3) VICE steps the cutoff by +1 every 3.11 ticks.
    // An earlier round concluded F-S must not sweep at all -- that was
    // over-generalised from raistlin, where NOTHING is routed through the
    // filter, so its cutoff register reads 0 whatever the sweep does.
    for (int chip = 0; chip < (p->voices > 3 ? 2 : 1); chip++) {
        if (!p->filt_sweep[chip]) { continue; }
        // F-S only sweeps when the filter is in AUTO mode. Tunes that set F-S
        // WITHOUT ever issuing AUT leave the cutoff pinned at 0 in VICE --
        // "Coming Home" (BJ Pools, F-S=+5) holds 0 for the whole tune while we
        // walked it 0,1,2,3... The sweep is what moves an AUT-seeded cutoff
        // between note-ons; with no AUT there is nothing for it to move.
        {
            int auto_on = 0;
            for (int i = 0; i < 3; i++) {
                if (p->v[chip * 3 + i].aut_set) { auto_on = 1; }
            }
            if (!auto_on) { continue; }
        }
        int rate = p->filt_sweep[chip] < 0 ? -p->filt_sweep[chip] : p->filt_sweep[chip];
        if (++p->filt_sweep_cnt[chip] >= rate) {
            p->filt_sweep_cnt[chip] = 0;
            p->filt_cutoff[chip] += (p->filt_sweep[chip] < 0) ? -1 : 1;
            // Clamp rather than wrap. With AUT active the cutoff is re-seeded at
            // every note-on so the sweep never runs far; without it the sweep
            // would otherwise walk the whole 0-255 range.
            if (p->filt_cutoff[chip] < 0) { p->filt_cutoff[chip] = 0; }
            if (p->filt_cutoff[chip] > 255) { p->filt_cutoff[chip] = 255; }
            p->filt_cutoff_set[chip] = 1;
        }
    }
}

static void write_regs(MusPlayer* p)
{
    for (int i = 0; i < p->voices; i++) {
        MusVoice* v = &p->v[i];
        unsigned int base = (i < 3) ? 0xD400 : MUS_SID2_BASE;
        unsigned int vb = base + (unsigned)(i % 3) * 7;
        csid_poke(vb + 0, (unsigned char)(v->freq & 0xFF));
        csid_poke(vb + 1, (unsigned char)((v->freq >> 8) & 0xFF));
        int pwv = v->pw + v->pvib_pos * v->pvib_depth;
        if (pwv < 0) { pwv = 0; }
        if (pwv > 4095) { pwv = 4095; }
        csid_poke(vb + 2, (unsigned char)(pwv & 0xFF));
        csid_poke(vb + 3, (unsigned char)((pwv >> 8) & 0x0F));
        int ctrl = v->wave | (v->gate ? 1 : 0) | (v->sync ? 2 : 0) |
                   (v->ring ? 4 : 0);
        csid_poke(vb + 4, (unsigned char)ctrl);
        csid_poke(vb + 5, (unsigned char)((v->atk << 4) | v->dcy));
        csid_poke(vb + 6, (unsigned char)((v->sus << 4) | v->rls));
    }
    for (int chip = 0; chip < (p->voices > 3 ? 2 : 1); chip++) {
        unsigned int base = chip ? MUS_SID2_BASE : 0xD400;
        int routed = 0;
        for (int i = 0; i < 3; i++) {
            if (p->v[chip * 3 + i].filt_through) { routed |= (1 << i); }
        }
        // Cutoff, resonance and mode are chip-wide state and are written
        // UNCONDITIONALLY -- they are NOT gated on whether a voice happens to be
        // routed through the filter. "La Donna e Mobile" routes nothing at all
        // (its $D417 low nibble stays 8) yet still drives cutoff 80/100/72/98,
        // resonance 15 and low-pass mode; suppressing those writes cost it three
        // whole registers. Only the routing bits themselves come from FLT.
        int cutoff = p->filt_cutoff[chip];
        // The format's F-C/AUT carry a single byte, which the player puts in the
        // HIGH register; the 3-bit low half has no source and VICE leaves it 0.
        csid_poke(base + 0x15, 0);
        csid_poke(base + 0x16, (unsigned char)cutoff);
        // Bit 3 (external input) is set unconditionally -- VICE writes it on
        // every tune traced.
        csid_poke(base + 0x17,
             (unsigned char)((p->filt_res[chip] << 4) | routed | 0x08));
        csid_poke(base + 0x18,
             (unsigned char)(p->filt_mode[chip] | (p->volume[chip] & 0xF)));
    }
}

int mus_render(MusPlayer* p, short* out, int frames)
{
    int done = 0;
    while (done < frames) {
        if (p->frame_acc <= 0.0) {
            int alive = 0;
            if (p->start_delay > 0) {
                p->start_delay--;
                alive = 1;
                goto emit;
            }
#ifdef MUS_TRACE_HOOKS
            mus_trace_set_time((double)p->rendered / (double)p->samplerate);
#endif
            for (int i = 0; i < p->voices; i++) {
                voice_step(p, &p->v[i], MUS_CHIP_OF(i));
                if (!p->v[i].halted) { alive = 1; }
            }
            apply_modulation(p);
        emit:
            write_regs(p);
            if (!alive) { p->finished = 1; }
            p->frame_acc += p->samples_per_frame;
        }
        int chunk = (int)p->frame_acc;
        if (chunk > frames - done) { chunk = frames - done; }
        if (chunk < 1) { chunk = 1; }
        csid_chip_render_stereo(out + done * 2, chunk);
        p->frame_acc -= chunk;
        p->rendered += chunk;
        done += chunk;
    }
    return done;
}

int mus_finished(MusPlayer* p) { return p ? p->finished : 1; }
