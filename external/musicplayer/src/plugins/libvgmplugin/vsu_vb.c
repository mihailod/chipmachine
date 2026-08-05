// Virtual Boy VSU (VSU-VUE) -- chipmachine's own implementation.
//
// Replaces libvgm's emu/cores/vsu.c, which is Mednafen code and GPL-2-or-later,
// and was the LAST GPL file in the Mac App Store build. Written from Nintendo's
// own Virtual Boy Development Manual (Part 6 "Sound Processor", chapters 2-4)
// rather than derived from any existing emulator, and shipped to BOTH variants
// -- same approach as pwm_32x.c here and the VIC-I in vtplugin.
//
// EVERY constant below comes from that manual. The relevant ones:
//
//   * Six sound sources. 1-5 play a 32-word, 6-bit waveform from one of five
//     waveform RAM banks; 5 adds sweep/modulation; 6 is noise.
//   * Pitch:  f = clock / ((2048 - F) * 32)          (clock = 5 MHz)
//     so the waveform advances one word every clock/(2048-F) seconds and the
//     32-word table is one cycle.
//   * Noise:  f = (clock/10) / (2048 - F)
//   * Output level = ((L/R level * envelope) >> 3) + 1, and 0 if EITHER is 0.
//     (Manual's own 16x16 table, 4.7: F x F -> 29, not 31.)
//   * Envelope step  = 15.36 ms * (N + 1)
//   * Sound interval = 3.84 ms  * (n + 1)     (counter clock 260.4 Hz)
//   * Sweep/mod interval = (0.96 or 7.68 ms) * N,  N = 0 turns it off
//   * Sweep: X(t) = X(t-1) +/- X(t-1) / 2^n, and the sound STOPS once the
//     running value passes 0x7FF.
//   * Modulation: signed 8-bit from modulation RAM added to the frequency each
//     interval; the frequency REGISTER itself is left alone.
//   * Noise is an M-type 15-stage shift register tapped between stage 8 and one
//     of 15, 11, 14, 5, 9, 7, 10, 12 (selected by S6EV1 bits 6-4).
//
// The timer periods are exact whole numbers of output samples at the real
// clock, which is a good sign they were derived from it: the chip runs at
// clock/120 = 41666.67 Hz, and 3.84 ms is 160 samples, 15.36 ms is 640, 0.96 ms
// is 40 and 7.68 ms is 320. They are still computed from the reported rate here
// so an unusual clock in a VGM header cannot desynchronise them.

#include <stdlib.h>
#include <string.h>

#include "../../../../zxtune/3rdparty/vgm/stdtype.h"
#include "../../../../zxtune/3rdparty/vgm/emu/EmuStructs.h"
#include "../../../../zxtune/3rdparty/vgm/emu/SoundDevs.h"
#include "../../../../zxtune/3rdparty/vgm/emu/snddef.h"
#include "../../../../zxtune/3rdparty/vgm/emu/EmuHelper.h"
#include "../../../../zxtune/3rdparty/vgm/emu/cores/vsu.h"

#define FCC_CHPM	0x4348504D	// "CHPM" -- chipmachine's own core

#define VSU_CHANNELS	6
#define VSU_WAVE_BANKS	5
#define VSU_WAVE_WORDS	32
#define VSU_MOD_WORDS	32

// Waveform samples are 6-bit UNSIGNED (0..63). Real hardware feeds them to an
// unsigned DAC and the analogue stage is AC-coupled, so the mid-scale value is
// silence; subtracting it here is that coupling. Without it, a channel sitting
// on a DC waveform would contribute a level-dependent offset and every volume
// change would thump.
#define VSU_WAVE_CENTRE	32

// Calibration: the summed 6 x (6-bit sample x 5-bit level) is nowhere near
// 16 bits. Measured against the Mednafen core over the corpus -- see README.
#define VSU_OUTPUT_SHIFT	3

