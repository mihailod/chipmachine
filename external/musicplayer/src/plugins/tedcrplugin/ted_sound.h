/* Standalone TED (MOS 7360/8360) sound core -- interface only; see ted_sound.c.
 *
 * Register writes go through tedsnd_store(); tedsnd_render() pulls output
 * samples for the TED sound cycles fed via *delta_t.
 */
#pragma once
/* Included from both C++ (the machine layer) and C (ted_sound.c), hence
 * <stdint.h> and the guarded linkage block. */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The sound counters are clocked at one quarter of the CPU clock:
 *   PAL   886723.75 / 4 = 221681
 *   NTSC  894886.25 / 4 = 223722
 * speed = output sample rate (44100). */
#define TEDSND_CLOCK_PAL 221681
#define TEDSND_CLOCK_NTSC 223722

void tedsnd_init(int cycles_per_sec, int speed);
void tedsnd_reset(void);

/* addr = 0x0E..0x12 (TED sound registers $FF0E..$FF12). Bits the sound side
 * does not own -- the bitmap base field in $FF12, the unused bits of $FF10 --
 * are ignored here, so the machine can pass the whole byte through. */
void tedsnd_store(int addr, unsigned char value);

/* Renders up to nr samples into pbuf (soc channels each), consuming *delta_t
 * TED sound cycles. Returns samples produced. */
int tedsnd_render(int16_t* pbuf, int nr, int soc, int* delta_t);

#ifdef __cplusplus
}
#endif
