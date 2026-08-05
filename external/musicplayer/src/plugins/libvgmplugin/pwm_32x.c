// Sega 32X PWM sound unit -- chipmachine's own implementation.
//
// Replaces libvgm's emu/cores/pwm.c, which is Gens code (Stephane Dallongeville
// / Stephane Akhoun / David Korth) and GPL-2 by descent, and was the last GPL
// core standing between the Mac App Store build and a GPL-free libvgm slice for
// this chip. Written against the documented behaviour of the 32X PWM registers
// rather than derived from any existing emulator, and shipped to BOTH variants
// -- same approach as the VIC-I replacement in vtplugin. See LEGAL / LEGAL-PLUS
// and the plugin README.
//
// WHAT THE HARDWARE DOES
//
// The 32X sound hardware is not a synthesiser: it is a pulse-width-modulated
// DAC fed by the SH-2s. Every PWM period the unit latches one pulse width per
// channel and emits it; the width IS the sample. So "emulating" it is mostly a
// matter of getting the timing and the centre point right.
//
//   reg 0  PWM_CTRL   output mode bits + timer interval (see below)
//   reg 1  PWM_CYCLE  period, in ticks of the 32X clock (23011360 Hz on NTSC).
//                     The output sample rate is therefore clock / cycle.
//   reg 2  PWM_LEFT   push a pulse width into the left FIFO
//   reg 3  PWM_RIGHT  push a pulse width into the right FIFO
//   reg 4  PWM_MONO   push the same width into both FIFOs
//
// Each channel has a short hardware FIFO (3 entries) that the unit drains one
// entry per period; the SH-2 side keeps it topped up. A VGM log replays those
// writes at their original spacing, so with the device running at clock/cycle
// the natural model is one pop per output sample, holding the last value when
// the log has not supplied a new one.
//
// A pulse width is a 12-bit number spanning the period, so silence is cycle/2
// and the signal swings either side of it. That is the only conversion needed:
// sample = width - cycle/2, scaled to fill 16 bits.
//
// PWM_CTRL is deliberately NOT acted on. Its low bits select the output mode
// per side and its bits 8-11 set the SH-2 timer interval, which is an interrupt
// concern and irrelevant to replay. Every 32X VGM in the corpus writes it
// exactly once, with 0x100 -- timer interval 1, mode bits clear -- and expects
// stereo sound out of both channels regardless, so honouring a "mode 0 = mute"
// reading of those bits would silence the entire platform. The register is
// latched for completeness and ignored.

#include <stdlib.h>
#include <string.h>

#include "../../../../zxtune/3rdparty/vgm/stdtype.h"
#include "../../../../zxtune/3rdparty/vgm/emu/EmuStructs.h"
#include "../../../../zxtune/3rdparty/vgm/emu/SoundDevs.h"
#include "../../../../zxtune/3rdparty/vgm/emu/snddef.h"
#include "../../../../zxtune/3rdparty/vgm/emu/EmuHelper.h"
#include "../../../../zxtune/3rdparty/vgm/emu/cores/pwm.h"

#define FCC_CHPM	0x4348504D	// "CHPM" -- chipmachine's own core

// Hardware FIFO depth, per channel.
#define PWM_FIFO_LEN	3

// Power-on period. The hardware comes up with the cycle register clear, which
// would be a divide by zero; every log writes a real value before its first
// sample, so this only has to be sane enough to report a starting rate. 1045
// puts it at the ~22 kHz that 32X titles typically run.
#define PWM_DEFAULT_CYCLE	1045

typedef struct
{
	DEV_DATA _devData;

	UINT32 clock;
	UINT32 rate;
	UINT16 cycle;
	UINT16 ctrl;

	UINT16 fifo[2][PWM_FIFO_LEN];
	UINT8 fifoCnt[2];
	INT32 last[2];			// held between pops
	UINT8 muteMask;

	DEVCB_SRATE_CHG SmpRateFunc;
	void* SmpRateData;
} PWM_CHIP;

static UINT8 device_start_pwm_cm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf);
static void device_stop_pwm_cm(void* chipPtr);
static void device_reset_pwm_cm(void* chipPtr);
static void pwm_cm_update(void* chipPtr, UINT32 samples, DEV_SMPL** outputs);
static void pwm_cm_write(void* chipPtr, UINT8 offset, UINT16 data);
static void pwm_cm_set_mute_mask(void* chipPtr, UINT32 muteMask);
static void pwm_cm_set_srchg_cb(void* chipPtr, DEVCB_SRATE_CHG cbFunc, void* dataPtr);

static DEVDEF_RWFUNC devFunc_PWM_CM[] =
{
	{RWF_REGISTER | RWF_WRITE, DEVRW_A8D16, 0, (void*)pwm_cm_write},
	{RWF_CHN_MUTE | RWF_WRITE, DEVRW_ALL, 0, (void*)pwm_cm_set_mute_mask},
	{0x00, 0x00, 0, NULL}
};

static DEV_DEF devDef_PWM_CM =
{
	"PWM", "chipmachine", FCC_CHPM,

	device_start_pwm_cm,
	device_stop_pwm_cm,
	device_reset_pwm_cm,
	pwm_cm_update,

	NULL,	// SetOptionBits
	pwm_cm_set_mute_mask,
	NULL,	// SetPanning
	pwm_cm_set_srchg_cb,
	NULL,	// SetLoggingCallback
	NULL,	// LinkDevice

	devFunc_PWM_CM,
};

static const char* DeviceName(const DEV_GEN_CFG* devCfg)
{
	return "32X PWM";
}

static UINT16 DeviceChannels(const DEV_GEN_CFG* devCfg)
{
	return 2;
}

