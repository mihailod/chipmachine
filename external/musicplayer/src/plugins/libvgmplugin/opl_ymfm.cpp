// ymfm OPL cores for libvgm -- YM3526 (OPL1), Y8950 (MSX-AUDIO) and YMF278B (OPL4).
//
// These are the last three chips whose only libvgm core is GPL:
//
//   * YM3526 and Y8950 are served by emu/cores/fmopl.c, MAME-derived and
//     GPL-2.0+. (Its third chip, the YM3812/OPL2, is already unused -- 262intf's
//     sibling oplintf.c lists AdLibEmu first, "because it's better than MAME".)
//   * YMF278B is served by emu/cores/ymf278b.c, the MAME OPL4, GPL-2.0.
//
// So the Mac App Store variant could not drop either file without losing every
// YM3526, Y8950 and OPL4 song. This file wraps Aaron Giles' ymfm (BSD-3,
// vendored at external/ymfm) in libvgm's C DEV_DEF interface, the same way
// opn_ymfm.cpp does for the OPN family; read that file first, it carries the
// general notes. See external/ymfm/PROVENANCE.md for the vendoring.
//
// Sample rate needs no correction here, unlike the OPN: ymfm's OPL registers
// use prescale 4 over 18 operators, so fm_engine_base::sample_rate() returns
// clock/72 -- exactly what oplintf.c reports for all three OPL devices. ymfm's
// ymf278b likewise returns clock/768, matching ymf278b.c. Nothing lands near the
// ~60 kHz ceiling above which libvgm's resampler misbehaves.

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "../../../../zxtune/3rdparty/vgm/stdtype.h"

extern "C" {
#include "../../../../zxtune/3rdparty/vgm/emu/SoundDevs.h"
#include "../../../../zxtune/3rdparty/vgm/emu/EmuStructs.h"
#include "../../../../zxtune/3rdparty/vgm/emu/EmuCores.h"
#include "../../../../zxtune/3rdparty/vgm/emu/snddef.h"
#include "../../../../zxtune/3rdparty/vgm/emu/EmuHelper.h"
#include "../../../../zxtune/3rdparty/vgm/emu/SoundEmu.h"	// SndEmu_GetDeviceFunc
}

#include "ymfm.h"
#include "ymfm_opl.h"
#include "ymfm_rom_intf.h"
#include "cm_yrw801.h"
// NOTE: deliberately NOT including ymfm_fm.ipp -- see the same note in
// opn_ymfm.cpp. ymfm_opl.cpp already instantiates the fm_engine templates; a
// second copy in this TU cannot be merged once `ld -r` has made those weak
// symbols private externs. Calls to fm_engine_base<> members from here compile
// to ordinary out-of-line calls that link against ymfm_opl.cpp's instantiation.

#define FCC_YMFM	0x594D464D	// "YMFM"

#define LINKDEV_OPL3	0x00

