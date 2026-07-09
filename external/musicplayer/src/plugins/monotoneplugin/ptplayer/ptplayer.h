#include <stdint.h>

#ifndef PTPLAYER_H
#define PTPLAYER_H

#define MAX_PT_SIZE (\
	0x0D /* File header */ + \
	256 /* Order table */ + \
	256 /* Patterns */ * ( \
	64 * (1 /* Row records */ + \
	16 * 4 /* Channel entries */ )))

typedef struct {
	uint8_t note;
	uint8_t effect;
	uint8_t effectval;
	uint8_t unused;
} unpacked_t;

typedef struct {
	int orders, channels;

	uint8_t ordertable[256];

	unpacked_t data[256][64][16];
} buffer_t;

typedef struct {
	int16_t interval, freq, portafreq, effectraw;
	uint8_t enabled, active;

	uint8_t note, effect, parm1, parm2;
	uint8_t lastnote, portadelta;
	uint8_t vibspeed, vibdepth, vibindex;

	int ctr;
} channel_t;

typedef struct {
	buffer_t *buf;

	int row, order, tempo, tempotick, audiospeed, audiotick, allowadvance;
	int samplesplayed;

	channel_t channel[16];
} songstatus_t;

int PTPlayer_UnpackFile(uint8_t *filedata, buffer_t *buffer);
void PTPlayer_Reset(buffer_t *buffer);
songstatus_t *PTPlayer_GetStatus();
void PTPlayer_ProcessTick();
int PTPlayer_PlayInt16(int16_t *buf, int bufsize, int audiofreq);
int PTPlayer_PlayFloat(float *buf, int bufsize, int audiofreq);

void PTPlayer_PlayNoteInt16(uint8_t note, int16_t *buf, int bufsize, int audiofreq);
void PTPlayer_PlayNoteFloat(uint8_t note, float *buf, int bufsize, int audiofreq);

#endif
