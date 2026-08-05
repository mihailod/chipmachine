// Bandai WonderSwan / WonderSwan Color sound -- chipmachine's own implementation.
// See ws_apu.h for the interface and README.md for the documentation trail.
//
// EVERY constant here comes from the WSdev Wiki (https://ws.nesdev.org/wiki/Sound
// plus its "Hyper Voice" and "DMA" pages):
//
//   * Four channels, each playing 32 x 4-bit samples from a 64-byte wavetable in
//     internal RAM (16 bytes per channel, base address = port $8F << 6). The
//     HIGH nibble of each byte is the LATER sample.
//   * "Every 2048 - divisor cycles, the index of the sample to be fetched from
//     the wavetable is incremented" -- so at the 3072000 Hz master clock a
//     channel's wavetable sample rate is 3072000 / (2048 - divisor).
//   * Output DAC runs at clock / 128 = 24000 Hz.
//   * Per-channel level is a plain product: out = sample * volume, 4-bit x
//     4-bit, so 15 * 15 = 225 is full scale on the 8-bit per-channel bus, and
//     the four buses sum into the unsigned 10-bit L/R output ("none of the
//     adders are clamped").
//   * Channel 2 can output an 8-bit unsigned PCM sample from $89 instead, at
//     100%/50%/off per side ($94).
//   * Channel 3 sweep: the SIGNED 8-bit value in $8C is added to its 11-bit
//     frequency divisor every ($8D & 0x1F) + 1 ticks of a 375 Hz clock, with
//     wraparound, and the tick counter restarts on every write to $8D.
//   * Channel 4 noise: 15-bit LFSR, new bit = NOT(bit 7 XOR tap bit), shifted in
//     at bit 0, one step per wavetable index increment; the resulting bit is
//     used as a wavetable sample of 0 or 15. Tap bit per mode ($8E & 7):
//     14, 10, 13, 4, 8, 6, 9, 11.
//   * Sound DMA ($4A-$52) streams bytes from anywhere in the 1 MB space into
//     either channel 2's voice register or Hyper Voice, at 4000/6000/12000/
//     24000 Hz, with auto-repeat and hold.
//   * Hyper Voice ($64-$6B, Color only) converts 8-bit samples to signed 16-bit
//     by one of four extension modes, shifts right by 0-3, and is added to the
//     headphone output AFTER the four channels are shifted left by 5 -- which
//     is exactly the scale this file's centred mix already produces.
//
// One behaviour is NOT in the register documentation but is required by real
// rips, and is documented in Mamiya's own public in_wsr readme (archived as
// awesome-wsdev/archive/in_wsr.txt): a frequency register holding $FFFF -- the
// whole 16-bit word, not the 11-bit divisor -- must produce SILENCE. Its
// changelog entry for 2006/4/14 records this, found because Rockman & Forte
// otherwise plays a spurious tone. A divisor of $7FF alone still sounds (Digimon
// D-Project's noise relies on it), so the test is on the full word.
//
// Deliberately ignored, and why:
//
//   * Port $91's speaker/headphone ENABLE bits. This core always renders the
//     stereo headphone path. VGM logs are register dumps taken from emulators
//     and many never write $91 at all, so honouring "not enabled = silent"
//     would mute whole packs -- the same trap PWM_CTRL sets in pwm_32x.c.
//     $91's speaker shift only affects the mono speaker mix, which is not the
//     output used here.
//   * Port $95 (sound test) and $9E (speaker volume / SOUND button).
//   * General DMA ($40-$48). It is a memory-to-memory block move with no audio
//     side effects; the machine performs it, not the APU.
//
// The DC question. Wavetable output is UNSIGNED (0..15 scaled by volume) and the
// hardware's analogue stage is AC-coupled, so a channel sitting on a constant
// waveform must not push a level-dependent offset into the mix. Each channel is
// therefore centred on its own midpoint before summing. To keep that exact in
// integers the mix runs in HALF units: a wavetable channel contributes
// (2 * sample - 15) * volume and a voice channel (2 * pcm - 255) >> shift, both
// of which are zero for a mid-scale input.