namespace
{

using cm_ymfm::rom_intf;

// ------------------------------------------------------- YM3526 / Y8950

// Both are mono (opl_registers::OUTPUTS == 1) and libvgm feeds the one lane to
// both output channels, exactly as fmopl.c's ym3526_update_one/y8950_update_one
// do (`bufL[i] = bufR[i] = lt`).
template <typename ChipT>
struct opl_chip
{
	DEV_DATA _devData;		// must stay first -- libvgm casts to it
	rom_intf* intf;
	ChipT* chip;
	UINT32 rate;
};

template <typename ChipT>
static opl_chip<ChipT>* opl_create(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf, DEV_DEF* devDef)
{
	opl_chip<ChipT>* info = (opl_chip<ChipT>*)calloc(1, sizeof(opl_chip<ChipT>));
	if (info == NULL) return NULL;

	info->intf = new rom_intf();
	info->chip = new ChipT(*info->intf);
	info->rate = info->chip->sample_rate(cfg->clock);
	SRATE_CUSTOM_HIGHEST(cfg->srMode, info->rate, cfg->smplRate);

	INIT_DEVINF(retDevInf, &info->_devData, info->rate, devDef);
	return info;
}

template <typename ChipT>
static void opl_stop(void* chipPtr)
{
	opl_chip<ChipT>* info = (opl_chip<ChipT>*)chipPtr;
	delete info->chip;
	delete info->intf;
	free(info);
}

template <typename ChipT>
static void opl_reset(void* chipPtr)
{
	((opl_chip<ChipT>*)chipPtr)->chip->reset();
}

template <typename ChipT>
static UINT8 opl_write(void* chipPtr, UINT8 offset, UINT8 data)
{
	((opl_chip<ChipT>*)chipPtr)->chip->write(offset, data);
	return 0x00;
}

template <typename ChipT>
static UINT8 opl_read(void* chipPtr, UINT8 offset)
{
	return ((opl_chip<ChipT>*)chipPtr)->chip->read(offset);
}

template <typename ChipT>
static void opl_update(void* chipPtr, UINT32 samples, DEV_SMPL** outputs)
{
	opl_chip<ChipT>* info = (opl_chip<ChipT>*)chipPtr;
	typename ChipT::output_data out;
	for (UINT32 i = 0; i < samples; i++)
	{
		info->chip->generate(&out, 1);
		outputs[0][i] = out.data[0];
		outputs[1][i] = out.data[0];
	}
}

// Y8950 ADPCM. libvgm's rwFuncs for this chip use user code 0 for the DEVRW_BLOCK
// / DEVRW_MEMSIZE pair (there is only one region), and ymfm reads it back through
// ymfm_external_read(ACCESS_ADPCM_B, ...).
static void y8950_alloc_rom(void* chipPtr, UINT32 memsize)
{
	((opl_chip<ymfm::y8950>*)chipPtr)->intf->alloc(cm_ymfm::REGION_ADPCM_B, memsize);
}
static void y8950_write_rom(void* chipPtr, UINT32 offset, UINT32 length, const UINT8* data)
{
	((opl_chip<ymfm::y8950>*)chipPtr)->intf->write(cm_ymfm::REGION_ADPCM_B, offset, length, data);
}

// ------------------------------------------------------------- YMF278B

// libvgm does NOT model the OPL4 as one chip. emu/cores/ymf278b.c is the
// wavetable (PCM) half only; the FM half is a SEPARATE linked device -- its
// DeviceLinkIDs returns {1, {DEVID_YMF262}}, and VGMPlayer then hard-requests
// FCC_ADLE for it, so the FM is always AdLibEmu (LGPL-2.1, already compiled into
// both variants). ymf278b.c forwards register ports 0-3 to that device and keeps
// only ports 4/5 for itself.
//
// ymfm's ymf278b has its own internal FM engine and no fm_override hook, so
// there are two ways to wire this up. Both were built and measured; this adapter
// takes the second:
//
//   (a) Let ymfm do the FM and do not declare the link. One device instead of
//       two -- which changes the mix, because libvgm's _CHIP_VOLUME table
//       (player/vgmplayer.cpp) holds one volume per chip TYPE and applies it per
//       device: the FM would move from the YMF262 entry to the YMF278B entry,
//       and the OPL3 emulation would change from AdLibEmu to ymfm at the same
//       time. Two variables at once, for no gain.
//
//   (b) Keep libvgm's structure exactly: declare the YMF262 link, forward ports
//       0-3 to it, and emit only ymfm's PCM. plus and mas then share the same FM
//       core and the same two _CHIP_VOLUME entries, and the only thing that
//       changes is the wavetable engine -- which is the whole point.
//
// The catch in (b) is that ymfm's PCM writes are gated on the FM register file:
// write_data_pcm() returns early unless NEW2 is set, and NEW2 arrives through an
// FM port. So ports 0-3 have to go to BOTH the linked YMF262 (for sound) and to
// ymfm (for its register state) -- and ymfm's FM output then has to be dropped,
// or the FM would be rendered twice. ymf278b::generate() mixes FM and PCM into
// lanes 4/5 before we can see them, so opl4_pcm subclasses it and reimplements
// generate() to clock both engines identically but emit the PCM alone.
class opl4_pcm : public ymfm::ymf278b
{
public:
	explicit opl4_pcm(ymfm::ymfm_interface& intf) : ymfm::ymf278b(intf) {}

