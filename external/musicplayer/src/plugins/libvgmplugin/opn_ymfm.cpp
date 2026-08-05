// ymfm OPN cores for libvgm -- YM2203 / YM2608 / YM2610 / YM2610B.
//
// libvgm's own OPN emulation (emu/cores/fmopn.c) is MAME-derived and GPL-2.0+,
// and it is the ONLY core libvgm ships for these chips -- so the Mac App Store
// variant could not drop it without losing every YM2203/YM2608/YM2610 song
// (~600-700 catalog rows: all of NEC PC-98, PC-88 and Neo Geo, plus much of the
// arcade corpus). This file wraps Aaron Giles' ymfm (BSD-3, vendored at
// external/ymfm) in libvgm's C DEV_DEF interface so the mas build has a
// permissive OPN core. See external/ymfm/PROVENANCE.md.
//
// Modelled on upstream libvgm's own ymfmintf.cpp (which wraps ymfm's YM2414),
// extended with the two things the OPN family needs and the OPZ does not:
//
//   * SSG. YM2203/YM2608 contain an AY-3-8910-compatible SSG. libvgm models it
//     as a SEPARATE linked device (DEVID_AY8910, LINKDEV_SSG) that it mixes and
//     volumes independently, and fmopn.c forwards register writes 0x00-0x0F to
//     it. ymfm has its own internal SSG, which would put the SSG inside the FM
//     output at ymfm's balance instead -- audibly different mix. So we install
//     an ymfm::ssg_override that forwards to libvgm's linked device exactly as
//     fmopn does; ymfm's internal SSG then stays silent (it never sees a write).
//
//   * ADPCM. YM2610 has ADPCM-A + ADPCM-B sample ROMs and YM2608 has an ADPCM-B
//     (DELTA-T) region, all fed by VGM data blocks through libvgm's
//     DEVRW_MEMSIZE / DEVRW_BLOCK entry points. ymfm pulls sample bytes through
//     ymfm_interface::ymfm_external_read() instead, so the wrapper owns the
//     buffers and serves the reads.
//
// Sample rate: ymfm reports a rate that already accounts for every prescaler
// setting (OPN_FIDELITY_MIN = the fastest FM rate), so unlike fmopn this needs
// no SetSampleRateChangeCallback -- a 0x2D/0x2E/0x2F prescaler write changes the
// chip's internal division, not our output rate.

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
#include "../../../../zxtune/3rdparty/vgm/emu/SoundEmu.h"      // SndEmu_GetDeviceFunc
#include "../../../../zxtune/3rdparty/vgm/emu/cores/fmopn.h"   // ssg_callbacks only
#include "../../../../zxtune/3rdparty/vgm/emu/cores/ayintf.h"  // AY8910_CFG, AYTYPE_*
}

#define LINKDEV_SSG	0x00

#include "ymfm.h"
#include "ymfm_opn.h"
// NOTE: deliberately NOT including ymfm_fm.ipp. ymfm_opn.cpp already includes it
// and instantiates the fm_engine templates there; pulling it in here too would
// instantiate a second copy in this TU, and because the plugin compiles at
// hidden visibility and partial-links with `ld -r`, those weak template symbols
// become private externs that the linker cannot merge -- it fails on
// opn_registers_base<>::write. The public chip classes in ymfm_opn.h are all
// this wrapper needs.

#ifdef OPN_YMFM_RHYTHM_ROM
// The YM2608's rhythm section (bass/snare/cymbal/hihat/tom/rim) plays from an
// ADPCM-A ROM *inside* the chip. It cannot be read out, so every emulator ships
// the same dump; libvgm's copy carries the note "This data is derived from the
// chip's output - internal ROM can't be read. It was verified, using real
// YM2608, that this ADPCM stream produces 100% correct output signal." It is
// Yamaha ROM data rather than anybody's source code, which is why it is used
// here even though the file sits in the GPL fmopn file set -- without it every
// PC-98 track loses its drums. Build with -UOPN_YMFM_RHYTHM_ROM to omit it.
#include "../../../../zxtune/3rdparty/vgm/emu/cores/fmopn_2608rom.h"
#endif

#define FCC_YMFM 0x594D464D	// "YMFM"

namespace
{

// ---------------------------------------------------------------- interface

// Serves ymfm's sample fetches out of the ROM regions libvgm hands us.
class opn_intf : public ymfm::ymfm_interface
{
public:
	opn_intf() { memset(m_rom, 0, sizeof(m_rom)); memset(m_size, 0, sizeof(m_size)); }
	~opn_intf() override
	{
		for (int i = 0; i < 2; i++) free(m_rom[i]);
	}