typedef struct
{
	// --- registers, as written -------------------------------------------
	UINT8 enabled;		// SxINT D7
	UINT8 useInterval;	// SxINT D5
	UINT8 intervalCnt;	// SxINT D4-D0
	UINT8 volL, volR;	// SxLRV
	UINT16 freq;		// SxFQL + SxFQH, 11 bits
	UINT8 envInit;		// SxEV0 D7-D4
	UINT8 envGrow;		// SxEV0 D3
	UINT8 envStep;		// SxEV0 D2-D0
	UINT8 envEnable;	// SxEV1 D0
	UINT8 envRepeat;	// SxEV1 D1
	UINT8 waveBank;		// SxRAM D2-D0

	// --- SOUND 5 only -----------------------------------------------------
	UINT8 swpEnable;	// S5EV1 D6
	UINT8 swpRepeat;	// S5EV1 D5
	UINT8 swpModulate;	// S5EV1 D4  (0 = sweep, 1 = modulation)
	UINT8 swpClkSel;	// S5SWP D7
	UINT8 swpInterval;	// S5SWP D6-D4
	UINT8 swpDir;		// S5SWP D3  (0 = subtract, 1 = add)
	UINT8 swpShift;		// S5SWP D2-D0

	// --- SOUND 6 only -----------------------------------------------------
	UINT8 noiseTap;		// S6EV1 D6-D4

	// --- live state -------------------------------------------------------
	UINT32 phase;		// 16.16, index into the 32-word waveform
	UINT8 envValue;		// current 4-bit envelope
	UINT8 envDone;		// envelope finished, no longer stepping
	UINT16 sweepFreq;	// running frequency for sweep/modulation
	UINT8 modIndex;		// modulation RAM read position
	UINT16 lfsr;		// noise shift register, 15 bits
	UINT32 envCounter;	// all four counters are in OUTPUT SAMPLES
	UINT32 intCounter;
	UINT32 swpCounter;
} VSU_CHANNEL;

typedef struct
{
	DEV_DATA _devData;

	UINT32 clock;
	UINT32 rate;

	UINT8 waveRAM[VSU_WAVE_BANKS][VSU_WAVE_WORDS];	// 6-bit
	INT8 modRAM[VSU_MOD_WORDS];						// signed 8-bit
	VSU_CHANNEL ch[VSU_CHANNELS];
	UINT8 muteMask;

	// Timer periods, in output samples (see the file header).
	UINT32 envUnit;		// 15.36 ms
	UINT32 intUnit;		// 3.84 ms
	UINT32 swpUnit[2];	// 0.96 ms / 7.68 ms
} VSU_CHIP;

static UINT8 device_start_vsu_cm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf);
static void device_stop_vsu_cm(void* chipPtr);
static void device_reset_vsu_cm(void* chipPtr);
static void vsu_cm_update(void* chipPtr, UINT32 samples, DEV_SMPL** outputs);
static void vsu_cm_write(void* chipPtr, UINT16 offset, UINT8 data);
static void vsu_cm_set_mute_mask(void* chipPtr, UINT32 muteMask);

static DEVDEF_RWFUNC devFunc_VSU_CM[] =
{
	// VGMPlayer's default start branch binds the VSU through
	// RWF_REGISTER | DEVRW_A16D8 (command 0xC7, Cmd_Ofs16_Data8).
	{RWF_REGISTER | RWF_WRITE, DEVRW_A16D8, 0, (void*)vsu_cm_write},
	{RWF_CHN_MUTE | RWF_WRITE, DEVRW_ALL, 0, (void*)vsu_cm_set_mute_mask},
	{0x00, 0x00, 0, NULL}
};

static DEV_DEF devDef_VSU_CM =
{
	"VSU-VUE", "chipmachine", FCC_CHPM,

	device_start_vsu_cm,
	device_stop_vsu_cm,
	device_reset_vsu_cm,
	vsu_cm_update,

	NULL,	// SetOptionBits
	vsu_cm_set_mute_mask,
	NULL,	// SetPanning
	NULL,	// SetSampleRateChangeCallback
	NULL,	// SetLoggingCallback
	NULL,	// LinkDevice

	devFunc_VSU_CM,
};