#include <stdlib.h>
#include <string.h>

#include "ws_apu.h"

#define WS_WAVE_STEPS	32		// samples in one channel's waveform
#define WS_SWEEP_HZ		375		// sweep tick clock

// The mix runs in half units, where one channel spans +/-225 (4-bit sample x
// 4-bit volume) and the four-channel sum spans +/-900. 16 lifts that to roughly
// +/-14400, which leaves headroom on the loudest four-channel material and
// matches the loudness of the core this replaces -- measured, not derived, over
// the VGMRips corpus (see libvgmplugin/README.md). It also happens to be exactly
// the hardware's own headphone scale: (2 * bus - full) * 16 == bus * 32 - const,
// and the mixing diagram shifts the channel sum left by 5 before Hyper Voice is
// added, so Hyper Voice mixes in at its natural 16-bit value.
#define WS_OUTPUT_GAIN	16

// --- register file shorthands (WSdev names) ---------------------------------
#define SNDSWP	apu->ioRam[0x8C]	// sweep amount, signed
#define SWPSTP	apu->ioRam[0x8D]	// sweep ticks - 1
#define NSCTL	apu->ioRam[0x8E]	// noise control
#define WAVDTP	apu->ioRam[0x8F]	// wavetable base >> 6
#define SNDMOD	apu->ioRam[0x90]	// channel control
#define PCSRL	apu->ioRam[0x92]	// LFSR readback, low
#define PCSRH	apu->ioRam[0x93]	// LFSR readback, high
#define PCVOL	apu->ioRam[0x94]	// channel 2 voice volume
#define SDMACTL	apu->ioRam[0x52]	// sound DMA control
#define HVCTLL	apu->ioRam[0x6A]	// Hyper Voice control, low
#define HVCTLH	apu->ioRam[0x6B]	// Hyper Voice control, high

// Tap bit per LFSR mode; sequence lengths 32767, 1953, 254, 217, 73, 63, 42, 28.
static const uint8_t WS_NOISE_TAP[8] = { 14, 10, 13, 4, 8, 6, 9, 11 };

// $52 bits 0-1. The wiki's rates; note in_wsr guessed 12/16/20/24 kHz instead,
// which is why sound-DMA-heavy rips can differ from that player.
static const uint16_t WS_DMA_HZ[4] = { 4000, 6000, 12000, 24000 };

static void ws_update_period(WSAPU* apu, uint8_t chNum)
{
	uint16_t reg = (uint16_t)((apu->ioRam[0x81 + chNum * 2] << 8) |
	                           apu->ioRam[0x80 + chNum * 2]);
	WSAPU_CHANNEL* c = &apu->ch[chNum];

	c->silent = (reg == 0xFFFF);
	c->period = (uint16_t)(2048 - (reg & 0x7FF));
}

// One LFSR step: new bit = NOT(bit 7 XOR tap bit), shifted in at bit 0. Note
// the feedback is an XNOR, so ALL-ONES is the lock-up state, not all-zero --
// which is why reset seeds the register with 0 (and why the hardware's own
// "LFSR reset" bit can safely clear it).
static void ws_clock_noise(WSAPU* apu)
{
	uint8_t tap = WS_NOISE_TAP[NSCTL & 0x07];
	uint16_t bit = (uint16_t)(~((apu->lfsr >> 7) ^ (apu->lfsr >> tap)) & 0x01);

	apu->lfsr = (uint16_t)(((apu->lfsr << 1) | bit) & 0x7FFF);
	PCSRL = (uint8_t)(apu->lfsr & 0xFF);
	PCSRH = (uint8_t)((apu->lfsr >> 8) & 0x7F);
}