static const char** DeviceChannelNames(const DEV_GEN_CFG* devCfg)
{
	static const char* names[2] = { "Left", "Right" };
	return names;
}

static const DEVLINK_IDS* DeviceLinkIDs(const DEV_GEN_CFG* devCfg)
{
	return NULL;
}

const DEV_DECL sndDev_32X_PWM =
{
	DEVID_32X_PWM,
	DeviceName,
	DeviceChannels,
	DeviceChannelNames,
	DeviceLinkIDs,
	{	// cores
		&devDef_PWM_CM,
		NULL
	}
};

static void pwm_cm_update_rate(PWM_CHIP* chip)
{
	UINT32 newRate = chip->cycle ? (chip->clock / chip->cycle) : 0;
	if (newRate == 0 || newRate == chip->rate) return;
	chip->rate = newRate;
	if (chip->SmpRateFunc != NULL)
		chip->SmpRateFunc(chip->SmpRateData, newRate);
}

static UINT8 device_start_pwm_cm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf)
{
	PWM_CHIP* chip = (PWM_CHIP*)calloc(1, sizeof(PWM_CHIP));
	if (chip == NULL) return 0xFF;

	chip->clock = cfg->clock;
	chip->cycle = PWM_DEFAULT_CYCLE;
	chip->rate = chip->clock / chip->cycle;
	SRATE_CUSTOM_HIGHEST(cfg->srMode, chip->rate, cfg->smplRate);

	pwm_cm_set_mute_mask(chip, 0x00);

	chip->_devData.chipInf = chip;
	INIT_DEVINF(retDevInf, &chip->_devData, chip->rate, &devDef_PWM_CM);
	return 0x00;
}

static void device_stop_pwm_cm(void* chipPtr)
{
	free((PWM_CHIP*)chipPtr);
}

static void device_reset_pwm_cm(void* chipPtr)
{
	PWM_CHIP* chip = (PWM_CHIP*)chipPtr;

	chip->ctrl = 0x0000;
	chip->cycle = PWM_DEFAULT_CYCLE;
	memset(chip->fifo, 0, sizeof(chip->fifo));
	memset(chip->fifoCnt, 0, sizeof(chip->fifoCnt));
	chip->last[0] = chip->last[1] = 0;
	pwm_cm_update_rate(chip);
}

static void pwm_cm_push(PWM_CHIP* chip, UINT8 ch, UINT16 width)
{
	// A full FIFO drops the write, as on hardware.
	if (chip->fifoCnt[ch] >= PWM_FIFO_LEN) return;
	chip->fifo[ch][chip->fifoCnt[ch]] = width;
	chip->fifoCnt[ch]++;
}

static void pwm_cm_write(void* chipPtr, UINT8 offset, UINT16 data)
{
	PWM_CHIP* chip = (PWM_CHIP*)chipPtr;

	data &= 0x0FFF;
	switch (offset & 0x07)
	{
	case 0:	// PWM_CTRL -- latched, not acted on (see the file header)
		chip->ctrl = data;
		break;
	case 1:	// PWM_CYCLE
		if (data != chip->cycle)
		{
			chip->cycle = data;
			pwm_cm_update_rate(chip);
		}
		break;
	case 2:	// left
		pwm_cm_push(chip, 0, data);
		break;
	case 3:	// right
		pwm_cm_push(chip, 1, data);
		break;
	case 4:	// mono -- same width to both sides
		pwm_cm_push(chip, 0, data);
		pwm_cm_push(chip, 1, data);
		break;
	default:
		break;
	}
}

static INT32 pwm_cm_pop(PWM_CHIP* chip, UINT8 ch)
{
	INT32 centre, width;

	if (chip->fifoCnt[ch] > 0)
	{
		width = chip->fifo[ch][0];
		chip->fifoCnt[ch]--;
		memmove(&chip->fifo[ch][0], &chip->fifo[ch][1], chip->fifoCnt[ch] * sizeof(UINT16));

		// The width spans the period, so the zero point is half of it, and the
		// usable swing is +/- cycle/2. Scale that to a 16-bit sample. Widths
		// outside the period are meaningless but do occur in logs; clamping
		// after the shift keeps them from wrapping.
		centre = chip->cycle >> 1;
		if (centre < 1) centre = 1;
		chip->last[ch] = ((width - centre) * 0x8000) / centre;
		if (chip->last[ch] > 0x7FFF) chip->last[ch] = 0x7FFF;
		else if (chip->last[ch] < -0x8000) chip->last[ch] = -0x8000;
	}
	// else: hold the previous value until the log supplies another.

	return (chip->muteMask & (1 << ch)) ? 0 : chip->last[ch];
}

static void pwm_cm_update(void* chipPtr, UINT32 samples, DEV_SMPL** outputs)
{
	PWM_CHIP* chip = (PWM_CHIP*)chipPtr;
	UINT32 i;

	for (i = 0; i < samples; i++)
	{
		outputs[0][i] = pwm_cm_pop(chip, 0);
		outputs[1][i] = pwm_cm_pop(chip, 1);
	}
}

static void pwm_cm_set_mute_mask(void* chipPtr, UINT32 muteMask)
{
	((PWM_CHIP*)chipPtr)->muteMask = (UINT8)(muteMask & 0x03);
}

static void pwm_cm_set_srchg_cb(void* chipPtr, DEVCB_SRATE_CHG cbFunc, void* dataPtr)
{
	PWM_CHIP* chip = (PWM_CHIP*)chipPtr;
	chip->SmpRateFunc = cbFunc;
	chip->SmpRateData = dataPtr;
}
