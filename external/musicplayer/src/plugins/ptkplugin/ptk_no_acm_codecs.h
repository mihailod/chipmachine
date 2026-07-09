// Force-included (via -include) into samples_unpack.cpp ONLY.
//
// In BZR2 + __WINAMP__ mode the engine builds with the full ptk_def_properties.h,
// which enables PTK_GSM / PTK_MP3 / PTK_AT3. The loaders and replayer need those
// macros (for the Mp3_BitRate/At3_BitRate header fields and the pack-type switch
// structure), so they stay enabled globally. But the *decoder bodies* for those
// three formats live in samples_unpack.cpp and are implemented on top of the
// Windows ACM codec API (msacm), which does not exist on macOS / Linux.
//
// Disabling the three macros for just this translation unit drops the msacm
// bodies while keeping the portable WavPack / ADPCM / 8-bit decoders that real
// modules use. PTKPlugin.cpp provides no-op stubs for the (never-reached on
// these platforms) GSM/MP3/AT3 entry points so the replayer still links.
#pragma once

// Pull in the property set first so its include guard is set; our #undefs then
// survive the (now no-op) include that samples_unpack.h does later.
#include <ptk_def_properties.h>

#undef PTK_GSM
#undef PTK_MP3
#undef PTK_AT3
