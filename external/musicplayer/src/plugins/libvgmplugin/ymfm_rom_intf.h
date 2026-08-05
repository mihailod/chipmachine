// Shared ymfm_interface for the libvgm ymfm adapters (opn_ymfm.cpp, opl_ymfm.cpp).
//
// libvgm hands a chip its sample memory through two DEVDEF_RWFUNC entries --
// DEVRW_MEMSIZE (allocate N bytes) and DEVRW_BLOCK (fill a range from a VGM data
// block). ymfm instead PULLS sample bytes one at a time through
// ymfm_interface::ymfm_external_read(), so the wrapper has to own the buffers
// and serve the reads. That is all this class does.
//
// Every ymfm chip in this plugin needs it, and they need different regions:
//   YM2608          ADPCM-A (internal rhythm ROM, static) + ADPCM-B
//   YM2610/YM2610B  ADPCM-A + ADPCM-B sample ROMs
//   Y8950           ADPCM-B only
//   YMF278B         one PCM address space made of ROM followed by SRAM
//
// so the regions are indexed and each chip wires up only the ones it has.

#ifndef CM_YMFM_ROM_INTF_H
#define CM_YMFM_ROM_INTF_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ymfm.h"

namespace cm_ymfm
{

enum rom_region
{
	REGION_ADPCM_A = 0,	// YM2608 rhythm ROM, YM2610 ADPCM-A
	REGION_ADPCM_B = 1,	// YM2608/YM2610 DELTA-T, Y8950 ADPCM
	REGION_PCM_ROM = 2,	// YMF278B wavetable ROM  (/MCS0, address 0 upwards)
	REGION_PCM_RAM = 3,	// YMF278B wavetable SRAM (directly above the ROM)
	REGION_COUNT   = 4
};

class rom_intf : public ymfm::ymfm_interface
{
public:
	rom_intf()
	{
		memset(m_rom, 0, sizeof(m_rom));
		memset(m_size, 0, sizeof(m_size));
		memset(m_static, 0, sizeof(m_static));
		memset(m_staticSize, 0, sizeof(m_staticSize));
	}
	~rom_intf() override
	{
		for (int i = 0; i < REGION_COUNT; i++) free(m_rom[i]);
	}

	uint8_t* alloc(int idx, uint32_t size)
	{
		if (m_size[idx] == size) return m_rom[idx];
		free(m_rom[idx]);
		m_rom[idx] = (uint8_t*)calloc(1, size ? size : 1);
		m_size[idx] = m_rom[idx] ? size : 0;
		return m_rom[idx];
	}
	void write(int idx, uint32_t offset, uint32_t length, const uint8_t* data)
	{
		if (m_rom[idx] == NULL || offset >= m_size[idx]) return;
		if (offset + length > m_size[idx]) length = m_size[idx] - offset;
		memcpy(m_rom[idx] + offset, data, length);
	}
	// The YM2608's rhythm ROM is fixed data we point at rather than own.
	void set_static(int idx, const uint8_t* data, uint32_t size)
	{
		m_static[idx] = data;
		m_staticSize[idx] = size;
	}

	uint8_t ymfm_external_read(ymfm::access_class type, uint32_t address) override
	{
		if (type == ymfm::ACCESS_PCM) return pcm_read(address);
		int idx = adpcm_region(type);
		if (idx < 0) return 0;
		if (m_rom[idx] != NULL && address < m_size[idx]) return m_rom[idx][address];
		if (m_static[idx] != NULL && address < m_staticSize[idx]) return m_static[idx][address];
		return 0;
	}
	void ymfm_external_write(ymfm::access_class type, uint32_t address, uint8_t data) override
	{
		if (type == ymfm::ACCESS_PCM) { pcm_write(address, data); return; }
		// ADPCM-B is RAM on the YM2608/Y8950; a VGM normally preloads it as a
		// data block, but a log may also write it a byte at a time.
		int idx = adpcm_region(type);
		if (idx != REGION_ADPCM_B || m_rom[idx] == NULL || address >= m_size[idx]) return;
		m_rom[idx][address] = data;
	}

private:
	// The OPL4 sees ONE address space: wavetable ROM from 0, SRAM directly above
	// it, wrapping at 4 MB, unmapped reads returning 0xFF. Mirrors libvgm's
	// ymf278b_readMem/writeMem. (Its "memory access mode = 1" SRAM remapping,
	// which no VGM in the corpus uses, is not reproduced.)
	static const uint32_t PCM_ADDR_MASK = 0x3FFFFF;

	uint8_t pcm_read(uint32_t address) const
	{
		address &= PCM_ADDR_MASK;
		if (address < m_size[REGION_PCM_ROM]) return m_rom[REGION_PCM_ROM][address];
		address -= m_size[REGION_PCM_ROM];
		if (address < m_size[REGION_PCM_RAM]) return m_rom[REGION_PCM_RAM][address];
		return 0xFF;
	}
	void pcm_write(uint32_t address, uint8_t data)
	{
		address &= PCM_ADDR_MASK;
		if (address < m_size[REGION_PCM_ROM]) return;	// can't write to ROM
		address -= m_size[REGION_PCM_ROM];
		if (address < m_size[REGION_PCM_RAM]) m_rom[REGION_PCM_RAM][address] = data;
	}

	static int adpcm_region(ymfm::access_class type)
	{
		if (type == ymfm::ACCESS_ADPCM_A) return REGION_ADPCM_A;
		if (type == ymfm::ACCESS_ADPCM_B) return REGION_ADPCM_B;
		return -1;
	}

	uint8_t* m_rom[REGION_COUNT];
	uint32_t m_size[REGION_COUNT];
	const uint8_t* m_static[REGION_COUNT];
	uint32_t m_staticSize[REGION_COUNT];
};

} // namespace cm_ymfm

#endif	// CM_YMFM_ROM_INTF_H