static const char* DeviceName(const DEV_GEN_CFG* devCfg)
{
	return "VSU-VUE";
}

static UINT16 DeviceChannels(const DEV_GEN_CFG* devCfg)
{
	return VSU_CHANNELS;
}

static const char** DeviceChannelNames(const DEV_GEN_CFG* devCfg)
{
	static const char* names[VSU_CHANNELS] =
	{
		"1", "2", "3", "4", "5 (Sweep/Mod)", "6 (Noise)"
	};
	return names;
}

static const DEVLINK_IDS* DeviceLinkIDs(const DEV_GEN_CFG* devCfg)
{
	return NULL;
}

const DEV_DECL sndDev_VBoyVSU =
{
	DEVID_VBOY_VSU,
	DeviceName,
	DeviceChannels,
	DeviceChannelNames,
	DeviceLinkIDs,
	{	// cores
		&devDef_VSU_CM,
		NULL
	}
};

// Manual 4.17: tapped between a constant stage 8 and one of these.
static const UINT8 VSU_NOISE_TAP[8] = { 15, 11, 14, 5, 9, 7, 10, 12 };

static void vsu_cm_key_on(VSU_CHIP* chip, VSU_CHANNEL* c)
{
	// Manual 4.10: starting clears the frequency counter, the RAM address, the
	// interval counter, the envelope value and step counter, and the modulation
	// counters.
	c->phase = 0;
	c->intCounter = 0;
	c->envCounter = 0;
	c->swpCounter = 0;
	c->modIndex = 0;
	c->envValue = c->envInit;
	c->envDone = 0;
	c->sweepFreq = c->freq;
	c->lfsr = 0x7FFF;	// any non-zero seed; see vsu_cm_clock_noise()
}

// One step of the M-type shift register. The manual gives the tap table but not
// the seed or the feedback polarity; a maximal-length sequence needs a non-zero
// seed under XOR feedback, so the register is seeded all-ones at key-on. Any
// maximal-length variant produces the same class of noise, and the tap position
// -- which is what the manual specifies and what actually colours the sound --
// is exact.
static void vsu_cm_clock_noise(VSU_CHANNEL* c)
{
	UINT8 tap = VSU_NOISE_TAP[c->noiseTap & 0x07];
	UINT16 fb = ((c->lfsr >> 7) ^ (c->lfsr >> (tap - 1))) & 0x01;
	c->lfsr = (UINT16)(((c->lfsr << 1) | fb) & 0x7FFF);
}

// Manual 4.6/4.7. Zero if either input is zero -- the +1 is NOT applied then.
static INT32 vsu_cm_level(UINT8 vol, UINT8 env)
{
	if (vol == 0 || env == 0) return 0;
	return (INT32)(((vol * env) >> 3) + 1);
}

static UINT16 vsu_cm_effective_freq(VSU_CHIP* chip, VSU_CHANNEL* c, int isCh5)
{
	if (!isCh5 || !c->swpEnable) return c->freq;
	if (c->swpModulate)
	{
		// Manual 4.14/4.16: the modulation value is added to the REGISTER value
		// each time; the register is not modified.
		INT32 f = (INT32)c->freq + chip->modRAM[c->modIndex & 0x1F];
		if (f < 0) f = 0;
		if (f > 0x7FF) f = 0x7FF;
		return (UINT16)f;
	}
	return c->sweepFreq;
}

