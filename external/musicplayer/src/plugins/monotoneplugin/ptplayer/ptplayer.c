#include "ptplayer.h"
#include <math.h>
#include <string.h>

#define maxchannels 16
#define IBN 16
#define maxnote (3 + (8 * 12) + 1)
#define vibtablesize 64
#define vibtabledepth (IBN * 8)

/* chipmachine local patch: these were file-scope globals with very generic
 * names (`s`, `notesHz`, `VibTable`, `notectr`) that collide at full-link time
 * with other vendored plugins. Make them static -- every accessor lives in this
 * TU, so PTPlayer_GetStatus() still works. Re-apply on any PTPlayer revendor. */
static songstatus_t s;

static short notesHz[maxnote * IBN];
static short VibTable[vibtablesize];

int PTPlayer_UnpackFile(uint8_t *filedata, buffer_t *buffer) {
	if(!memcmp(filedata, "\x08MONOTONE", 9)) {
		if(filedata[0x5D] > maxchannels)
			return -1;

		// Convert Monotone data

		int orders;

		for(orders = 0; orders < 256; orders++) {
			if(filedata[0x5F + orders] == 0xFF) break;

			buffer->ordertable[orders] = filedata[0x5F + orders];
		}

		buffer->orders = orders;
		buffer->channels = filedata[0x5D];

		uint16_t *in = (uint16_t *)(filedata + 0x15F);
		int ptr;
		unpacked_t *i;

		memset(buffer->data, 0, sizeof(buffer->data));

		const uint8_t effect_lut[] = {
			0, 1, 2, 3, 4, 0xB, 0xD, 0xF
		};

		for(int p = 0; p < filedata[0x5C]; p++) {
			for(int r = 0; r < 64; r++) {
				for(int c = 0; c < buffer->channels; c++) {
					ptr = ((p * 64) + r) * buffer->channels + c;
					i = &buffer->data[p][r][c];
					i->note = in[ptr] >> 9;
					i->effect = effect_lut[(in[ptr] >> 6) & 7];
					i->effectval = (i->effect == 0 || i->effect == 4) ?
					(((in[ptr] & 0x38) << 1) | (in[ptr] & 7)) :
					(in[ptr] & 0x3F);

					if(i->effect == 4) i->effectval *= 2;
				}
			}
		}

		PTPlayer_Reset(buffer);

		return 0;
	} else if(!memcmp(filedata, "\x08POLYTONE\x01", 10)) {
		if(filedata[0x0B] > maxchannels)
			return -1;

		// Unpack Polytone data

		buffer->channels = filedata[0x0B];
		buffer->orders = (filedata[0x0C]) ? filedata[0x0C] : 256;

		memcpy(buffer->ordertable, filedata + 0x0D, buffer->orders);

		uint8_t *in = filedata + 0x0D + buffer->orders;
		uint8_t rowid, chnlid, eff;
		unpacked_t *i;
		uint8_t oldeff[16];

		memset(buffer->data, 0, sizeof(buffer->data));

		for(int p = 0; p < filedata[0x0A]; p++) {
			memset(oldeff, 0, sizeof(oldeff));

			for(int r = 0; r < 64; r++) {
				if(((rowid = *in) & 63) == r) {
					in++;

					if(!(rowid & 64)) for(int c = 0; c < buffer->channels; c++) {
						if(((chnlid = *in) & 15) == c) {
							in++;

							i = &buffer->data[p][r][c];

							if(chnlid & 0x40) {
								i->note = *(in++);
							}

							if(chnlid & 0x20) {
								eff = *(in++);

								i->effect = eff & 0xF;

								if((eff >> 4) == 0xE) {
									i->effectval = oldeff[c];
								} else if((eff >> 4) == 0xF) {
									i->effectval = *(in++);
								} else {
									i->effectval = eff >> 4;
								}

								oldeff[c] = i->effectval;
							}

							if(chnlid & 0x80) break;
						}
					}

					if(rowid & 128) break;
				}
			}
		}

		PTPlayer_Reset(buffer);

		return 0;
	} else {
		return -1;
	}
}