	// idx 0 = ADPCM-A, idx 1 = ADPCM-B
	UINT8* alloc(int idx, UINT32 size)
	{
		if (m_size[idx] == size) return m_rom[idx];
		free(m_rom[idx]);
		m_rom[idx] = (UINT8*)calloc(1, size ? size : 1);
		m_size[idx] = m_rom[idx] ? size : 0;
		return m_rom[idx];
	}
	void write(int idx, UINT32 offset, UINT32 length, const UINT8* data)
	{
		if (m_rom[idx] == NULL || offset >= m_size[idx]) return;
		if (offset + length > m_size[idx]) length = m_size[idx] - offset;
		memcpy(m_rom[idx] + offset, data, length);
	}
	// YM2608's rhythm ROM is fixed data we point at rather than own.
	void set_static(int idx, const UINT8* data, UINT32 size)
	{
		m_static[idx] = data;
		m_staticSize[idx] = size;
	}

	uint8_t ymfm_external_read(ymfm::access_class type, uint32_t address) override
	{
		int idx = region(type);
		if (idx < 0) return 0;
		if (m_rom[idx] != NULL && address < m_size[idx]) return m_rom[idx][address];
		if (m_static[idx] != NULL && address < m_staticSize[idx]) return m_static[idx][address];
		return 0;
	}
	void ymfm_external_write(ymfm::access_class type, uint32_t address, uint8_t data) override
	{
		// ADPCM-B is RAM on the YM2608; a VGM normally preloads it as a data
		// block, but a log may also write it a byte at a time.
		int idx = region(type);
		if (idx != 1 || m_rom[1] == NULL || address >= m_size[1]) return;
		m_rom[1][address] = data;
	}

private:
	static int region(ymfm::access_class type)
	{
		if (type == ymfm::ACCESS_ADPCM_A) return 0;
		if (type == ymfm::ACCESS_ADPCM_B) return 1;
		return -1;
	}
	UINT8* m_rom[2];
	UINT32 m_size[2];
	const UINT8* m_static[2] = { NULL, NULL };
	UINT32 m_staticSize[2] = { 0, 0 };
};

// Routes ymfm's SSG accesses to the AY8910 device libvgm linked to us, so the
// SSG keeps its own mixer channel and volume exactly as with fmopn.
class ssg_bridge : public ymfm::ssg_override
{
public:
	void link(const ssg_callbacks* funcs, void* param)
	{
		if (funcs == NULL) { m_param = NULL; memset(&m_func, 0, sizeof(m_func)); return; }
		m_func = *funcs;
		m_param = param;
	}
	bool linked() const { return m_param != NULL; }
	void set_clock(UINT32 clock)
	{
		if (m_param != NULL && m_func.set_clock != NULL) m_func.set_clock(m_param, clock);
	}

