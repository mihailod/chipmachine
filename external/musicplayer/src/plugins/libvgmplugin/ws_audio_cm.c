// Bandai WonderSwan / WonderSwan Color sound -- libvgm DEV_DEF wrapper.
//
// Replaces libvgm's emu/cores/ws_audio.c, which is Mamiya's in_wsr code cut from
// OSWAN 0.70 and therefore GPL-2-or-later -- the one GPL chip core still left in
// the Mac App Store build after the VSU went. The chip itself lives in
// ../wsrplugin/wswan/ws_apu.c, written from the WSdev Wiki's register
// documentation and shared with the .wsr player, which drives the same hardware
// from a whole emulated machine. This file is only the adapter.
//
// What a VGM log can reach: VGMPlayer's WonderSwan register command builds the
// port as 0x80 + (data & 0x7F), so nothing below $80 is ever written -- no sound
// DMA, no Hyper Voice. The APU's DMA reader is left NULL for the same reason:
// there is no cartridge ROM in a VGM log to stream from.
//
// See ../wsrplugin/wswan/README.md and this directory's README.md.

#include <stdlib.h>
#include <string.h>

#include "../../../../zxtune/3rdparty/vgm/stdtype.h"
#include "../../../../zxtune/3rdparty/vgm/emu/EmuStructs.h"
#include "../../../../zxtune/3rdparty/vgm/emu/SoundDevs.h"
#include "../../../../zxtune/3rdparty/vgm/emu/snddef.h"
#include "../../../../zxtune/3rdparty/vgm/emu/EmuHelper.h"
#include "../../../../zxtune/3rdparty/vgm/emu/cores/ws_audio.h"

#include "../wsrplugin/wswan/ws_apu.h"

#define FCC_CHPM	0x4348504D	// "CHPM" -- chipmachine's own core

// The sound hardware can see 16 KB of the WonderSwan's internal RAM; a VGM log
// uploads the wavetable into it through the memory-write function.
#define WS_RAM_SIZE	0x4000

typedef struct
{
	DEV_DATA _devData;
	WSAPU apu;
	UINT8 ram[WS_RAM_SIZE];
} WSA_CHIP;

static UINT8 ws_cm_init(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf);
static void ws_cm_done(void* info);
static void ws_cm_reset(void* info);
static void ws_cm_update(void* info, UINT32 samples, DEV_SMPL** outputs);
static void ws_cm_port_write(void* info, UINT8 port, UINT8 value);
static UINT8 ws_cm_port_read(void* info, UINT8 port);
static void ws_cm_write_ram_byte(void* info, UINT16 offset, UINT8 value);
static UINT8 ws_cm_read_ram_byte(void* info, UINT16 offset);
static void ws_cm_set_mute_mask(void* info, UINT32 muteMask);

// Same set the core this replaces offered, so VGMPlayer's DEVID_WSWAN start
// branch binds the same two functions (RWF_REGISTER A8D8 write for the ports,
// RWF_MEMORY A16D8 write for the wavetable RAM).
static DEVDEF_RWFUNC devFunc_WS_CM[] =
{
	{RWF_REGISTER | RWF_WRITE, DEVRW_A8D8, 0, (void*)ws_cm_port_write},
	{RWF_REGISTER | RWF_READ, DEVRW_A8D8, 0, (void*)ws_cm_port_read},
	{RWF_MEMORY | RWF_WRITE, DEVRW_A16D8, 0, (void*)ws_cm_write_ram_byte},
	{RWF_MEMORY | RWF_READ, DEVRW_A16D8, 0, (void*)ws_cm_read_ram_byte},
	{RWF_CHN_MUTE | RWF_WRITE, DEVRW_ALL, 0, (void*)ws_cm_set_mute_mask},
	{0x00, 0x00, 0, NULL}
};

