// ymfm OPL/OPL2/OPL3 devices for s98plugin -- the non-GPL replacement for
// s98mame.cpp.
//
// s98mame.cpp wraps MAME's fmopl.c (YM3812) and ymf262.c (YMF262), both of
// which are GPL-2.0+: Jarek Burczynski did not agree to MAME's 2016 BSD-3
// relicensing, so those two files kept the copyleft tag while most of MAME
// went permissive. They are the only GPL code in this plugin. This file wraps
// Aaron Giles' ymfm (BSD-3, vendored at external/ymfm) instead, exactly as
// libvgmplugin/opl_ymfm.cpp does for libvgm's copies of the same cores.
//
// Only 11 songs in the catalog use these chips at all -- 9 OPL3 (a Sound
// Blaster rip) and 2 OPL2 (the PC-9801 "Sound Orchestra" board, YM3812 +
// YM2203). Everything else in the S98 corpus is OPN/OPNA/OPM/OPLL/PSG and
// never reaches this file.
//
// Two behavioural notes against the MAME implementation this replaces:
//
//   * RESAMPLING. MAME's cores resample internally -- YM3812Init(clock, rate)
//     takes the output rate and steps a phase accumulator. ymfm does not: it
//     generates at the chip's native rate (clock/72 for OPL2, clock/288 for
//     OPL3) and leaves rate conversion to the caller. So this file box-filters
//     from native down to the output rate. A box filter is used rather than
//     dropping samples because both chips run above the output rate here
//     (55466 Hz and 50000 Hz against 44100 Hz), so plain decimation would
//     alias.
//
//   * THE OPL3 HIGH BANK. m_s98.cpp encodes a port-1 write as 0x100|reg, which
//     for OPL3 is the second register bank. s98mame.cpp passed that straight to
//     YMF262Write() as an address byte, so bank-1 writes landed in bank 0 --
//     4-operator mode and per-channel stereo were silently wrong. ymfm has a
//     distinct write_address_hi() for that bank, and this file uses it.

#include <string.h>
#include <stdint.h>

#include "s98device.h"

#include "ymfm.h"
#include "ymfm_opl.h"

namespace
{

// ymfm requires an interface object for timers, IRQs and external memory.
// None of that applies to an S98 register log: there is no CPU to interrupt
// and no ADPCM memory on these two chips, so every callback keeps its default
// no-op implementation.
struct s98_ymfm_intf : public ymfm::ymfm_interface
{
};

// ---------------------------------------------------------------------------
// Shared resampler.
//
// Holds one native-rate sample and integrates over however much of it each
// output sample covers, fetching the next native sample when the current one
// is used up. That is a box filter of width exactly one output sample, which
// is cheap, has no ringing, and is enough anti-aliasing for a 1.26:1 or
// 1.13:1 downconversion.
// ---------------------------------------------------------------------------
template <typename ChipT>
class S98DeviceOplBase : public S98DEVICEIF
{
public:
	S98DeviceOplBase() : chip(intf), bEnable(false), uPan(0),
	                     step(1.0), frac(1.0), curL(0.0), curR(0.0) {}

	void Init(Uint32 clock, Uint32 rate)
	{
		if (!rate) rate = 44100;
		Uint32 native = chip.sample_rate(clock);
		if (!native) native = rate;
		step = (double)native / (double)rate;
		frac = 1.0;                       // force a fetch on the first sample
		curL = curR = 0.0;
		Reset();
	}

	void SetPan(Uint32 pan) { uPan = pan; }
	void Disable(void)      { bEnable = false; }

	void Mix(Sample* pBuffer, int numSamples)
	{
		if (!bEnable) return;
		int maskl = (uPan & 1) ? 0 : 1;
		int maskr = (uPan & 2) ? 0 : 1;
		while (numSamples--)
		{
			double accL = 0.0, accR = 0.0, remaining = step;
			while (remaining > 0.0)
			{
				if (frac >= 1.0) { fetch(); frac -= 1.0; }
				double take = 1.0 - frac;
				if (take > remaining) take = remaining;
				accL += curL * take;
				accR += curR * take;
				frac      += take;
				remaining -= take;
			}
			accL /= step;
			accR /= step;
			*(pBuffer++) += (Sample)(accL * maskl);
			*(pBuffer++) += (Sample)(accR * maskr);
		}
	}

protected:
	// Pull one native-rate sample into curL/curR. Defined per chip because
	// OPL2 is mono and OPL3 has four output lanes.
	virtual void fetch() = 0;

	s98_ymfm_intf intf;
	ChipT  chip;
	bool   bEnable;
	Uint32 uPan;
	double step, frac, curL, curR;
};

// ---------------------------------------------------------------------------
// YM3812 (OPL2), and the OPL1 entry point, which s98mame.cpp also served with
// the OPL2 core. Mono: the single lane feeds both output channels, matching
// fmopl.c's ym3812_update_one.
// ---------------------------------------------------------------------------
class S98DEVICE_OPL2 : public S98DeviceOplBase<ymfm::ym3812>
{
public:
	void Reset(void)
	{
		chip.reset();
		// Waveform-select disabled, as s98mame.cpp did after its reset:
		// register 0x01 = 0x00.
		chip.write_address(0x01);
		chip.write_data(0x00);
	}

	void SetReg(Uint32 addr, Uint32 data)
	{
		bEnable = true;
		chip.write_address((uint8_t)(addr & 0xff));
		chip.write_data((uint8_t)data);
	}

protected:
	void fetch()
	{
		ymfm::ym3812::output_data out;
		chip.generate(&out, 1);
		curL = curR = (double)out.data[0];
	}
};

// ---------------------------------------------------------------------------
// YMF262 (OPL3). Four output lanes; L = 0 + 2, R = 1 + 3, the same pairing
// s98mame.cpp used.
// ---------------------------------------------------------------------------
class S98DEVICE_OPL3 : public S98DeviceOplBase<ymfm::ymf262>
{
public:
	void Reset(void) { chip.reset(); }

	void SetReg(Uint32 addr, Uint32 data)
	{
		bEnable = true;
		if (addr & 0x100)
			chip.write_address_hi((uint8_t)(addr & 0xff));
		else
			chip.write_address((uint8_t)(addr & 0xff));
		chip.write_data((uint8_t)data);
	}

protected:
	void fetch()
	{
		ymfm::ymf262::output_data out;
		chip.generate(&out, 1);
		curL = (double)(out.data[0] + out.data[2]);
		curR = (double)(out.data[1] + out.data[3]);
	}
};

} // namespace

S98DEVICEIF *CreateS98DeviceOPL(void)  { return new S98DEVICE_OPL2; }
S98DEVICEIF *CreateS98DeviceOPL2(void) { return new S98DEVICE_OPL2; }
S98DEVICEIF *CreateS98DeviceOPL3(void) { return new S98DEVICE_OPL3; }