void PTPlayer_Reset(buffer_t *buffer) {
	memset(&s, 0, sizeof(songstatus_t));

	// Populate songstatus

	s.buf = buffer;
	s.row = -1;
	s.order = 0;
	s.tempotick = 1;
	s.audiotick = 1;
	s.allowadvance = 1;

	s.tempo = (s.buf->channels > 4) ? s.buf->channels : 4;
	s.audiospeed = 60;

	for(int ch = 0; ch < maxchannels; ch++) {
		s.channel[ch].enabled = 1;
		s.channel[ch].active = 0;
	}

	s.samplesplayed = 0;

	// Generate vibrato table

	double interval = pow(2, 1.0 / (12 * IBN));
	notesHz[0] = 440;
	double temphz = 27.5;
	notesHz[IBN] = round(temphz);

	for(int i = IBN - 1; i >= 1; i--) {
		temphz /= interval;
		if(temphz < 19) temphz = 19;
		notesHz[i] = round(temphz);
	}

	temphz = 27.5;

	for(int i = IBN + 1; i < maxnote * IBN; i++) {
		temphz *= interval;
		notesHz[i] = round(temphz);
	}

	for(int i = 0; i < vibtablesize; i++) {
		VibTable[i] = round(vibtabledepth * sin(((double)i) * M_PI / vibtablesize * 2));
	}
}

songstatus_t *PTPlayer_GetStatus() {
	return &s;
}

void PTPlayer_ProcessTick() {
	int i, ch;

	if(!--s.tempotick && s.allowadvance != 0) {
		// Process a new row

		if(s.allowadvance == -1) s.allowadvance = 0;

		if(++s.row >= 0x40) {
			// Process the next order

			s.order++;
			s.row = 0;
		}

		for(ch = 0; ch < s.buf->channels; ch++) {
			switch(s.channel[ch].effect) {
				case 0xB:
					s.order = s.channel[ch].parm1;
					s.row = 0;
					break;

				case 0xD:
					s.order++;
					s.row = s.channel[ch].parm1;
					break;
			}
		}

		if(s.order >= s.buf->orders) s.order = 0;

		unpacked_t *rowdata = s.buf->data[s.buf->ordertable[s.order]][s.row];

		for(ch = 0; ch < s.buf->channels; ch++) {
			s.channel[ch].note = rowdata[ch].note;
			s.channel[ch].effect = rowdata[ch].effect;
			s.channel[ch].effectraw = rowdata[ch].effectval;

			if(s.channel[ch].effect == 0 || s.channel[ch].effect == 4) {
				s.channel[ch].parm1 = (rowdata[ch].effectval >> 4) & 0xF;
				s.channel[ch].parm2 = rowdata[ch].effectval & 0xF;
			} else {
				s.channel[ch].parm1 = rowdata[ch].effectval;
				s.channel[ch].parm2 = 0;
			}

			if(s.channel[ch].note == 127) {
				s.channel[ch].active = 0;
			} else if(s.channel[ch].note && s.channel[ch].note <= maxnote && s.channel[ch].effect != 3) {
				s.channel[ch].active = 1;
				s.channel[ch].interval = s.channel[ch].note * IBN;
				s.channel[ch].freq = notesHz[s.channel[ch].interval];
				s.channel[ch].lastnote = s.channel[ch].note;
				s.channel[ch].vibindex = 0;
				s.channel[ch].ctr = 0;
			}

			if(s.channel[ch].effect) switch(s.channel[ch].effect) {
				case 3:
					if(s.channel[ch].note && s.channel[ch].note <= maxnote)
						s.channel[ch].portafreq = notesHz[s.channel[ch].note * IBN];
					
					if(s.channel[ch].parm1) s.channel[ch].portadelta = s.channel[ch].parm1;
					break;

				case 4:
					if(s.channel[ch].parm1) s.channel[ch].vibspeed = s.channel[ch].parm1;
					if(s.channel[ch].parm2) s.channel[ch].vibdepth = s.channel[ch].parm2;
					s.channel[ch].vibindex += s.channel[ch].vibspeed;
					s.channel[ch].vibindex %= vibtablesize;
					break;

				case 0xF:
					if(s.channel[ch].parm1 > 0x20) {
						s.audiospeed = s.channel[ch].parm1;
					} else {
						s.tempo = s.channel[ch].parm1;
					}

					break;
			}
		}

		s.tempotick = s.tempo;
	} else {
		// Process effects

		for(ch = 0; ch < s.buf->channels; ch++) {
			if(s.channel[ch].effect || s.channel[ch].effectraw) {
				switch(s.channel[ch].effect) {
					case 0:
						switch((s.tempo - s.tempotick) % 3) {
							case 0:
								s.channel[ch].interval = s.channel[ch].lastnote * IBN;
								break;

							case 1:
								s.channel[ch].interval = (s.channel[ch].lastnote + s.channel[ch].parm1) * IBN;
								break;

							case 2:
								s.channel[ch].interval = (s.channel[ch].lastnote + s.channel[ch].parm2) * IBN;
								break;
						}

						s.channel[ch].freq = notesHz[s.channel[ch].interval];
						break;

					case 1:
						s.channel[ch].freq += s.channel[ch].parm1;
						// I mean, it's not _really_ necessary to clamp the upper frequency limit
						break;

					case 2:
						s.channel[ch].freq -= s.channel[ch].parm1;
						if(s.channel[ch].freq < 20) s.channel[ch].freq = 20;
						break;

					case 3:
						if(s.channel[ch].freq < s.channel[ch].portafreq) {
							s.channel[ch].freq += s.channel[ch].portadelta;

							if(s.channel[ch].freq > s.channel[ch].portafreq) {
								s.channel[ch].freq = s.channel[ch].portafreq;
							}
						}
						
						if(s.channel[ch].freq > s.channel[ch].portafreq) {
							s.channel[ch].freq -= s.channel[ch].portadelta;

							if(s.channel[ch].freq < s.channel[ch].portafreq) {
								s.channel[ch].freq = s.channel[ch].portafreq;
							}
						}
						break;

					case 4:
						s.channel[ch].freq = notesHz[s.channel[ch].interval + (((VibTable[s.channel[ch].vibindex]) * s.channel[ch].vibdepth) / vibtabledepth)];
						s.channel[ch].vibindex += s.channel[ch].vibspeed;
						s.channel[ch].vibindex %= vibtablesize;
						break;
				}
			}
		}
	}
}