static DEV_DEF devDef_WS_CM =
{
	"WonderSwan", "chipmachine", FCC_CHPM,

	ws_cm_init,
	ws_cm_done,
	ws_cm_reset,
	ws_cm_update,

	NULL,	// SetOptionBits
	ws_cm_set_mute_mask,
	NULL,	// SetPanning
	NULL,	// SetSampleRateChangeCallback
	NULL,	// SetLoggingCallback
	NULL,	// LinkDevice

	devFunc_WS_CM,
};

static const char* DeviceName(const DEV_GEN_CFG* devCfg)
{
	return "WonderSwan";
}

static UINT16 DeviceChannels(const DEV_GEN_CFG* devCfg)
{
	return WSAPU_CHANNELS;
}

static const char** DeviceChannelNames(const DEV_GEN_CFG* devCfg)
{
	static const char* names[WSAPU_CHANNELS] =
	{
		"1", "2 (Voice)", "3 (Sweep)", "4 (Noise)"
	};
	return names;
}

static const DEVLINK_IDS* DeviceLinkIDs(const DEV_GEN_CFG* devCfg)
{
	return NULL;
}

const DEV_DECL sndDev_WSwan =
{
	DEVID_WSWAN,
	DeviceName,
	DeviceChannels,
	DeviceChannelNames,
	DeviceLinkIDs,
	{	// cores
		&devDef_WS_CM,
		NULL
	}
};

static UINT8 ws_cm_init(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf)
{
	WSA_CHIP* chip = (WSA_CHIP*)calloc(1, sizeof(WSA_CHIP));
	UINT32 rate;
	if (chip == NULL) { return 0xFF; }

	// The DAC emits one sample every 128 master cycles (3.072 MHz -> 24 kHz).
	rate = cfg->clock / 128;
	if (rate == 0) { rate = 1; }
	SRATE_CUSTOM_HIGHEST(cfg->srMode, rate, cfg->smplRate);

	wsapu_init(&chip->apu, cfg->clock, rate);
	wsapu_set_ram(&chip->apu, chip->ram, WS_RAM_SIZE - 1);
	wsapu_set_mute_mask(&chip->apu, 0x00);

	chip->_devData.chipInf = chip;
	INIT_DEVINF(retDevInf, &chip->_devData, rate, &devDef_WS_CM);

	return 0x00;
}

static void ws_cm_done(void* info)
{
	free((WSA_CHIP*)info);
}

static void ws_cm_reset(void* info)
{
	WSA_CHIP* chip = (WSA_CHIP*)info;

	memset(chip->ram, 0x00, sizeof(chip->ram));
	wsapu_reset(&chip->apu);
}

static void ws_cm_port_write(void* info, UINT8 port, UINT8 value)
{
	wsapu_write_port(&((WSA_CHIP*)info)->apu, port, value);
}

static UINT8 ws_cm_port_read(void* info, UINT8 port)
{
	return wsapu_read_port(&((WSA_CHIP*)info)->apu, port);
}

static void ws_cm_write_ram_byte(void* info, UINT16 offset, UINT8 value)
{
	WSA_CHIP* chip = (WSA_CHIP*)info;

	chip->ram[offset & (WS_RAM_SIZE - 1)] = value;
}

static UINT8 ws_cm_read_ram_byte(void* info, UINT16 offset)
{
	WSA_CHIP* chip = (WSA_CHIP*)info;

	return chip->ram[offset & (WS_RAM_SIZE - 1)];
}

static void ws_cm_set_mute_mask(void* info, UINT32 muteMask)
{
	wsapu_set_mute_mask(&((WSA_CHIP*)info)->apu, muteMask);
}

static void ws_cm_update(void* info, UINT32 samples, DEV_SMPL** outputs)
{
	WSA_CHIP* chip = (WSA_CHIP*)info;

	// DEV_SMPL is a 32-bit signed sample, which is what the APU renders.
	wsapu_render(&chip->apu, (INT32*)outputs[0], (INT32*)outputs[1], samples);
}
