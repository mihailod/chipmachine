#ifndef GT2_ENGINE_H
#define GT2_ENGINE_H

// Lightweight umbrella header for the vendored GoatTracker v2 playback core.
//
// Upstream gplay.c includes "goattrk2.h", which drags in the whole editor:
// BME (the author's SDL-based multimedia engine), the console/display layer,
// the file browser and every model module. The plugin only needs the sequencer
// (gplay.c) driving reSID (gsid.cpp). This header supplies exactly the symbols
// gplay.c references -- the song-data arrays (from gsong.h), the reSID register
// bank (from gsid.h) and the handful of config/editor globals -- and nothing
// else. gplay.c is patched to include this instead of goattrk2.h.

#include <string.h>

#include "gcommon.h"
#include "gsid.h"
#include "gsong.h"
#include "gplay.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Player configuration globals (defined in gt2_load.c) ---------------
// GoatTracker keeps these in goattrk2.c as user-tweakable settings. We fix them
// at GoatTracker's own defaults (PAL, 6581, 1x speed multiplier, hard-restart
// ADSR 0x0f00) since a .sng carries no such metadata.
extern unsigned multiplier;
extern unsigned adparam;
extern unsigned ntsc;
extern unsigned sidmodel;
extern unsigned finevibrato;
extern unsigned optimizepulse;
extern unsigned interpolate;
extern unsigned residdelay;
extern unsigned optimizerealtime;
extern int followplay;

// --- BME timing / SDL flush hooks the sequencer calls (no-ops here) ------
void incrementtime(void);
void resettime(void);
void sound_suspend(void);
void sound_flush(void);

// --- Editor play-position state referenced by the sequencer -------------
// Only meaningful for the editor's "play from cursor" modes; for straight
// playback from the start they stay zero, but gplay.c still reads them.
extern int espos[MAX_CHN];
extern int esend[MAX_CHN];
extern int epnum[MAX_CHN];

// --- Loader entry point (gt2_load.c) ------------------------------------
// Load a GoatTracker .sng (GTS! / GTS2 / GTS3 / GTS4 / GTS5) from a file path
// into the global song arrays. Returns the number of subsongs, 0 on failure.
int gt2_load(const char* filename);

#ifdef __cplusplus
}
#endif

#endif
