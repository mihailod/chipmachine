/*
 * Minimal replacement for SDL's little-endian byte-swap macros, used by
 * klystron's song/instrument loader (macros.h FIX_ENDIAN, cydentry.c). Only the
 * little-endian variants are referenced. klystrack files are little-endian, so
 * on an LE host these are identity; the byteswap builtins keep it correct on a
 * hypothetical BE host too.
 */
#ifndef KLYSTRON_SDL_ENDIAN_SHIM_H
#define KLYSTRON_SDL_ENDIAN_SHIM_H

#include <stdint.h>

#define SDL_LIL_ENDIAN 1234
#define SDL_BIG_ENDIAN 4321

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define SDL_BYTEORDER SDL_BIG_ENDIAN
#define SDL_SwapLE16(x) __builtin_bswap16(x)
#define SDL_SwapLE32(x) __builtin_bswap32(x)
#define SDL_SwapLE64(x) __builtin_bswap64(x)
#else
#define SDL_BYTEORDER SDL_LIL_ENDIAN
#define SDL_SwapLE16(x) (x)
#define SDL_SwapLE32(x) (x)
#define SDL_SwapLE64(x) (x)
#endif

#endif /* KLYSTRON_SDL_ENDIAN_SHIM_H */
