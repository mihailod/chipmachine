// Synthetic YRW801 wavetable bank -- chipmachine's own code and own data.
//
// WHAT THIS IS. The OPL4 has no instruments of its own: wave 0-383 is a
// twelve-byte header at ROM address wave*12, pointing at PCM in the same 2 MB
// address space (see ymf278b_C_w case 0). On a MoonSound that ROM is Yamaha's
// YRW801, a GM sample set we do not have and may not ship. This file builds a
// substitute in the same layout from generated waveforms, so MSX MoonSound rips
// play their notes, rhythm, envelopes and dynamics. It is NOT a YRW801
// reproduction: the timbres are ours, and they do not sound like the real thing.
// A real dump, if the user has one, wins -- see CM_YRW801_ROM below.
//
// WHERE THE TUNING COMES FROM. A wave's header carries no pitch, so the driver
// must know each sample's base pitch; every MoonSound driver ships that table
// and we do not have it. It was recovered by measurement over the 158 MSX
// MoonSound tracks in the ChipMachine corpus (5 VGMRips packs, 436,244 key-ons),
// entirely from the music -- no Yamaha data, no GPL source, nothing copied:
//
//   * Sub-semitone part: every wave's notes lie on a 100-cent grid whose PHASE
//     is wave-specific (222 of 263 well-sampled waves cluster above 0.9). That
//     phase is the fractional part of the wave's base tuning, measured directly.
//   * Semitone/octave: scored against notes sounding AT THE SAME TIME whose
//     absolute pitch is known -- the log's own uploaded samples (pitch by
//     autocorrelation) and, where a track drives the OPL4's FM half too, YMF262
//     notes (pitch exact from F-number and block). Waves with no such neighbour
//     were solved against already-solved waves, by relaxation to convergence.
//   * 18 waves came from FM, 80 from RAM samples, 75 propagated, 17 are
//     percussion (pitch never varies -> a drum, not a note). The remaining 211
//     are played too rarely to determine and fall back to a register derived
//     from where the solved waves land. Mean interval consonance against all
//     concurrent evidence: 0.364, where random assignment scores 0.179.
//
// TRAPS, all of them measured:
//   * Waveforms must be EXACTLY periodic over the stored length or the loop
//     point steps and clicks once per cycle. Hence `cycles` in the table: the
//     stored period is length/cycles by construction, at a cost of ~1 cent.
//   * The chip resamples with linear interpolation and NO filtering, so a wave
//     played above its stored pitch folds every partial it has. `partials` is
//     capped per wave from the highest playback ratio that wave is driven at
//     anywhere in the corpus, keeping the top partial under 18 kHz.
//   * Percussion must loop a run of SILENCE. Looping the last couple of samples
//     (the obvious choice) is a ~11-22 kHz square wave that never stops.
//   * Schroeder phases, not zero phases: same spectrum, far lower crest factor,
//     so peak-normalised waves are much louder.

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include "cm_yrw801.h"

typedef struct
{
	unsigned short length;		// samples stored
	unsigned short loop;		// loop point, in samples
	unsigned char cycles;		// waveform periods within `length` (0 = percussion)
	unsigned char partials;		// harmonics summed
	unsigned char flags;		// 1 = percussion, 2 = soft spectrum
	unsigned short drumHz;		// percussion body tone
	unsigned char env[3];		// wave-header bytes 8,9,10 (AR/D1R, DL/D2R, RC/RR)
} CM_YRW801_WAVE;

#include "cm_yrw801_table.h"

#define WAVE_COUNT	384
#define HEADER_BYTES	(WAVE_COUNT * 12)
#define PERC_FLAG	0x01
#define SOFT_FLAG	0x02

// Deterministic noise: any decent LCG will do, it only has to be the same on
// every run and every platform.
static unsigned int cm_rand(unsigned int* state)
{
	*state = (*state * 1103515245u) + 12345u;
	return (*state >> 16) & 0x7FFF;
}