static void vsu_cm_step_sweep(VSU_CHIP* chip, VSU_CHANNEL* c)
{
	if (c->swpModulate)
	{
		c->modIndex++;
		if (c->modIndex >= VSU_MOD_WORDS)
		{
			// D5: repeat the 32-word table, or hold at the last value.
			if (c->swpRepeat) c->modIndex = 0;
			else c->modIndex = VSU_MOD_WORDS - 1;
		}
	}
	else
	{
		// Manual: X(t) = X(t-1) +/- X(t-1) / 2^n, cumulative.
		INT32 f = c->sweepFreq;
		INT32 delta = f >> c->swpShift;
		f = c->swpDir ? (f + delta) : (f - delta);
		if (f > 0x7FF)
		{
			// "Sound output is stopped when the added value exceeds 7FFH."
			c->enabled = 0;
			c->sweepFreq = 0x7FF;
			return;
		}
		if (f < 0) f = 0;
		c->sweepFreq = (UINT16)f;
	}
}

static void vsu_cm_step_envelope(VSU_CHANNEL* c)
{
	if (!c->envEnable || c->envDone) return;

	if (c->envGrow)
	{
		if (c->envValue < 0x0F) c->envValue++;
		else if (c->envRepeat) c->envValue = c->envInit;
		else c->envDone = 1;		// hold at maximum
	}
	else
	{
		if (c->envValue > 0) c->envValue--;
		else if (c->envRepeat) c->envValue = c->envInit;
		else c->envDone = 1;
		// NOTE: a decay that reaches 0 must NOT clear `enabled`. The manual says
		// the output "is stopped", but it also says outright that "when the
		// envelope value is 0, the sound is still being output at level 0, and
		// the sound output is not considered to have stopped" -- level 0 is
		// already silence, so nothing is gained by gating the channel, and
		// gating it breaks a real idiom: with automatic enveloping ON, games
		// drive SxEV0 as a real-time volume (the manual describes this too), so
		// a channel that has decayed to 0 must come straight back when the next
		// SxEV0 write lands. Killing `enabled` here made one track render at
		// 0.40x the reference while everything else measured 1.00.
	}
}

static UINT8 device_start_vsu_cm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf)
{
	VSU_CHIP* chip = (VSU_CHIP*)calloc(1, sizeof(VSU_CHIP));
	if (chip == NULL) return 0xFF;

	chip->clock = cfg->clock;
	// The chip emits one sample every 120 clocks (5 MHz -> 41666.67 Hz).
	chip->rate = chip->clock / 120;
	if (chip->rate == 0) chip->rate = 1;
	SRATE_CUSTOM_HIGHEST(cfg->srMode, chip->rate, cfg->smplRate);

	chip->envUnit = (UINT32)(((UINT64)chip->rate * 1536) / 100000);	// 15.36 ms
	chip->intUnit = (UINT32)(((UINT64)chip->rate * 384) / 100000);	//  3.84 ms
	chip->swpUnit[0] = (UINT32)(((UINT64)chip->rate * 96) / 100000);	//  0.96 ms
	chip->swpUnit[1] = (UINT32)(((UINT64)chip->rate * 768) / 100000);// 7.68 ms
	if (chip->envUnit == 0) chip->envUnit = 1;
	if (chip->intUnit == 0) chip->intUnit = 1;
	if (chip->swpUnit[0] == 0) chip->swpUnit[0] = 1;
	if (chip->swpUnit[1] == 0) chip->swpUnit[1] = 1;

	vsu_cm_set_mute_mask(chip, 0x00);

	chip->_devData.chipInf = chip;
	INIT_DEVINF(retDevInf, &chip->_devData, chip->rate, &devDef_VSU_CM);
	return 0x00;
}

static void device_stop_vsu_cm(void* chipPtr)
{
	free((VSU_CHIP*)chipPtr);
}

static void device_reset_vsu_cm(void* chipPtr)
{
	VSU_CHIP* chip = (VSU_CHIP*)chipPtr;
	UINT8 i;

	memset(chip->waveRAM, 0, sizeof(chip->waveRAM));
	memset(chip->modRAM, 0, sizeof(chip->modRAM));
	memset(chip->ch, 0, sizeof(chip->ch));
	for (i = 0; i < VSU_CHANNELS; i++)
		chip->ch[i].lfsr = 0x7FFF;
}