// The wavetable lives in internal RAM at (WAVDTP << 6); channel n's 16 bytes
// follow at +n*16, and the HIGH nibble of a byte is the LATER of its two
// samples.
static uint8_t ws_wave_sample(WSAPU* apu, uint8_t chNum, uint8_t index)
{
	uint32_t addr = (uint32_t)((WAVDTP << 6) + chNum * 16 + (index >> 1));
	uint8_t b;

	if (apu->ram == NULL) { return 0; }
	b = apu->ram[addr & apu->ramMask];

	return (index & 1) ? (uint8_t)(b >> 4) : (uint8_t)(b & 0x0F);
}

void wsapu_init(WSAPU* apu, uint32_t clock, uint32_t rate)
{
	memset(apu, 0x00, sizeof(*apu));

	apu->clock = clock;
	apu->rate = rate ? rate : 1;

	// Master cycles per output sample, 16.16 -- exactly 128 at the real clock
	// and rate, but derived so a resampled or unusual rate cannot detune the
	// channels.
	apu->cyclesPerSmpl = (uint32_t)(((uint64_t)apu->clock << 16) / apu->rate);
	apu->sweepTickSmpls = (uint32_t)(((uint64_t)apu->rate << 16) / WS_SWEEP_HZ);
	if (apu->sweepTickSmpls == 0) { apu->sweepTickSmpls = 1 << 16; }

	wsapu_reset(apu);
}

void wsapu_reset(WSAPU* apu)
{
	uint8_t i;

	memset(apu->ioRam, 0x00, sizeof(apu->ioRam));
	memset(apu->ch, 0x00, sizeof(apu->ch));

	for (i = 0; i < WSAPU_CHANNELS; i++) { apu->ch[i].period = 2048; }

	apu->lfsr = 0x0000;	// see ws_clock_noise(): all-ones is the dead state
	apu->sweepAccum = 0;
	apu->sweepTicks = 0;

	apu->dmaSource = apu->dmaLength = 0;
	apu->dmaSourceShadow = apu->dmaLengthShadow = 0;
	apu->dmaAccum = 0;
	apu->dmaTickSmpls = 0;

	apu->hvLeft = apu->hvRight = 0;
	apu->hvChannel = 0;
}

void wsapu_set_ram(WSAPU* apu, uint8_t* ram, uint32_t ramMask)
{
	apu->ram = ram;
	apu->ramMask = ramMask;
}

void wsapu_set_dma_reader(WSAPU* apu, wsapu_read_cb read, void* ctx)
{
	apu->read = read;
	apu->readCtx = ctx;
}

void wsapu_set_mute_mask(WSAPU* apu, uint32_t muteMask)
{
	apu->muteMask = (uint8_t)(muteMask & 0x0F);
}

// --- Hyper Voice -----------------------------------------------------------

// "8-bit values of the form Vvvv vvvv are transformed to signed 16-bit samples
// based on the scaling mode", then shifted right by the volume field. Mode 3
// ("ignored") leaves the top three bits undefined and is not shifted; it is
// treated as sign extension here, which is the closest defined behaviour.
static int16_t ws_hv_convert(WSAPU* apu, uint8_t sample)
{
	uint8_t mode = (uint8_t)((HVCTLL >> 2) & 0x03);
	uint8_t shift = (uint8_t)(HVCTLL & 0x03);
	uint32_t value;

	switch (mode)
	{
	case 0:	 value = (uint32_t)sample << 8; break;					// unsigned
	case 1:	 value = (uint32_t)(0xE000 | (sample << 8)); break;		// negated
	default: value = (uint32_t)(int32_t)((int8_t)sample << 8); break;	// signed
	}
	if (mode != 3) { value >>= shift; }

	return (int16_t)(value & 0xFFFF);
}