	// The PCM mix level (register 0xF9): attenuation in -3 dB steps, as a .11
	// gain. This is ymfm's own s_mix_scale, which generate() would have applied.
	static INT32 mix_scale(uint32_t level)
	{
		static const INT32 scale[8] = { 0x7fa, 0x5a4, 0x3fd, 0x2d2, 0x1fe, 0x169, 0xff, 0 };
		return scale[level & 7];
	}

	// PCM register writes are ignored until the FM's NEW2 bit is set.
	bool new2() { return m_fm.regs().new2flag() != 0; }

	// Same loop as ymfm::ymf278b::generate(), minus the FM contribution.
	//
	// The FM engine is still clocked at the identical cadence -- it drives the
	// timers and the BUSY/LD status bits that a log can poll, and skipping its
	// clock would desynchronise the PCM engine's envelope timing. Only its
	// OUTPUT is discarded, because the linked AdLibEmu YMF262 is producing it.
	//
	// The FM runs at clock/(19*36) while the chip outputs at clock/768, i.e.
	// 192/171 FM ticks per sample; ymfm carries the remainder in m_fm_pos and
	// ticks an extra time when it passes 171. Those two constants are private in
	// ymfm::ymf278b, so they are restated here.
	void generate_pcm(DEV_SMPL* outL, DEV_SMPL* outR, UINT32 numsamples)
	{
		static const uint32_t FM_EXTRA_SAMPLE_THRESH = 171;
		static const uint32_t FM_EXTRA_SAMPLE_STEP = 192 - 171;

		INT32 const pcm_l = mix_scale(m_pcm.regs().mix_pcm_l());
		INT32 const pcm_r = mix_scale(m_pcm.regs().mix_pcm_r());
		for (UINT32 samp = 0; samp < numsamples; samp++)
		{
			m_fm_pos += FM_EXTRA_SAMPLE_STEP;
			if (m_fm_pos >= FM_EXTRA_SAMPLE_THRESH)
			{
				m_fm.clock(fm_engine::ALL_CHANNELS);
				m_fm_pos -= FM_EXTRA_SAMPLE_THRESH;
			}
			m_fm.clock(fm_engine::ALL_CHANNELS);
			m_pcm.clock(ymfm::pcm_engine::ALL_CHANNELS);

			ymfm::pcm_engine::output_data pcmout;
			m_pcm.output(pcmout.clear(), ymfm::pcm_engine::ALL_CHANNELS);

			// Lanes 0/1 are the DO2 pin (the one that is mixed with the FM on
			// real hardware, and the only one a MoonSound uses), lanes 2/3 the
			// separate DO1 pin. libvgm sums all 24 slots regardless of that
			// routing, so sum both pairs here; the PCM mix level applies to the
			// DO2 pair only, as in ymfm's own generate().
			INT32 l = ((pcmout.data[0] * pcm_l) >> 11) + pcmout.data[2];
			INT32 r = ((pcmout.data[1] * pcm_r) >> 11) + pcmout.data[3];

			// LEVEL MATCH (x3/4), not a fudge, and MEASURED rather than derived.
			//
			// The two engines carry different internal headroom: libvgm's
			// ymf278b_pcm_update() ends each slot with a documented -15 dB trim
			// ("should bring it into balance with FM"), ymfm scales in its PCM
			// channel output instead, and the two do not cancel. libvgm's
			// _CHIP_VOLUME table (player/vgmplayer.cpp) is calibrated to
			// libvgm's level, so ymfm has to be brought onto it or the OPL4 sits
			// wrong against every other chip in a multi-chip VGM.
			//
			// Unscaled, ymfm measured 1.29x libvgm across 33 arcade tracks that
			// carry their own sample ROM (Strikers 1945 II, Gunbird 2); x3/4
			// puts the aggregate at 0.970, range 0.913-1.028, no clipping.
			// Do NOT recalibrate against MSX MoonSound rips -- they ship no ROM
			// data block and render as noise in both cores. See README.md.
			// Chasing a closed form is pointless: the residual is not a gain
			// error but the sample-interpolation difference, also in README.md.
			outL[samp] = (DEV_SMPL)((l * 3) >> 2);
			outR[samp] = (DEV_SMPL)((r * 3) >> 2);
		}

		if (m_load_remaining > 0)
			m_load_remaining -= (numsamples < m_load_remaining) ? numsamples : m_load_remaining;
	}
};

// The linked YMF262, reached exactly as ymf278b.c's OPL3FM does.
struct opl3_link
{
	void* chip;
	DEVFUNC_WRITE_A8D8 write;
	DEVFUNC_CTRL reset;
	DEVFUNC_WRITE_VOL_LR setVol;
};

struct opl4_chip
{
	DEV_DATA _devData;		// must stay first
	rom_intf* intf;
	opl4_pcm* chip;
	UINT32 rate;
	UINT8 port_C;
	// FM mix level, register 0xF8, three bits per side. Tracked HERE and not read
	// back out of ymfm's register file, because the two disagree on the power-on
	// value: ymf278b.c's device_reset sets fm_l = fm_r = 3, ymfm's regs default to
	// 0. Level 0 is +6 dB louder than level 3, so reading ymfm's copy would run
	// the linked YMF262 2.7x hot on any log that never writes 0xF8.
	UINT8 fm_l, fm_r;
	opl3_link fm;
};

static void opl3_dummy_write(void* param, UINT8 address, UINT8 data) { }
static void opl3_dummy_reset(void* param) { }

// The FM half is a separate libvgm device, so its share of the OPL4 mix cannot
// be applied to samples we produce -- it is passed on as a device volume, as
// ymf278b.c's refresh_opl3_volume() does.
//
// The ladder is the OPL4's -3 dB-per-step mix level with the two extremes
// (level 7 = mute) and libvgm's OPL4FM_VOL_BALANCE of 0x100 (i.e. x2) folded in,
// on libvgm's volume scale where 0x8000 is 100%. Each -3 dB step is the
// hardware's, approximated the way libvgm's vol_tab does it -- linear
// interpolation between the exact halvings, so a pair of steps is exactly x0.5:
//
//   level    0       1       2       3       4       5       6      7
//   gain   2.000   1.500   1.000   0.750   0.500   0.375   0.250   0
//
// Level 3 is the reset default, so an OPL4 log that never touches 0xF8 leaves
// the linked YMF262 at 0x6000 -- 75% -- exactly as in the plus build.
static INT32 fm_mix_volume(UINT8 level)
{
	static const INT32 vol[8] =
		{ 0x10000, 0xC000, 0x8000, 0x6000, 0x4000, 0x3000, 0x2000, 0x0000 };
	return vol[level & 7];
}

static void refresh_opl3_volume(opl4_chip* info)
{
	if (info->fm.setVol == NULL) return;
	info->fm.setVol(info->fm.chip, fm_mix_volume(info->fm_l), fm_mix_volume(info->fm_r));
}

static UINT8 opl4_write(void* chipPtr, UINT8 offset, UINT8 data)
{
	opl4_chip* info = (opl4_chip*)chipPtr;

	// Ports 0-3 are the FM half. They go to the linked YMF262, which actually
	// renders them, AND to ymfm, whose register file has to track NEW2 (PCM
	// writes are ignored until it is set) and the FM timers.
	if (offset < 4)
	{
		info->fm.write(info->fm.chip, offset, data);
		info->chip->write(offset, data);
		return 0x00;
	}
	if (offset == 4)
	{
		info->port_C = data;
		info->chip->write(4, data);
		return 0x00;
	}
	if (offset == 5)
	{
		info->chip->write(5, data);
		// ymfm ignores PCM writes until NEW2 is set; mirror that here so our copy
		// of the mix level cannot drift from the chip's.
		if (info->port_C == 0xF8 && info->chip->new2())
		{
			info->fm_l = data & 0x07;
			info->fm_r = (data >> 3) & 0x07;
			refresh_opl3_volume(info);
		}
		return 0x00;
	}
	return 0x00;
}

static UINT8 opl4_read(void* chipPtr, UINT8 offset)
{
	return ((opl4_chip*)chipPtr)->chip->read(offset);
}

static void opl4_update(void* chipPtr, UINT32 samples, DEV_SMPL** outputs)
{
	opl4_chip* info = (opl4_chip*)chipPtr;
	info->chip->generate_pcm(outputs[0], outputs[1], samples);
}

// Mirrors device_reset_ymf278b(): reset the linked FM too, and restore the mix
// levels to their power-on values (FM 3, PCM 0 -- the PCM side is ymfm's
// register default already).
static void opl4_reset(void* chipPtr)
{
	opl4_chip* info = (opl4_chip*)chipPtr;
	if (info->fm.reset != NULL) info->fm.reset(info->fm.chip);
	info->chip->reset();
	info->port_C = 0;
	info->fm_l = info->fm_r = 3;
	refresh_opl3_volume(info);
}

static void opl4_stop(void* chipPtr)
{
	opl4_chip* info = (opl4_chip*)chipPtr;
	delete info->chip;
	delete info->intf;
	free(info);
}

// A MoonSound log ships no ROM block, so the stub allocated at device start
// stands; a log that does ship one (every arcade OPL4 board) replaces it here,
// before any sample is rendered. rom_intf::alloc hands back the same zeroed
// buffer when the sizes happen to match, which is what a fresh alloc would give.
static void opl4_alloc_rom(void* chipPtr, UINT32 memsize)
{
	((opl4_chip*)chipPtr)->intf->alloc(cm_ymfm::REGION_PCM_ROM, memsize);
}
static void opl4_write_rom(void* chipPtr, UINT32 offset, UINT32 length, const UINT8* data)
{
	((opl4_chip*)chipPtr)->intf->write(cm_ymfm::REGION_PCM_ROM, offset, length, data);
}
static void opl4_alloc_ram(void* chipPtr, UINT32 memsize)
{
	((opl4_chip*)chipPtr)->intf->alloc(cm_ymfm::REGION_PCM_RAM, memsize);
}
static void opl4_write_ram(void* chipPtr, UINT32 offset, UINT32 length, const UINT8* data)
{
	((opl4_chip*)chipPtr)->intf->write(cm_ymfm::REGION_PCM_RAM, offset, length, data);
}

static UINT8 opl4_link_opl3(void* chipPtr, UINT8 linkID, const DEV_INFO* defInfOPL3)
{
	opl4_chip* info = (opl4_chip*)chipPtr;
	if (linkID != LINKDEV_OPL3) return EERR_UNK_DEVICE;

	if (defInfOPL3 == NULL)
	{
		info->fm.chip = NULL;
		info->fm.write = opl3_dummy_write;
		info->fm.reset = opl3_dummy_reset;
		info->fm.setVol = NULL;
		return 0x00;
	}

	if (SndEmu_GetDeviceFunc(defInfOPL3->devDef, RWF_REGISTER | RWF_WRITE, DEVRW_A8D8, 0, (void**)&info->fm.write))
		return 0xFF;
	if (SndEmu_GetDeviceFunc(defInfOPL3->devDef, RWF_VOLUME_LR | RWF_WRITE, DEVRW_VALUE, 0, (void**)&info->fm.setVol))
		info->fm.setVol = NULL;	// same as libvgm: a core without volume control is not fatal
	if (defInfOPL3->devDef->Reset == NULL) return 0xFF;
	info->fm.reset = defInfOPL3->devDef->Reset;
	info->fm.chip = defInfOPL3->dataPtr;
	refresh_opl3_volume(info);
	return 0x00;
}

// The OPL4's FM half runs at clock * 288/684; libvgm's init_opl3_devinfo()
// writes that as clock * 8 / 19. Reproduced exactly, or the linked YMF262 would
// be pitched wrong.
static void declare_opl3_link(DEV_INFO* devInf, const DEV_GEN_CFG* cfg)
{
	DEV_GEN_CFG* fmCfg = (DEV_GEN_CFG*)calloc(1, sizeof(DEV_GEN_CFG));
	if (fmCfg == NULL) return;
	*fmCfg = *cfg;
	fmCfg->clock = cfg->clock * 8 / 19;
	fmCfg->flags = 0x00;
	fmCfg->emuCore = 0;

	devInf->linkDevCount = 1;
	devInf->linkDevs = (DEVLINK_INFO*)calloc(1, sizeof(DEVLINK_INFO));
	if (devInf->linkDevs == NULL) { free(fmCfg); devInf->linkDevCount = 0; return; }
	devInf->linkDevs[0].devID = DEVID_YMF262;
	devInf->linkDevs[0].linkID = LINKDEV_OPL3;
	devInf->linkDevs[0].cfg = fmCfg;
}

} // namespace

