#ifndef _SOUNDDRIVER_DUMMY_H_
#define _SOUNDDRIVER_DUMMY_H_

#include <ptk_types.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define STDCALL

#define AUDIO_PCM_FREQ 44100
#define AUDIO_DBUF_CHANNELS 2
#define AUDIO_DBUF_RESOLUTION 16

extern int AUDIO_Latency;
extern int AUDIO_Milliseconds;
extern int done;

inline int AUDIO_Init_Driver(uint32_t (STDCALL *Mixer)(UINT8 *, UINT32)) { return 1; }
inline void AUDIO_Play(void) {}
inline void AUDIO_Stop(void) {}
inline void AUDIO_Stop_Driver(void) {}
inline void AUDIO_ResetTimer(void) {}
inline int AUDIO_GetSamples(void) { return 0; }

#endif