static void ws_hv_write_sample(WSAPU* apu, uint8_t sample)
{
	int16_t value = ws_hv_convert(apu, sample);
	uint8_t layout = (uint8_t)((HVCTLH >> 5) & 0x03);

	switch (layout)
	{
	case 1:	apu->hvLeft = value; break;					// mono, left only
	case 2:	apu->hvRight = value; break;				// mono, right only
	case 3:	apu->hvLeft = apu->hvRight = value; break;	// mono, both
	default:	// stereo: left then right, in sequence
		if (apu->hvChannel == 0) { apu->hvLeft = value; }
		else { apu->hvRight = value; }
		apu->hvChannel ^= 1;
		break;
	}
}

// --- sound DMA -------------------------------------------------------------

static void ws_dma_reload_rate(WSAPU* apu)
{
	uint16_t hz = WS_DMA_HZ[SDMACTL & 0x03];

	apu->dmaTickSmpls = (uint32_t)(((uint64_t)apu->rate << 16) / hz);
	if (apu->dmaTickSmpls == 0) { apu->dmaTickSmpls = 1 << 16; }
}

// One byte transferred. "If enabled, the offset/length counters will be paused,
// and $00 will be written on every Sound DMA tick" (hold); auto-repeat restores
// the shadowed offset and length when the counter reaches 0, otherwise the
// enable bit clears.
static void ws_dma_step(WSAPU* apu)
{
	uint8_t hold = (uint8_t)((SDMACTL >> 3) & 0x01);
	uint8_t repeat = (uint8_t)((SDMACTL >> 4) & 0x01);
	uint8_t target = (uint8_t)((SDMACTL >> 5) & 0x01);
	uint8_t decrement = (uint8_t)((SDMACTL >> 6) & 0x01);
	uint8_t sample = 0x00;

	if (!hold)
	{
		if (apu->read != NULL)
		{
			sample = apu->read(apu->readCtx, apu->dmaSource & 0xFFFFF);
		}
		apu->dmaSource = (apu->dmaSource + (decrement ? -1 : 1)) & 0xFFFFF;
		apu->dmaLength--;
	}

	if (target) { ws_hv_write_sample(apu, sample); }
	else { apu->ioRam[0x89] = sample; }

	if (!hold && apu->dmaLength == 0)
	{
		if (repeat)
		{
			apu->dmaSource = apu->dmaSourceShadow;
			apu->dmaLength = apu->dmaLengthShadow;
		}
		else
		{
			SDMACTL &= (uint8_t)~0x80;
		}
	}
}

// --- ports -----------------------------------------------------------------

void wsapu_write_port(WSAPU* apu, uint8_t port, uint8_t value)
{
	apu->ioRam[port] = value;

	switch (port)
	{
	case 0x80: case 0x81:
		ws_update_period(apu, 0);
		break;
	case 0x82: case 0x83:
		ws_update_period(apu, 1);
		break;
	case 0x84: case 0x85:
		ws_update_period(apu, 2);
		break;
	case 0x86: case 0x87:
		ws_update_period(apu, 3);
		break;
	case 0x8D:
		// "The above-mentioned internal counter is reset on every write."
		apu->sweepTicks = 0;
		apu->sweepAccum = 0;
		break;
	case 0x8E:
		if (value & 0x08)	// LFSR reset
		{
			apu->lfsr = 0x0000;
			PCSRL = 0x00;
			PCSRH = 0x00;
		}
		break;

	// Sound DMA source and length: "upon writing to any of the bytes, said byte
	// (and only said byte) is copied to a shadow register", and the live
	// counters are the visible ports, so both move together.
	case 0x4A: case 0x4B: case 0x4C:
		apu->dmaSource = (uint32_t)(apu->ioRam[0x4A] | (apu->ioRam[0x4B] << 8) |
		                            ((apu->ioRam[0x4C] & 0x0F) << 16));
		apu->dmaSourceShadow = apu->dmaSource;
		break;
	case 0x4E: case 0x4F: case 0x50:
		apu->dmaLength = (uint32_t)(apu->ioRam[0x4E] | (apu->ioRam[0x4F] << 8) |
		                            ((apu->ioRam[0x50] & 0x0F) << 16));
		apu->dmaLengthShadow = apu->dmaLength;
		break;
	case 0x52:
		ws_dma_reload_rate(apu);
		// "Sound DMA enable will fail if the length is set to 0."
		if ((value & 0x80) && apu->dmaLength == 0)
		{
			apu->ioRam[0x52] = (uint8_t)(value & ~0x80);
		}
		apu->dmaAccum = 0;
		break;

	case 0x69:	// Hyper Voice input, written by hand rather than by DMA
		ws_hv_write_sample(apu, value);
		break;
	case 0x6B:
		if (value & 0x10)	// reset: the next DMA sample is the left channel
		{
			apu->hvChannel = 0;
		}
		break;
	default:
		break;
	}
}