// ------------------------------------------------------------- device tables

#ifdef EC_YM3526_YMFM
static UINT8 device_start_ym3526_ymfm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf);
extern "C" DEV_DEF devDef_YMFM_3526;

static DEVDEF_RWFUNC devFunc_YMFM_3526[] =
{
	{RWF_REGISTER | RWF_WRITE, DEVRW_A8D8, 0, (void*)opl_write<ymfm::ym3526>},
	{RWF_REGISTER | RWF_READ, DEVRW_A8D8, 0, (void*)opl_read<ymfm::ym3526>},
	{0x00, 0x00, 0, NULL}
};
extern "C" DEV_DEF devDef_YMFM_3526 =
{
	"YM3526", "ymfm", FCC_YMFM,
	device_start_ym3526_ymfm,
	opl_stop<ymfm::ym3526>,
	opl_reset<ymfm::ym3526>,
	opl_update<ymfm::ym3526>,
	NULL,	// SetOptionBits
	NULL,	// SetMuteMask -- ymfm exposes no per-channel mute
	NULL,	// SetPanning
	NULL,	// SetSampleRateChangeCallback
	NULL,	// SetLoggingCallback
	NULL,	// LinkDevice
	devFunc_YMFM_3526,
};
static UINT8 device_start_ym3526_ymfm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf)
{
	return (opl_create<ymfm::ym3526>(cfg, retDevInf, &devDef_YMFM_3526) == NULL) ? 0xFF : 0x00;
}
#endif	// EC_YM3526_YMFM