static void vsu_cm_write(void* chipPtr, UINT16 offset, UINT8 data)
{
	VSU_CHIP* chip = (VSU_CHIP*)chipPtr;
	VSU_CHANNEL* c;
	UINT8 chNum, reg;

	// IMPORTANT: the offset VGM carries for this chip (command 0xC7) is a WORD
	// INDEX, not the byte address from the manual. Every VSU register sits on a
	// four-byte boundary, so the log stores address>>2 and the range is 0..352 --
	// 352 being 0x580>>2, the SSTOP register, which is exactly the highest
	// offset real files write. Feeding the manual's byte addresses into this
	// function decodes nothing and the chip renders silence.
	//
	//    0..159   waveform RAM, 5 banks of 32 words   (0x000 + bank*0x80 + w*4)
	//  160..191   modulation RAM, 32 words            (0x280 + w*4)
	//  256..351   channel registers, 16 per channel   (0x400 + ch*0x40 + r*4)
	//       352   SSTOP                               (0x580)
	offset &= 0x7FFF;

	if (offset < 160)
	{
		// The manual forbids writing waveform RAM while sound is playing;
		// libvgm has an option for emulators that allow it anyway. Writes are
		// always taken here -- a VGM log only replays what the game did.
		chip->waveRAM[offset >> 5][offset & 0x1F] = data & 0x3F;
		return;
	}
	if (offset < 192)
	{
		chip->modRAM[offset - 160] = (INT8)data;
		return;
	}
	if (offset == 352)
	{
		// SSTOP: D0 = 1 stops every sound source.
		if (data & 0x01)
		{
			for (chNum = 0; chNum < VSU_CHANNELS; chNum++)
				chip->ch[chNum].enabled = 0;
		}
		return;
	}
	if (offset < 256 || offset > 351) return;

	chNum = (UINT8)((offset - 256) >> 4);
	if (chNum >= VSU_CHANNELS) return;
	c = &chip->ch[chNum];
	reg = (UINT8)((offset - 256) & 0x0F);

	switch (reg)
	{
	case 0:	// SxINT
		c->useInterval = (data >> 5) & 0x01;
		c->intervalCnt = data & 0x1F;
		if (data & 0x80)
		{
			// Manual 4.10: re-triggering mid-note restarts everything EXCEPT
			// the envelope value, which vsu_cm_key_on would reload -- so keep
			// the current value when the channel was already running.
			UINT8 wasOn = c->enabled;
			UINT8 keepEnv = c->envValue;
			c->enabled = 1;
			vsu_cm_key_on(chip, c);
			if (wasOn) c->envValue = keepEnv;
		}
		else
		{
			c->enabled = 0;
		}
		break;
	case 1:	// SxLRV
		c->volL = (data >> 4) & 0x0F;
		c->volR = data & 0x0F;
		break;
	case 2:	// SxFQL
		c->freq = (UINT16)((c->freq & 0x700) | data);
		break;
	case 3:	// SxFQH
		c->freq = (UINT16)((c->freq & 0x0FF) | ((data & 0x07) << 8));
		break;
	case 4:	// SxEV0
		c->envInit = (data >> 4) & 0x0F;
		c->envGrow = (data >> 3) & 0x01;
		c->envStep = data & 0x07;
		// Manual 4.13 note: writing the initial value during automatic
		// enveloping restarts the envelope immediately from that value.
		c->envValue = c->envInit;
		c->envDone = 0;
		c->envCounter = 0;
		break;
	case 5:	// SxEV1
		c->envEnable = data & 0x01;
		c->envRepeat = (data >> 1) & 0x01;
		if (chNum == 4)
		{
			c->swpEnable = (data >> 6) & 0x01;
			c->swpRepeat = (data >> 5) & 0x01;
			c->swpModulate = (data >> 4) & 0x01;
		}
		else if (chNum == 5)
		{
			c->noiseTap = (data >> 4) & 0x07;
		}
		break;
	case 6:	// SxRAM
		c->waveBank = data & 0x07;
		if (c->waveBank >= VSU_WAVE_BANKS) c->waveBank = VSU_WAVE_BANKS - 1;
		break;
	case 7:	// S5SWP (SOUND 5 only)
		if (chNum == 4)
		{
			c->swpClkSel = (data >> 7) & 0x01;
			c->swpInterval = (data >> 4) & 0x07;
			c->swpDir = (data >> 3) & 0x01;
			c->swpShift = data & 0x07;
		}
		break;
	default:
		break;
	}
}