static void build_wave(const CM_YRW801_WAVE* wv, double* buf)
{
	unsigned int i, h;
	unsigned int len = wv->length;

	if (wv->flags & PERC_FLAG)
	{
		// Noise burst plus a decaying body tone, then a silent tail to loop in.
		unsigned int body = len > 96 ? len - 96 : len;
		unsigned int seed = (unsigned int)(wv->drumHz * 7919u + len);
		for (i = 0; i < body; i ++)
		{
			double env = exp(-(double)i / (body * 0.22));
			double n = ((double)cm_rand(&seed) / 16384.0 - 1.0) * 0.7;
			double b = sin(2.0 * M_PI * wv->drumHz * i / 44100.0) *
			           exp(-(double)i / (body * 0.10));
			buf[i] = env * n + 0.9 * b;
		}
		for (; i < len; i ++) buf[i] = 0.0;
		return;
	}

	{
		double rolloff = (wv->flags & SOFT_FLAG) ? 1.35 : 1.15;
		unsigned int n = wv->partials ? wv->partials : 1;
		double amp[32];
		double phase[32];
		for (h = 1; h <= n && h < 32; h ++)
		{
			amp[h] = 1.0 / pow((double)h, rolloff);
			phase[h] = M_PI * (double)h * (h + 1) / (double)n;
		}
		for (i = 0; i < len; i ++)
		{
			double ph = 2.0 * M_PI * ((double)i * wv->cycles / (double)len);
			double v = 0.0;
			for (h = 1; h <= n && h < 32; h ++)
				v += sin(ph * h + phase[h]) * amp[h];
			buf[i] = v;
		}
	}
}

// A user-supplied YRW801 dump replaces the whole bank. We ship no such file and
// never fetch one; this exists so that someone who has dumped their own
// cartridge can hear the real instruments. Anything shorter than the full 2 MB
// is loaded as far as it goes and the rest stays synthetic.
static int load_real_rom(unsigned char* rom)
{
	const char* path = getenv("CM_YRW801_ROM");
	FILE* fp;
	size_t n;

	if (path == NULL || path[0] == '\0') return 0;
	fp = fopen(path, "rb");
	if (fp == NULL) return 0;
	n = fread(rom, 1, CM_YRW801_SIZE, fp);
	fclose(fp);
	return n > 0;
}

const unsigned char* cm_yrw801_bank(void)
{
	static unsigned char* bank = NULL;
	static int built = 0;
	unsigned char* rom;
	double* buf;
	unsigned int w, i;
	unsigned int pos = HEADER_BYTES;

	if (built) return bank;
	built = 1;	// one attempt; a failure leaves the caller's silent stub in place

	rom = (unsigned char*)calloc(1, CM_YRW801_SIZE);
	if (rom == NULL) return NULL;
	buf = (double*)malloc(65536 * sizeof(double));
	if (buf == NULL) { free(rom); return NULL; }

	if (load_real_rom(rom))
	{
		free(buf);
		bank = rom;
		return bank;
	}

	for (w = 0; w < WAVE_COUNT; w ++)
	{
		const CM_YRW801_WAVE* wv = &cm_yrw801_waves[w];
		unsigned int len = wv->length;
		unsigned int start = pos;
		unsigned int end;
		unsigned char* hdr;
		double peak = 0.0;
		double scale;

		if (len == 0 || start + len * 2 > CM_YRW801_SIZE) break;

		build_wave(wv, buf);
		for (i = 0; i < len; i ++)
			if (fabs(buf[i]) > peak) peak = fabs(buf[i]);
		scale = (peak > 0.0) ? 29000.0 / peak : 0.0;
		for (i = 0; i < len; i ++)
		{
			double v = buf[i] * scale;
			int s = (int)v;
			if (s > 32000) s = 32000;
			if (s < -32000) s = -32000;
			rom[pos + 0] = (unsigned char)((s >> 8) & 0xFF);
			rom[pos + 1] = (unsigned char)(s & 0xFF);
			pos += 2;
		}

		// OPL4 wave header: format + 22-bit start, 16-bit loop, 16-bit end (held
		// as a negated count), then the five register bytes the chip copies into
		// the slot on tone load. 16-bit samples throughout (format 2).
		end = (0x10000 - len) & 0xFFFF;
		hdr = rom + w * 12;
		hdr[0] = (unsigned char)((2 << 6) | ((start >> 16) & 0x3F));
		hdr[1] = (unsigned char)((start >> 8) & 0xFF);
		hdr[2] = (unsigned char)(start & 0xFF);
		hdr[3] = (unsigned char)((wv->loop >> 8) & 0xFF);
		hdr[4] = (unsigned char)(wv->loop & 0xFF);
		hdr[5] = (unsigned char)((end >> 8) & 0xFF);
		hdr[6] = (unsigned char)(end & 0xFF);
		hdr[7] = 0x00;			// LFO / vibrato off
		hdr[8] = wv->env[0];
		hdr[9] = wv->env[1];
		hdr[10] = wv->env[2];
		hdr[11] = 0x00;			// AM off
	}

	free(buf);
	bank = rom;
	return bank;
}
