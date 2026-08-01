#ifndef MUS_H
#define MUS_H
#include "../csidplugin/csid/csid_engine.h"
// Second SID chip base for Stereo Sidplayer voices 4-6 (the address VICE and
// the 2SID convention use).
#define MUS_SID2_BASE 0xD420u
typedef struct MusPlayer MusPlayer;
MusPlayer* mus_create(const unsigned char* mus, int mus_size,
                      const unsigned char* str, int str_size, int samplerate);
#ifdef MUS_TRACE_HOOKS
// Offline-only: dump every SID register write with its timestamp, for diffing
// against a VICE oracle trace. Not compiled into the shipping plugin.
void mus_set_reg_trace(const char* path);
void mus_trace_set_time(double t);
void mus_set_event_trace(const char* path);
#endif

void mus_destroy(MusPlayer* p);
// Writes `frames` INTERLEAVED STEREO frames (2*frames int16). A Stereo
// Sidplayer pair puts its first SID left and its second right, as VICE does.
int mus_render(MusPlayer* p, short* out, int frames);
int mus_finished(MusPlayer* p);
const char* mus_title_line(MusPlayer* p, int line);
#endif
