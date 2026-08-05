// Bandai WonderSwan / WonderSwan Color sound -- chipmachine's own implementation.
//
// Written from the WSdev Wiki's register documentation (https://ws.nesdev.org,
// pages "Sound", "Hyper Voice" and "DMA"), not derived from any emulator. See
// README.md next to this file for the details and the traps.
//
// Two users:
//   * libvgmplugin/ws_audio_cm.c -- a libvgm DEV_DEF wrapper, replacing that
//     library's in_wsr-derived emu/cores/ws_audio.c. VGM logs can only reach
//     ports $80 and up, so they never touch sound DMA or Hyper Voice.
//   * the .wsr player next door, which drives the whole machine and does.

#ifndef WS_APU_H
#define WS_APU_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WSAPU_CHANNELS	4

// Sound DMA streams from anywhere in the 1 MB space, so it cannot read through
// the wavetable RAM pointer; the machine supplies this instead. Left NULL,
// sound DMA reads as 0.
typedef uint8_t (*wsapu_read_cb)(void* ctx, uint32_t address);

typedef struct
{
	uint16_t period;		// 2048 - divisor, i.e. master cycles per wave step
	uint8_t silent;			// frequency register held $FFFF
	uint32_t phase;			// 16.16 index into the 32-step waveform
} WSAPU_CHANNEL;

typedef struct
{
	uint32_t clock;
	uint32_t rate;
	uint32_t cyclesPerSmpl;	// clock / rate, 16.16

	WSAPU_CHANNEL ch[WSAPU_CHANNELS];

	uint16_t lfsr;			// 15-bit noise shift register
	uint32_t sweepTickSmpls;	// output samples per 375 Hz tick, 16.16
	uint32_t sweepAccum;	// 16.16 remainder of the above
	uint8_t sweepTicks;		// ticks elapsed towards ($8D & 0x1F) + 1

	uint8_t ioRam[0x100];	// ports $40-$9E as written

	uint8_t* ram;			// wavetable source (internal RAM)
	uint32_t ramMask;

	wsapu_read_cb read;		// sound DMA source
	void* readCtx;

	// --- sound DMA -------------------------------------------------------
	uint32_t dmaSource, dmaLength;		// live counters
	uint32_t dmaSourceShadow, dmaLengthShadow;
	uint32_t dmaTickSmpls;	// output samples per DMA byte, 16.16
	uint32_t dmaAccum;

	// --- Hyper Voice (WonderSwan Color) ----------------------------------
	int16_t hvLeft, hvRight;
	uint8_t hvChannel;		// which side the next stereo sample belongs to

	uint8_t muteMask;
} WSAPU;

// clock is the master clock (3072000 on real hardware); rate is the output
// sample rate. Hardware runs its DAC at clock / 128 = 24000 Hz.
void wsapu_init(WSAPU* apu, uint32_t clock, uint32_t rate);
void wsapu_reset(WSAPU* apu);

// The wavetable lives in the machine's internal RAM; the APU only reads it.
void wsapu_set_ram(WSAPU* apu, uint8_t* ram, uint32_t ramMask);
void wsapu_set_dma_reader(WSAPU* apu, wsapu_read_cb read, void* ctx);

void wsapu_write_port(WSAPU* apu, uint8_t port, uint8_t value);
uint8_t wsapu_read_port(WSAPU* apu, uint8_t port);

// Adds nothing of its own to the buffers -- they are overwritten, one int32 per
// sample per side, at the headphone mix point.
void wsapu_render(WSAPU* apu, int32_t* left, int32_t* right, uint32_t samples);

void wsapu_set_mute_mask(WSAPU* apu, uint32_t muteMask);

#ifdef __cplusplus
}
#endif

#endif
