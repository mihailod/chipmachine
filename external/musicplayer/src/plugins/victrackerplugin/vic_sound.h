/* Standalone VIC-I (MOS 6560/6561) sound core.
 *
 * The oscillator/noise/volume engine extracted verbatim from VICE's
 * vic20/vic20sound.c (GPLv2, by Rami Rasanen and Ville-Matias Heikkila); the
 * VICE sound_chip_t / SID glue is dropped. Register writes go through
 * vicsnd_store(); vicsnd_render() pulls 44100Hz mono samples for the cycles
 * fed via *delta_t. See vic_sound.c. */
#pragma once
#include <cstdint>

extern "C" {
/* cycles_per_sec = VIC master clock (PAL 1108404); speed = output rate (44100) */
void vicsnd_init(int cycles_per_sec, int speed);
/* addr = 0x0A..0x0E (VIC sound registers $900A..$900E) */
void vicsnd_store(int addr, unsigned char value);
/* Renders up to nr samples into pbuf (soc channels each), consuming *delta_t
 * VIC cycles. Returns samples produced. */
int vicsnd_render(int16_t* pbuf, int nr, int soc, int* delta_t);
}
