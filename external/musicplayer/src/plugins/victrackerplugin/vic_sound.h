/* Standalone VIC-I (MOS 6560/6561) sound core -- interface only; see
 * vic_sound.c.
 *
 * Register writes go through vicsnd_store(); vicsnd_render() pulls 44100Hz mono
 * samples for the cycles fed via *delta_t.
 */
#pragma once
// Included from both C++ (vt_machine.cpp) and C (vic_sound.c), hence <stdint.h>
// and the guarded linkage block.
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
/* cycles_per_sec = VIC master clock (PAL 1108404); speed = output rate (44100) */
void vicsnd_init(int cycles_per_sec, int speed);
/* addr = 0x0A..0x0E (VIC sound registers $900A..$900E) */
void vicsnd_store(int addr, unsigned char value);
/* Renders up to nr samples into pbuf (soc channels each), consuming *delta_t
 * VIC cycles. Returns samples produced. */
int vicsnd_render(int16_t* pbuf, int nr, int soc, int* delta_t);

#ifdef __cplusplus
}
#endif
