#ifndef NED_ENGINE_H
#define NED_ENGINE_H

/* Thin C API over the vendored NerdTracker II (.ned) replay engine
 * (ned_engine.c). The engine keeps global state, so only one song may be
 * loaded/playing at a time -- NEDPlugin serializes access. */

#ifdef __cplusplus
extern "C" {
#endif

/* Load a .ned module from disk. Returns 1 on success, 0 on failure. */
int  ned_engine_load(const char *path);

/* Begin playback from the start of the order list. Returns 1 on success. */
int  ned_engine_start(void);

/* Render 'frames' mono signed-16-bit samples into buf. */
void ned_engine_render(short *buf, int frames);

/* Release the APU side. */
void ned_engine_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* NED_ENGINE_H */