	void ssg_reset() override
	{
		if (m_param != NULL && m_func.reset != NULL) m_func.reset(m_param);
	}
	uint8_t ssg_read(uint32_t regnum) override
	{
		if (m_param == NULL || m_func.read == NULL) return 0;
		m_func.write(m_param, 0, (UINT8)regnum);	// latch address
		return m_func.read(m_param, 1);
	}
	void ssg_write(uint32_t regnum, uint8_t data) override
	{
		if (m_param == NULL || m_func.write == NULL) return;
		m_func.write(m_param, 0, (UINT8)regnum);	// address
		m_func.write(m_param, 1, data);			// data
	}
	// The prescaler also divides the SSG clock; tell the linked device.
	void ssg_prescale_changed() override { if (m_pending) set_clock(m_pending); }
	void set_pending_clock(UINT32 clock) { m_pending = clock; }

private:
	ssg_callbacks m_func = {};
	void* m_param = NULL;
	UINT32 m_pending = 0;
};

// ------------------------------------------------------------- chip wrapper

template <typename ChipT>
struct opn_chip
{
	DEV_DATA _devData;		// must stay first -- libvgm casts to it
	opn_intf* intf;
	ssg_bridge* ssg;
	ChipT* chip;
	UINT32 clock;
	UINT32 rate;
	UINT32 div;				// ymfm samples generated per reported output sample
};

// How many ymfm samples to fold into one output sample.
//
// ymfm reports a rate that is prescaler-independent, so at OPN_FIDELITY_MIN the
// YM2203 runs at clock/24 and the YM2608 at clock/48 -- three times the rate
// libvgm's fmopn reports for the same chips (clock/72 and clock/144). Handing
// libvgm a 166 kHz device rate is outside anything its own cores produce (they
// all sit at or below ~60 kHz) and it destabilises the resampler: with the
// prescaler at its default 6 the run is fine, but the fixed-point step maths in
// Resmpl_Exec_LinearDown -- which carries its own comment about overflow "with
// extremely high chip sample rates" -- intermittently produced an absurd sample
// count, and Resmpl_EnsureBuffers() calls abort() outright when the resulting
// malloc fails. That is the heisen-abort this wrapper first shipped with: it
// depended on heap layout, vanished under lldb/Guard Malloc, and turned into an
// apparent hang whenever the huge allocation happened to succeed.
//
// So report fmopn's rate and fold ymfm's extra samples down ourselves. With the
// default prescaler ymfm already repeats each FM sample 3 times, so averaging
// three is exact there and a reasonable box filter for prescale 2/3.
// The YM2610 needs no folding: ymfm's clock/144 already equals fmopn's rate,
// which is exactly why that chip was stable from the start.
template <typename ChipT> struct opn_rate_div { static const UINT32 value = 1; };
template <> struct opn_rate_div<ymfm::ym2203> { static const UINT32 value = 3; };
template <> struct opn_rate_div<ymfm::ym2608> { static const UINT32 value = 3; };

// libvgm models the SSG as a separate device that the PLAYER creates and links,
// so the chip has to advertise it in its DEV_INFO. This mirrors opnintf.c's
// init_ssg_devinfo() -- clockDiv and the AY type per chip are taken from there
// (YM2203: /1, YM2608 and YM2610: /2), and the AY8910_CFG is freed by libvgm.
static void declare_ssg_link(DEV_INFO* devInf, const DEV_GEN_CFG* cfg,
                             UINT32 clockDiv, UINT8 ssgType)
{
	AY8910_CFG* ssgCfg = (AY8910_CFG*)calloc(1, sizeof(AY8910_CFG));
	if (ssgCfg == NULL) return;
	ssgCfg->_genCfg = *cfg;
	ssgCfg->_genCfg.clock = cfg->clock / clockDiv / 2;
	ssgCfg->_genCfg.flags = 0x00;
	ssgCfg->_genCfg.emuCore = 0;
	ssgCfg->chipType = ssgType;
	ssgCfg->chipFlags = YM2149_PIN26_HIGH;

	devInf->linkDevCount = 1;
	devInf->linkDevs = (DEVLINK_INFO*)calloc(1, sizeof(DEVLINK_INFO));
	if (devInf->linkDevs == NULL) { free(ssgCfg); devInf->linkDevCount = 0; return; }
	devInf->linkDevs[0].devID = DEVID_AY8910;
	devInf->linkDevs[0].linkID = LINKDEV_SSG;
	devInf->linkDevs[0].cfg = (DEV_GEN_CFG*)ssgCfg;
}

template <typename ChipT>
static opn_chip<ChipT>* create(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf, DEV_DEF* devDef)
{
	opn_chip<ChipT>* info = (opn_chip<ChipT>*)calloc(1, sizeof(opn_chip<ChipT>));
	if (info == NULL) return NULL;

	info->intf = new opn_intf();
	info->ssg = new ssg_bridge();
	info->chip = new ChipT(*info->intf);
	// MIN fidelity = ymfm's fastest-FM-rate mode. MED/MAX oversample 2x/24x for
	// a cleaner spectrum at a cost libvgm's own resampler mostly recovers, and
	// MAX would run the YM2203 at ~1 MHz per chip.
	info->chip->set_fidelity(ymfm::OPN_FIDELITY_MIN);
	info->chip->ssg_override(*info->ssg);
	info->clock = cfg->clock;
	info->div = opn_rate_div<ChipT>::value;
	info->rate = info->chip->sample_rate(cfg->clock) / info->div;
	SRATE_CUSTOM_HIGHEST(cfg->srMode, info->rate, cfg->smplRate);

	INIT_DEVINF(retDevInf, &info->_devData, info->rate, devDef);
	return info;
}

template <typename ChipT>
static void opn_stop(void* chipPtr)
{
	opn_chip<ChipT>* info = (opn_chip<ChipT>*)chipPtr;
	delete info->chip;
	delete info->ssg;
	delete info->intf;
	free(info);
}

template <typename ChipT>
static void opn_reset(void* chipPtr)
{
	opn_chip<ChipT>* info = (opn_chip<ChipT>*)chipPtr;
	info->chip->reset();
}

template <typename ChipT>
static UINT8 opn_write(void* chipPtr, UINT8 offset, UINT8 data)
{
	opn_chip<ChipT>* info = (opn_chip<ChipT>*)chipPtr;
	info->chip->write(offset, data);
	return 0x00;
}

template <typename ChipT>
static UINT8 opn_read(void* chipPtr, UINT8 offset)
{
	opn_chip<ChipT>* info = (opn_chip<ChipT>*)chipPtr;
	return info->chip->read(offset);
}

// YM2203: output_data is [FM, SSG-A, SSG-B, SSG-C] and the FM half is mono.
// The SSG lanes stay at zero because every SSG write went to the linked device.
static void ym2203_update(void* chipPtr, UINT32 samples, DEV_SMPL** outputs)
{
	opn_chip<ymfm::ym2203>* info = (opn_chip<ymfm::ym2203>*)chipPtr;
	ymfm::ym2203::output_data out;
	for (UINT32 i = 0; i < samples; i++)
	{
		INT32 sum = 0;
		for (UINT32 s = 0; s < info->div; s++)
		{
			info->chip->generate(&out, 1);
			sum += out.data[0];
		}
		DEV_SMPL smpl = (DEV_SMPL)(sum / (INT32)info->div);
		outputs[0][i] = smpl;
		outputs[1][i] = smpl;
	}
}

// YM2608 / YM2610: output_data is [FM-L, FM-R, SSG].
//
// The x2 is a LEVEL MATCH, not a fudge. Both chips halve their output sum on
// real hardware, and ymfm does it; libvgm's fmopn.c has that step commented out
// in both ym2608_update_one and ym2610_update_one --
//     //lt >>= 1; // shift right verified on real YM2608
// -- so libvgm's OPNA/OPNB run 2x hot, and the per-chip entry in its
// _CHIP_VOLUME table (player/vgmplayer.cpp) is calibrated to that. Matching it
// keeps the mas mix identical to plus and keeps these chips balanced against
// every other chip in a multi-chip VGM. Measured: without this, YM2610 renders
// at exactly 0.50 RMS of fmopn; with it, 1.00. The YM2203 needs no correction
// (fmopn does not halve it either) and is left alone.
template <typename ChipT>
static void opna_update(void* chipPtr, UINT32 samples, DEV_SMPL** outputs)
{
	opn_chip<ChipT>* info = (opn_chip<ChipT>*)chipPtr;
	typename ChipT::output_data out;
	for (UINT32 i = 0; i < samples; i++)
	{
		INT32 sumL = 0, sumR = 0;
		for (UINT32 s = 0; s < info->div; s++)
		{
			info->chip->generate(&out, 1);
			sumL += out.data[0];
			sumR += out.data[1];
		}
		outputs[0][i] = (DEV_SMPL)(sumL / (INT32)info->div) * 2;
		outputs[1][i] = (DEV_SMPL)(sumR / (INT32)info->div) * 2;
	}
}

template <typename ChipT>
static UINT8 opn_link_ssg(void* chipPtr, UINT8 linkID, const DEV_INFO* defInfSSG)
{
	opn_chip<ChipT>* info = (opn_chip<ChipT>*)chipPtr;
	if (linkID != LINKDEV_SSG) return EERR_UNK_DEVICE;

	if (defInfSSG == NULL) { info->ssg->link(NULL, NULL); return 0x00; }

	ssg_callbacks funcs;
	memset(&funcs, 0, sizeof(funcs));
	if (SndEmu_GetDeviceFunc(defInfSSG->devDef, RWF_REGISTER | RWF_WRITE, DEVRW_A8D8, 0, (void**)&funcs.write))
		return 0xFF;
	if (SndEmu_GetDeviceFunc(defInfSSG->devDef, RWF_REGISTER | RWF_READ, DEVRW_A8D8, 0, (void**)&funcs.read))
		return 0xFF;
	if (SndEmu_GetDeviceFunc(defInfSSG->devDef, RWF_CLOCK | RWF_WRITE, DEVRW_VALUE, 0, (void**)&funcs.set_clock))
		return 0xFF;
	if (defInfSSG->devDef->Reset == NULL) return 0xFF;
	funcs.reset = defInfSSG->devDef->Reset;

	info->ssg->link(&funcs, defInfSSG->dataPtr);
	info->ssg->set_pending_clock(info->chip->ssg_effective_clock(info->clock));
	return 0x00;
}

// ROM region plumbing. 'A' = ADPCM-A, 'B' = ADPCM-B / DELTA-T.
template <typename ChipT, int IDX>
static void opn_alloc_rom(void* chipPtr, UINT32 memsize)
{
	opn_chip<ChipT>* info = (opn_chip<ChipT>*)chipPtr;
	info->intf->alloc(IDX, memsize);
}

template <typename ChipT, int IDX>
static void opn_write_rom(void* chipPtr, UINT32 offset, UINT32 length, const UINT8* data)
{
	opn_chip<ChipT>* info = (opn_chip<ChipT>*)chipPtr;
	info->intf->write(IDX, offset, length, data);
}

} // namespace