uint8_t wsapu_read_port(WSAPU* apu, uint8_t port)
{
	switch (port)
	{
	// The live sound-DMA counters are visible at their ports.
	case 0x4A: return (uint8_t)(apu->dmaSource & 0xFF);
	case 0x4B: return (uint8_t)((apu->dmaSource >> 8) & 0xFF);
	case 0x4C: return (uint8_t)((apu->dmaSource >> 16) & 0x0F);
	case 0x4E: return (uint8_t)(apu->dmaLength & 0xFF);
	case 0x4F: return (uint8_t)((apu->dmaLength >> 8) & 0xFF);
	case 0x50: return (uint8_t)((apu->dmaLength >> 16) & 0x0F);
	default:   return apu->ioRam[port];
	}
}

// --- mix -------------------------------------------------------------------

// Channel 3's sweep tick: SNDSWP (signed) is added to the 11-bit divisor every
// SWPSTP+1 ticks of the 375 Hz clock, wrapping rather than clamping. The swept
// value is written back into the register file, so a later write from the music
// driver still wins.
static void ws_step_sweep(WSAPU* apu)
{
	uint16_t divisor;

	// A channel parked at $FFFF is silent by the in_wsr rule; sweeping it would
	// destroy that marker and bring it back.
	if (apu->ch[2].silent) { return; }

	if (apu->sweepTicks < (SWPSTP & 0x1F))
	{
		apu->sweepTicks++;
		return;
	}
	apu->sweepTicks = 0;

	divisor = (uint16_t)((((apu->ioRam[0x85] << 8) | apu->ioRam[0x84]) +
	                      (int16_t)(int8_t)SNDSWP) & 0x7FF);
	apu->ioRam[0x84] = (uint8_t)(divisor & 0xFF);
	apu->ioRam[0x85] = (uint8_t)((apu->ioRam[0x85] & 0xF8) | (divisor >> 8));
	ws_update_period(apu, 2);
}