#ifdef EC_Y8950_YMFM
static UINT8 device_start_y8950_ymfm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf);
extern "C" DEV_DEF devDef_YMFM_8950;

static DEVDEF_RWFUNC devFunc_YMFM_8950[] =
{
	{RWF_REGISTER | RWF_WRITE, DEVRW_A8D8, 0, (void*)opl_write<ymfm::y8950>},
	{RWF_REGISTER | RWF_READ, DEVRW_A8D8, 0, (void*)opl_read<ymfm::y8950>},
	{RWF_MEMORY | RWF_WRITE, DEVRW_BLOCK, 0, (void*)y8950_write_rom},
	{RWF_MEMORY | RWF_WRITE, DEVRW_MEMSIZE, 0, (void*)y8950_alloc_rom},
	{0x00, 0x00, 0, NULL}
};
extern "C" DEV_DEF devDef_YMFM_8950 =
{
	"Y8950", "ymfm", FCC_YMFM,
	device_start_y8950_ymfm,
	opl_stop<ymfm::y8950>,
	opl_reset<ymfm::y8950>,
	opl_update<ymfm::y8950>,
	NULL, NULL, NULL, NULL, NULL, NULL,
	devFunc_YMFM_8950,
};
static UINT8 device_start_y8950_ymfm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf)
{
	return (opl_create<ymfm::y8950>(cfg, retDevInf, &devDef_YMFM_8950) == NULL) ? 0xFF : 0x00;
}
#endif	// EC_Y8950_YMFM

