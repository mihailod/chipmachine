// Thin C bridge over the vendored PlayerPRO MADDriver (repo-root playerpro/).
// Kept in C so the engine's C headers -- which redefine bool/OSType/etc. -- never
// reach the C++ plugin translation unit. The plugin only ever sees this header.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PPRender PPRender;

// Loads the module at `path` and starts the offline (NoHardwareDriver) engine.
// Returns NULL if the file isn't a PlayerPRO module we can play, or on error.
PPRender* pprender_open(const char* path);

// Native output sample rate (Hz).
int pprender_hz(PPRender* r);

// Renders up to `maxFrames` stereo frames into `out` (interleaved L,R int16).
// Returns the number of frames produced; 0 once the song has ended.
int pprender_fill(PPRender* r, short* out, int maxFrames);

void pprender_close(PPRender* r);

#ifdef __cplusplus
}
#endif
