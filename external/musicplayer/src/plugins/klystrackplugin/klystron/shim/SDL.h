/*
 * Minimal SDL shim for the vendored klystron `snd/` + libksnd slice.
 *
 * klystron's sound core was written against SDL for three things: SDL's
 * fixed-width integer typedefs, its little-endian byte-swap macros, and its
 * mutex/audio callback plumbing. We drive libksnd manually through
 * KSND_CreatePlayerUnregistered() / KSND_FillBuffer() (see ksnd.c), and build
 * with NOSDL_MIXER and without USESDLMUTEXES / USESDL_RWOPS, so the audio
 * thread, mutex and RWops paths are all compiled out. That leaves only the
 * integer types and the endian macros, which this header provides directly so
 * the plugin has no real SDL dependency.
 */
#ifndef KLYSTRON_SDL_SHIM_H
#define KLYSTRON_SDL_SHIM_H

#include <stdint.h>
#include <signal.h>

typedef uint8_t  Uint8;
typedef int8_t   Sint8;
typedef uint16_t Uint16;
typedef int16_t  Sint16;
typedef uint32_t Uint32;
typedef int32_t  Sint32;
typedef uint64_t Uint64;
typedef int64_t  Sint64;

/* cydtypes.h does `typedef SDL_mutex * CydMutex;` unconditionally, but the
 * mutex field only exists under USESDLMUTEXES (which we do not define), so an
 * opaque forward declaration is all that is ever needed. */
typedef struct SDL_mutex SDL_mutex;

#include "SDL_endian.h"

/*
 * Inert stubs for the SDL audio-device layer.
 *
 * With NOSDL_MIXER defined, cyd_register()/cyd_unregister() open a real SDL
 * audio device (SDL_OpenAudio et al.). We never call them: the plugin uses
 * KSND_CreatePlayerUnregistered() and pulls audio synchronously via
 * KSND_FillBuffer(). That code path is therefore dead, but it still has to
 * compile and link, so we provide do-nothing definitions here rather than
 * pull in SDL. Likewise SDL_Delay only runs inside cyd_output_buffer's
 * software-lock spin, which cannot be contended when there is a single caller
 * and no audio thread, so a no-op is correct in practice.
 */
#include <stdint.h>

#ifndef AUDIO_S16SYS
#define AUDIO_S16SYS 0x8010
#endif

typedef struct SDL_AudioSpec {
    int freq;
    uint16_t format;
    uint8_t channels;
    uint16_t samples;
    void (*callback)(void* userdata, Uint8* stream, int len);
    void* userdata;
} SDL_AudioSpec;

static inline void SDL_Delay(Uint32 ms) { (void)ms; }
static inline Uint32 SDL_GetTicks(void) { return 0; }
static inline int SDL_OpenAudio(SDL_AudioSpec* desired, SDL_AudioSpec* obtained) {
    (void)desired; (void)obtained; return -1;
}
static inline void SDL_PauseAudio(int pause_on) { (void)pause_on; }
static inline void SDL_CloseAudio(void) {}

/*
 * macros.h's VER_READ() expands to SDL_RWread(ctx, ...). Without USESDL_RWOPS,
 * `ctx` is klystron's own RWops struct (music.h), whose ->read function pointer
 * has exactly SDL_RWread's (context, ptr, size, maxnum) contract, so we forward
 * to it. This is the only RWops entry point reached in our build; the other
 * SDL_RW* helpers live only in the USESDL_RWOPS branch, which we never compile.
 */
#define SDL_RWread(ctx, ptr, size, maxnum) ((ctx)->read((ctx), (ptr), (size), (maxnum)))

#endif /* KLYSTRON_SDL_SHIM_H */