#ifdef EC_YMF278B_YMFM
static UINT8 device_start_ymf278b_ymfm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf);
extern "C" DEV_DEF devDef_YMFM_278B;

static DEVDEF_RWFUNC devFunc_YMFM_278B[] =
{
	{RWF_REGISTER | RWF_WRITE, DEVRW_A8D8, 0, (void*)opl4_write},
	{RWF_REGISTER | RWF_READ, DEVRW_A8D8, 0, (void*)opl4_read},
	{RWF_MEMORY | RWF_WRITE, DEVRW_BLOCK, 0x524F, (void*)opl4_write_rom},	// 'RO'
	{RWF_MEMORY | RWF_WRITE, DEVRW_MEMSIZE, 0x524F, (void*)opl4_alloc_rom},
	{RWF_MEMORY | RWF_WRITE, DEVRW_BLOCK, 0x5241, (void*)opl4_write_ram},	// 'RA'
	{RWF_MEMORY | RWF_WRITE, DEVRW_MEMSIZE, 0x5241, (void*)opl4_alloc_ram},
	{0x00, 0x00, 0, NULL}
};
extern "C" DEV_DEF devDef_YMFM_278B =
{
	"YMF278B", "ymfm", FCC_YMFM,
	device_start_ymf278b_ymfm,
	opl4_stop,
	opl4_reset,
	opl4_update,
	NULL, NULL, NULL, NULL, NULL,
	opl4_link_opl3,
	devFunc_YMFM_278B,
};
static UINT8 device_start_ymf278b_ymfm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf)
{
	opl4_chip* info = (opl4_chip*)calloc(1, sizeof(opl4_chip));
	if (info == NULL) return 0xFF;

	info->intf = new rom_intf();
	// Silent 2MB stub for the YRW801 wavetable ROM, so that sample RAM maps at
	// 0x200000 the way MoonSound hardware puts it. MSX MoonSound rips upload
	// their samples (VGM data block 0x87) but embed no ROM (block 0x84); with no
	// ROM the RAM would sit at address 0, every read from the driver's 0x200000+
	// addresses would miss and pcm_read would return the unmapped 0xFF, which is
	// DC through the envelope -- the buzz those files used to play as. Same fix
	// as the one in emu/cores/ymf278b.c for the plus build; the full reasoning
	// lives there.
	info->intf->alloc(cm_ymfm::REGION_PCM_ROM, CM_YRW801_SIZE);
	// ...and then fill it with the synthetic wavetable, so those rips play their
	// instruments instead of nothing. Built once per process; cm_yrw801.c has the
	// full story, including what it is not. An arcade OPL4 log carries its own
	// ROM block and replaces all of this before a sample is rendered.
	{
		const unsigned char* bank = cm_yrw801_bank();
		if (bank != NULL)
			info->intf->write(cm_ymfm::REGION_PCM_ROM, 0, CM_YRW801_SIZE, bank);
	}
	info->chip = new opl4_pcm(*info->intf);
	info->rate = info->chip->sample_rate(cfg->clock);
	// ymf278b.c leaves SRATE_CUSTOM_HIGHEST commented out for this chip; match it.

	info->fm_l = info->fm_r = 3;	// power-on FM mix level, as in ymf278b.c
	opl4_link_opl3(info, LINKDEV_OPL3, NULL);	// dummy handlers until linked

	INIT_DEVINF(retDevInf, &info->_devData, info->rate, &devDef_YMFM_278B);
	declare_opl3_link(retDevInf, cfg);
	return 0x00;
}
#endif	// EC_YMF278B_YMFM