static void vsu_cm_update(void* chipPtr, UINT32 samples, DEV_SMPL** outputs)
{
	VSU_CHIP* chip = (VSU_CHIP*)chipPtr;
	UINT32 i;
	UINT8 n;

	for (i = 0; i < samples; i++)
	{
		INT32 sumL = 0, sumR = 0;

		for (n = 0; n < VSU_CHANNELS; n++)
		{
			VSU_CHANNEL* c = &chip->ch[n];
			int isCh5 = (n == 4), isNoise = (n == 5);
			UINT16 f;
			UINT32 inc;
			INT32 wave, lvlL, lvlR;

			if (!c->enabled || (chip->muteMask & (1 << n))) continue;

			f = vsu_cm_effective_freq(chip, c, isCh5);
			if (f > 0x7FF) f = 0x7FF;

			// Phase advance per output sample, 16.16. For the wave channels the
			// table steps at clock/(2048-F); the noise register clocks at
			// (clock/10)/(2048-F).
			{
				UINT32 div = 2048 - f;
				UINT64 stepHz = isNoise ? (chip->clock / 10) : chip->clock;
				inc = (UINT32)(((stepHz << 16) / div) / chip->rate);
			}

			if (isNoise)
			{
				UINT32 before = c->phase;
				c->phase += inc;
				// Clock the shift register once per whole step crossed.
				{
					UINT32 steps = (c->phase >> 16) - (before >> 16);
					while (steps--) vsu_cm_clock_noise(c);
				}
				wave = (c->lfsr & 0x01) ? 0x3F : 0x00;
			}
			else
			{
				c->phase += inc;
				wave = chip->waveRAM[c->waveBank][(c->phase >> 16) & 0x1F];
			}

			lvlL = vsu_cm_level(c->volL, c->envValue);
			lvlR = vsu_cm_level(c->volR, c->envValue);

			sumL += (wave - VSU_WAVE_CENTRE) * lvlL;
			sumR += (wave - VSU_WAVE_CENTRE) * lvlR;
		}

		outputs[0][i] = (DEV_SMPL)(sumL << VSU_OUTPUT_SHIFT);
		outputs[1][i] = (DEV_SMPL)(sumR << VSU_OUTPUT_SHIFT);

		// --- timers, all counted in output samples ---------------------------
		for (n = 0; n < VSU_CHANNELS; n++)
		{
			VSU_CHANNEL* c = &chip->ch[n];
			if (!c->enabled) continue;

			// Auto-deactivate after the programmed interval (SxINT D5).
			if (c->useInterval)
			{
				c->intCounter++;
				if (c->intCounter >= chip->intUnit * (UINT32)(c->intervalCnt + 1))
				{
					c->enabled = 0;
					continue;
				}
			}

			if (c->envEnable && !c->envDone)
			{
				c->envCounter++;
				if (c->envCounter >= chip->envUnit * (UINT32)(c->envStep + 1))
				{
					c->envCounter = 0;
					vsu_cm_step_envelope(c);
				}
			}

			if (n == 4 && c->swpEnable && c->swpInterval != 0)
			{
				c->swpCounter++;
				if (c->swpCounter >= chip->swpUnit[c->swpClkSel] * (UINT32)c->swpInterval)
				{
					c->swpCounter = 0;
					vsu_cm_step_sweep(chip, c);
				}
			}
		}
	}
}

static void vsu_cm_set_mute_mask(void* chipPtr, UINT32 muteMask)
{
	((VSU_CHIP*)chipPtr)->muteMask = (UINT8)(muteMask & 0x3F);
}