// ------------------------------------------------------------- device tables

#ifdef EC_YM2203_YMFM
static UINT8 device_start_ym2203_ymfm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf);
extern "C" DEV_DEF devDef_YMFM_2203;

static DEVDEF_RWFUNC devFunc_YMFM_2203[] =
{
	{RWF_REGISTER | RWF_WRITE, DEVRW_A8D8, 0, (void*)opn_write<ymfm::ym2203>},
	{RWF_REGISTER | RWF_READ, DEVRW_A8D8, 0, (void*)opn_read<ymfm::ym2203>},
	{0x00, 0x00, 0, NULL}
};
extern "C" DEV_DEF devDef_YMFM_2203 =
{
	"YM2203", "ymfm", FCC_YMFM,
	device_start_ym2203_ymfm,
	opn_stop<ymfm::ym2203>,
	opn_reset<ymfm::ym2203>,
	ym2203_update,
	NULL,	// SetOptionBits
	NULL,	// SetMuteMask -- ymfm exposes no per-channel mute
	NULL,	// SetPanning
	NULL,	// SetSampleRateChangeCallback -- rate covers every prescaler
	NULL,	// SetLoggingCallback
	opn_link_ssg<ymfm::ym2203>,
	devFunc_YMFM_2203,
};
static UINT8 device_start_ym2203_ymfm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf)
{
	if (create<ymfm::ym2203>(cfg, retDevInf, &devDef_YMFM_2203) == NULL) return 0xFF;
	declare_ssg_link(retDevInf, cfg, 1, AYTYPE_YM2203);
	return 0x00;
}
#endif	// EC_YM2203_YMFM