int PTPlayer_PlayInt16(int16_t *buf, int bufsize, int audiofreq) {
	int i, ch;

	memset(buf, 0, bufsize * sizeof(short));

	for(i = 0; i < bufsize; i++) {
		// Process tick if necessary

		if(!--s.audiotick) {
			PTPlayer_ProcessTick();

			s.audiotick = audiofreq / s.audiospeed;
		}

		// Render the sound

		for(ch = 0; ch < s.buf->channels; ch++) {
			if(s.channel[ch].active && s.channel[ch].enabled) {
				s.channel[ch].ctr += s.channel[ch].freq;

				if((s.channel[ch].ctr / (audiofreq / 2)) & 1) {
					buf[i] += 32767 / s.buf->channels;
				}
			}
		}
	}

	return s.samplesplayed += bufsize;
}

int PTPlayer_PlayFloat(float *buf, int bufsize, int audiofreq) {
	int i, ch;

	memset(buf, 0, bufsize * sizeof(float));

	for(i = 0; i < bufsize; i++) {
		// Process tick if necessary

		if(!--s.audiotick) {
			PTPlayer_ProcessTick();

			s.audiotick = audiofreq / s.audiospeed;
		}

		// Render the sound

		for(ch = 0; ch < s.buf->channels; ch++) {
			if(s.channel[ch].active && s.channel[ch].enabled) {
				s.channel[ch].ctr += s.channel[ch].freq;

				if((s.channel[ch].ctr / (audiofreq / 2)) & 1) {
					buf[i] += 1.0 / ((float)s.buf->channels);
				}
			}
		}
	}

	return s.samplesplayed += bufsize;
}

static int notectr = 0;

void PTPlayer_PlayNoteInt16(uint8_t note, int16_t *buf, int bufsize, int audiofreq) {
	int i;

	memset(buf, 0, bufsize * sizeof(int16_t));

	for(i = 0; i < bufsize; i++) {
		// Render the sound

		notectr += notesHz[note * IBN];

		if((notectr / (audiofreq / 2)) & 1) {
			buf[i] += 32767 / s.buf->channels;
		}
	}
}

void PTPlayer_PlayNoteFloat(uint8_t note, float *buf, int bufsize, int audiofreq) {
	int i;

	memset(buf, 0, bufsize * sizeof(float));

	for(i = 0; i < bufsize; i++) {
		// Render the sound

		notectr += notesHz[note * IBN];

		if((notectr / (audiofreq / 2)) & 1) {
			buf[i] += 1.0 / ((float)s.buf->channels);
		}
	}
}