void wsapu_render(WSAPU* apu, int32_t* left, int32_t* right, uint32_t samples)
{
	uint32_t i;
	uint8_t n;

	for (i = 0; i < samples; i++)
	{
		int32_t sumL = 0, sumR = 0;
		uint8_t voiceOn = (uint8_t)((SNDMOD >> 5) & 0x01);
		uint8_t noiseOn = (uint8_t)((SNDMOD >> 7) & 0x01);
		uint8_t sweepOn = (uint8_t)((SNDMOD >> 6) & 0x01);

		for (n = 0; n < WSAPU_CHANNELS; n++)
		{
			WSAPU_CHANNEL* c = &apu->ch[n];
			uint8_t chOn = (uint8_t)((SNDMOD >> n) & 0x01);
			uint32_t inc, before, steps;
			uint8_t sample, vol;

			// Channel 2's voice mode does not need the wavetable enable bit;
			// every other mode does.
			if (n == 1 && voiceOn)
			{
				int32_t pcm, lvl;

				if (apu->muteMask & (1 << n)) { continue; }

				pcm = (int32_t)apu->ioRam[0x89];
				// $94: bit0 right 100%, bit1 right 50%, bit2 left 100%,
				// bit3 left 50%; the 50% bit is ignored when 100% is set.
				lvl = (PCVOL & 0x04) ? 2 : ((PCVOL & 0x08) ? 1 : 0);
				sumL += ((2 * pcm - 255) * lvl) >> 1;
				lvl = (PCVOL & 0x01) ? 2 : ((PCVOL & 0x02) ? 1 : 0);
				sumR += ((2 * pcm - 255) * lvl) >> 1;
				continue;
			}

			if (!chOn || c->silent) { continue; }

			// The enable bit gates the sample period counter too, so a disabled
			// channel does not advance its phase.
			inc = apu->cyclesPerSmpl / c->period;
			before = c->phase;
			c->phase += inc;
			// Wavetable steps crossed by this output sample -- taken BEFORE the
			// wrap below, which is what makes it a true count (at most 128).
			steps = (c->phase >> 16) - (before >> 16);
			// The phase MUST be wrapped into the 32-step table rather than left
			// to run free: a free-running 16.16 counter overflows 32 bits after
			// about a million samples, and the unsigned difference above then
			// reads as ~2^32 steps -- an effective hang some 40 seconds into
			// any noise-using track.
			c->phase &= (WS_WAVE_STEPS << 16) - 1;

			if (n == 3 && noiseOn)
			{
				// "For the LFSR register to advance, [both] the channel 4
				// enable bit and the LFSR enable bit have to be set" -- with
				// the latter clear the register holds, and its bit 0 still
				// drives the output.
				if (NSCTL & 0x10)
				{
					while (steps--) { ws_clock_noise(apu); }
				}
				sample = (uint8_t)((apu->lfsr & 0x01) ? 15 : 0);
			}
			else
			{
				sample = ws_wave_sample(apu, n,
				                        (uint8_t)((c->phase >> 16) & 0x1F));
			}

			if (apu->muteMask & (1 << n)) { continue; }

			// Volume is read live from $88-$8B rather than cached, because $89
			// is BOTH channel 2's volume and its voice sample -- which of the
			// two a write means is only decided by $90's voice bit, at mix time.
			vol = apu->ioRam[0x88 + n];
			// Half units: (2 * sample - 15) is zero at mid scale.
			sumL += (2 * (int32_t)sample - 15) * (int32_t)((vol >> 4) & 0x0F);
			sumR += (2 * (int32_t)sample - 15) * (int32_t)(vol & 0x0F);
		}

		// Hyper Voice joins AFTER the channel sum is scaled, at its own 16-bit
		// level -- see WS_OUTPUT_GAIN.
		left[i] = sumL * WS_OUTPUT_GAIN;
		right[i] = sumR * WS_OUTPUT_GAIN;
		if (HVCTLL & 0x80)
		{
			left[i] += apu->hvLeft;
			right[i] += apu->hvRight;
		}

		// --- 375 Hz sweep clock, counted in output samples ------------------
		if (sweepOn && ((SNDMOD >> 2) & 0x01))
		{
			apu->sweepAccum += 1 << 16;
			while (apu->sweepAccum >= apu->sweepTickSmpls)
			{
				apu->sweepAccum -= apu->sweepTickSmpls;
				ws_step_sweep(apu);
			}
		}

		// --- sound DMA, likewise --------------------------------------------
		if (SDMACTL & 0x80)
		{
			if (apu->dmaTickSmpls == 0) { ws_dma_reload_rate(apu); }
			apu->dmaAccum += 1 << 16;
			while ((SDMACTL & 0x80) && apu->dmaAccum >= apu->dmaTickSmpls)
			{
				apu->dmaAccum -= apu->dmaTickSmpls;
				ws_dma_step(apu);
			}
		}
	}
}
