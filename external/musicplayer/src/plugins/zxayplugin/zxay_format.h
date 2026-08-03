#pragma once

// What kind of ZX AY file is this, and who plays it?
//
// Detection is by CONTENT first and extension only as a tie-break, because the
// extension lies constantly in this corpus: ".zxs" and ".st13" are both Sound
// Tracker compiled modules, ".stp2" is plain Sound Tracker Pro, and modland
// hands us ".pt2" files that are actually Picatune2 XML. Every detector here
// was written against the format documentation in players/source/ and checked
// against the fixtures in testmus/zx.

#include <cstddef>
#include <cstdint>
#include <string>

namespace musix::zxay {

enum class Format
{
    unknown,
    pt1,  // Pro Tracker 1.xx          -- Z80 player
    pt2,  // Pro Tracker 2.x           -- Z80 player (PTxPlay)
    pt3,  // Pro Tracker 3.x / Vortex  -- Z80 player (PTxPlay)
    stc,  // Sound Tracker 1.x compiled (also .zxs, .st13) -- native
    stp,  // Sound Tracker Pro (also .stp2)                -- Z80 player
    asc,  // ASC Sound Master 0.x / 1.x                    -- native
    psc,  // Pro Sound Creator         -- Z80 player
    sqt,  // SQ-Tracker                -- Z80 player
    vtx,  // Vortex register dump (LZH-packed)             -- dump
    psg,  // Raw AY register dump                          -- dump
    fxm,  // Fuxoft AY Language ("FXSM")                   -- native
    amad, // AY Amadeus (ZXAYAMAD container over FXM)      -- native
    vt2,  // Vortex Tracker II text module   -- Arkos Tracker 3 importer
    ftc,  // Fast Tracker               -- Z80 player
    psm,  // Pro Sound Maker            -- native
    gtr,  // Global Tracker             -- native
    st11, // Sound Tracker 1.1 uncompiled, in a ZXAY container
          //                            -- compiled to .stc, then native
};

// Human-readable name for the "format" metadata column, e.g. "Pro Tracker 3".
const char* formatName(Format f);

// Content detection. `ext` is the lower-cased file extension with no dot; it is
// consulted only where the content alone is genuinely ambiguous.
Format detect(const uint8_t* data, size_t len, const std::string& ext);

// True if this plugin should claim the name at all (extension gate, cheap).
bool isSupportedExtension(const std::string& ext);

} // namespace musix::zxay