#ifdef EC_YM2608_YMFM
static UINT8 device_start_ym2608_ymfm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf);
extern "C" DEV_DEF devDef_YMFM_2608;

static DEVDEF_RWFUNC devFunc_YMFM_2608[] =
{
	{RWF_REGISTER | RWF_WRITE, DEVRW_A8D8, 0, (void*)opn_write<ymfm::ym2608>},
	{RWF_REGISTER | RWF_READ, DEVRW_A8D8, 0, (void*)opn_read<ymfm::ym2608>},
	{RWF_MEMORY | RWF_WRITE, DEVRW_BLOCK, 'B', (void*)opn_write_rom<ymfm::ym2608, 1>},
	{RWF_MEMORY | RWF_WRITE, DEVRW_MEMSIZE, 'B', (void*)opn_alloc_rom<ymfm::ym2608, 1>},
	{0x00, 0x00, 0, NULL}
};
extern "C" DEV_DEF devDef_YMFM_2608 =
{
	"YM2608", "ymfm", FCC_YMFM,
	device_start_ym2608_ymfm,
	opn_stop<ymfm::ym2608>,
	opn_reset<ymfm::ym2608>,
	opna_update<ymfm::ym2608>,
	NULL, NULL, NULL, NULL, NULL,
	opn_link_ssg<ymfm::ym2608>,
	devFunc_YMFM_2608,
};
static UINT8 device_start_ym2608_ymfm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf)
{
	opn_chip<ymfm::ym2608>* info = create<ymfm::ym2608>(cfg, retDevInf, &devDef_YMFM_2608);
	if (info == NULL) return 0xFF;
#ifdef OPN_YMFM_RHYTHM_ROM
	info->intf->set_static(0, (const UINT8*)YM2608_ADPCM_ROM, sizeof(YM2608_ADPCM_ROM));
#endif
	declare_ssg_link(retDevInf, cfg, 2, AYTYPE_YM2608);
	return 0x00;
}
#endif	// EC_YM2608_YMFM