// libvgm's DEV_DECL for the YMF278B lives in emu/cores/ymf278b.c, which the mas
// build does not compile. Supply it here instead. The channel/link metadata is
// the chip's, not any core's: 24 wavetable channels and one linked YMF262.
#ifdef EC_YMF278B_YMFM
extern "C" {

static const char* opl4_DeviceName(const DEV_GEN_CFG* devCfg) { return "YMF278B"; }
static UINT16 opl4_DeviceChannels(const DEV_GEN_CFG* devCfg) { return 24; }
static const char** opl4_DeviceChannelNames(const DEV_GEN_CFG* devCfg) { return NULL; }
static const DEVLINK_IDS* opl4_DeviceLinkIDs(const DEV_GEN_CFG* devCfg)
{
	static const DEVLINK_IDS dlIDs = { 1, {DEVID_YMF262} };
	return &dlIDs;
}

extern const DEV_DECL sndDev_YMF278B;	// keep external linkage (a const at
extern const DEV_DECL sndDev_YMF278B =	// namespace scope is internal in C++)
{
	DEVID_YMF278B,
	opl4_DeviceName,
	opl4_DeviceChannels,
	opl4_DeviceChannelNames,
	opl4_DeviceLinkIDs,
	{	// cores
		&devDef_YMFM_278B,
		NULL
	}
};

}	// extern "C"
#endif	// EC_YMF278B_YMFM