#ifdef EC_YM2610_YMFM
static UINT8 device_start_ym2610_ymfm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf);
static UINT8 device_start_ym2610b_ymfm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf);
extern "C" DEV_DEF devDef_YMFM_2610;
extern "C" DEV_DEF devDef_YMFM_2610B;

static DEVDEF_RWFUNC devFunc_YMFM_2610[] =
{
	{RWF_REGISTER | RWF_WRITE, DEVRW_A8D8, 0, (void*)opn_write<ymfm::ym2610>},
	{RWF_REGISTER | RWF_READ, DEVRW_A8D8, 0, (void*)opn_read<ymfm::ym2610>},
	{RWF_MEMORY | RWF_WRITE, DEVRW_BLOCK, 'A', (void*)opn_write_rom<ymfm::ym2610, 0>},
	{RWF_MEMORY | RWF_WRITE, DEVRW_MEMSIZE, 'A', (void*)opn_alloc_rom<ymfm::ym2610, 0>},
	{RWF_MEMORY | RWF_WRITE, DEVRW_BLOCK, 'B', (void*)opn_write_rom<ymfm::ym2610, 1>},
	{RWF_MEMORY | RWF_WRITE, DEVRW_MEMSIZE, 'B', (void*)opn_alloc_rom<ymfm::ym2610, 1>},
	{0x00, 0x00, 0, NULL}
};
extern "C" DEV_DEF devDef_YMFM_2610 =
{
	"YM2610", "ymfm", FCC_YMFM,
	device_start_ym2610_ymfm,
	opn_stop<ymfm::ym2610>,
	opn_reset<ymfm::ym2610>,
	opna_update<ymfm::ym2610>,
	NULL, NULL, NULL, NULL, NULL,
	opn_link_ssg<ymfm::ym2610>,
	devFunc_YMFM_2610,
};
// YM2610B is the same die with 6 FM channels instead of 4; ymfm subclasses it,
// and libvgm selects it through cfg->flags (see opnintf.c).
static DEVDEF_RWFUNC devFunc_YMFM_2610B[] =
{
	{RWF_REGISTER | RWF_WRITE, DEVRW_A8D8, 0, (void*)opn_write<ymfm::ym2610b>},
	{RWF_REGISTER | RWF_READ, DEVRW_A8D8, 0, (void*)opn_read<ymfm::ym2610b>},
	{RWF_MEMORY | RWF_WRITE, DEVRW_BLOCK, 'A', (void*)opn_write_rom<ymfm::ym2610b, 0>},
	{RWF_MEMORY | RWF_WRITE, DEVRW_MEMSIZE, 'A', (void*)opn_alloc_rom<ymfm::ym2610b, 0>},
	{RWF_MEMORY | RWF_WRITE, DEVRW_BLOCK, 'B', (void*)opn_write_rom<ymfm::ym2610b, 1>},
	{RWF_MEMORY | RWF_WRITE, DEVRW_MEMSIZE, 'B', (void*)opn_alloc_rom<ymfm::ym2610b, 1>},
	{0x00, 0x00, 0, NULL}
};
extern "C" DEV_DEF devDef_YMFM_2610B =
{
	"YM2610B", "ymfm", FCC_YMFM,
	device_start_ym2610b_ymfm,
	opn_stop<ymfm::ym2610b>,
	opn_reset<ymfm::ym2610b>,
	opna_update<ymfm::ym2610b>,
	NULL, NULL, NULL, NULL, NULL,
	opn_link_ssg<ymfm::ym2610b>,
	devFunc_YMFM_2610B,
};
// libvgm exposes ONE YM2610 entry in the device list and picks the 6-FM-channel
// "B" die from cfg->flags inside the start function -- see device_start_ym2610()
// in opnintf.c. Mirror that, or a YM2610B log silently loses two FM channels.
static UINT8 device_start_ym2610_ymfm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf)
{
	if (cfg->flags) return device_start_ym2610b_ymfm(cfg, retDevInf);
	if (create<ymfm::ym2610>(cfg, retDevInf, &devDef_YMFM_2610) == NULL) return 0xFF;
	declare_ssg_link(retDevInf, cfg, 2, AYTYPE_YM2610);
	return 0x00;
}
static UINT8 device_start_ym2610b_ymfm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf)
{
	if (create<ymfm::ym2610b>(cfg, retDevInf, &devDef_YMFM_2610B) == NULL) return 0xFF;
	declare_ssg_link(retDevInf, cfg, 2, AYTYPE_YM2610);
	return 0x00;
}
#endif	// EC_YM2610_YMFM
