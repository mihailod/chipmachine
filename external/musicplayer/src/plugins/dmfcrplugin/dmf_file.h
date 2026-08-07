#pragma once
//
// Clean-room DefleMask .dmf parser.
//
// Written against DefleMask's own published format documentation, archived
// verbatim in spec/ (DMF_SPECS_0x11 / 0x12 / 0x13 / 0x15 / 0x16 / 0x18.txt,
// fetched from https://www.deflemask.com/). No part of this file is derived
// from Furnace's loader or from any other DMF implementation -- that is the
// whole point of it existing. See README.md.
//
// The structures below mirror the on-disk layout the spec describes, not any
// particular engine's internal song model.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace dmfcr {

// ---------------------------------------------------------------------------
// System byte.
//
// The encoding of the "mode" bit MOVED between spec revisions: DMF_SPECS_0x15
// and _0x16 document SEGA Genesis EXT.CH3 as 0x12 ("less significant nibble"),
// while _0x18 documents it as 0x42 ("bits 7 and 8 are used to set the system
// mode"). Measured over the 2,054-file corpus, DefleMask only ever WROTE 0x42 --
// 0x12 does not occur at any version. Both are accepted anyway; costing nothing
// and the spec text does claim 0x12.
enum : uint8_t {
    SYS_YMU759 = 0x01,
    SYS_GENESIS = 0x02,
    SYS_SMS = 0x03,
    SYS_GAMEBOY = 0x04,
    SYS_PCENGINE = 0x05,
    SYS_NES = 0x06,
    SYS_C64_8580 = 0x07,
    SYS_ARCADE = 0x08,
    SYS_NEOGEO = 0x09,
    SYS_GENESIS_EXT = 0x42,
    SYS_GENESIS_EXT_OLD = 0x12,
    SYS_C64_6581 = 0x47,
    SYS_NEOGEO_EXT = 0x49,
};

inline bool isGenesis(uint8_t sys)
{
    return sys == SYS_GENESIS || sys == SYS_GENESIS_EXT || sys == SYS_GENESIS_EXT_OLD;
}

inline bool isGenesisExt(uint8_t sys)
{
    return sys == SYS_GENESIS_EXT || sys == SYS_GENESIS_EXT_OLD;
}

// SYSTEM_TOTAL_CHANNELS, straight from the spec's table.
int systemChannels(uint8_t sys);

// ---------------------------------------------------------------------------

struct FmOperator
{
    // Field set as documented for version >= 0x13. Files at version <= 0x12
    // carry a larger, OPL-flavoured operator record (DAM/DVB/EGT/KSL/SUS/VIB/WS
    // in place of DT2/RS); the parser reads that layout and maps the fields
    // that exist on a YM2612 into this same struct, leaving the rest at 0.
    uint8_t am = 0;
    uint8_t ar = 0;
    uint8_t dr = 0;
    uint8_t mult = 0;
    uint8_t rr = 0;
    uint8_t sl = 0;
    uint8_t tl = 0;
    uint8_t dt2 = 0;
    uint8_t rs = 0;
    uint8_t dt = 0;
    uint8_t d2r = 0;
    uint8_t ssgMode = 0; // bit 3 = enable, bits 0-2 = mode
};

// A DefleMask macro/envelope. `loopPos` < 0 means "no loop".
struct Macro
{
    std::vector<int32_t> values;
    int loopPos = -1;
    int mode = 0; // arpeggio only: 0 = normal (relative), 1 = fixed

    bool empty() const { return values.empty(); }
};

struct Instrument
{
    std::string name;
    bool fm = false;

    // FM
    uint8_t alg = 0;
    uint8_t fb = 0;
    uint8_t lfo = 0;  // FMS on Genesis
    uint8_t lfo2 = 0; // AMS on Genesis
    FmOperator ops[4];

    // STD
    Macro volume;
    Macro arpeggio;
    Macro duty;
    Macro wavetable;
};

struct Effect
{
    int16_t code = -1;
    int16_t value = -1;

    bool present() const { return code >= 0; }
};

// One pattern cell. Note/octave use the spec's encoding: note 1..12 with
// 12 == C (see kNoteC), note 100 == note off, note 0 && octave 0 == empty.
struct Row
{
    uint16_t note = 0;
    uint16_t octave = 0;
    int16_t volume = -1;
    int16_t instrument = -1;
    Effect effects[4]; // CHANNEL_EFFECTS_COLUMNS_COUNT is capped at 4 by the editor
};

enum : uint16_t
{
    kNoteEmpty = 0,
    kNoteC = 12,   // the spec's table ends "12 C-", so C is 12, not 0
    kNoteOff = 100,
};

struct Pattern
{
    std::vector<Row> rows;
};

struct Channel
{
    int effectColumns = 1;
    std::vector<Pattern> patterns; // one per pattern-matrix row, indexed by matrix value
};

struct Sample
{
    std::string name;
    uint8_t rate = 0;
    uint8_t pitch = 0;
    uint8_t amp = 0;
    uint8_t bits = 16;
    std::vector<int16_t> data;
};

// Supported DMF version window.
//
// DefleMask published a DMF_SPECS for 0x11, 0x12, 0x13, 0x15, 0x16 and 0x18.
// 0x14 and 0x17 are undocumented but sit cleanly between their neighbours and
// were resolved by measurement (see dmf_file.cpp). 0x10 and below, and 0x19 and
// above (DefleMask 1.1+), add fields this parser does not know: all 8 such
// files in the Genesis corpus desynchronise, and there is no spec to fix them
// against. They are declined rather than mis-parsed -- in the plus build
// Furnace still plays them.
enum : uint8_t
{
    kMinVersion = 0x11,
    kMaxVersion = 0x18,
};

struct Module
{
    uint8_t version = 0;
    uint8_t system = 0;

    std::string songName;
    std::string songAuthor;
    uint8_t highlightA = 0;
    uint8_t highlightB = 0;

    uint8_t timeBase = 0;
    uint8_t tickTime1 = 6;
    uint8_t tickTime2 = 6;
    uint8_t framesMode = 1;   // 0 = PAL, 1 = NTSC
    uint8_t usingCustomHz = 0;
    uint8_t customHz[3] = { 0, 0, 0 };
    uint32_t rowsPerPattern = 64;
    uint8_t matrixRows = 1;
    uint8_t arpTickSpeed = 1; // only present (and only meaningful) below v0x15

    int totalChannels = 0;
    std::vector<std::vector<uint8_t>> matrix; // [channel][order row]
    std::vector<Instrument> instruments;
    std::vector<std::vector<int32_t>> wavetables;
    std::vector<Channel> channels;
    std::vector<Sample> samples;

    // Playback base rate in Hz, derived from framesMode / customHz.
    double baseHz() const;
};

// Inflate a .dmf. A wrong trailing Adler-32 is treated as a hard error.
//
// This was investigated rather than assumed, because it looked at first like a
// false positive worth working around. 199 of the 833 Genesis files (24%) fail
// the checksum while still producing a complete deflate stream (zlib reports
// end-of-stream) whose first bytes are a perfectly well-formed DMF header --
// correct magic, plausible version, correct system byte, intact song name. It
// is tempting to conclude the checksum is spurious and raw-inflate past it,
// which is what an earlier draft of this file did.
//
// It is not spurious. Parsed through to the end, 182 of those 199 desynchronise
// part-way in -- typically inside the PCM sample block, where a sample's data
// runs an odd number of bytes and the following sample's name arrives with its
// first character missing. The corruption is real and simply happens to fall
// after the header. Only 8 of the 190 total parse failures across the Genesis
// corpus are checksum-clean, and every one of those is a DMF version with no
// published spec (see kMinVersion/kMaxVersion below).
//
// So the strict check is correct and the Furnace-based dmfplugin is right to
// reject these files as damaged. We reject them the same way and for the same
// reason -- there is no behaviour difference to gain here, only garbage to play.
bool inflateDmf(const uint8_t* data, size_t len, std::vector<uint8_t>& out,
                std::string& err);

// Parse an already-inflated payload.
//
// `strict` additionally requires the parse to consume the buffer EXACTLY. That
// is the parser's own regression test: DMF has no length fields or section
// markers, so any mis-sized version gate desynchronises the stream and leaves
// bytes over (or runs off the end). Used by the corpus validator.
bool parseDmf(const uint8_t* data, size_t len, Module& m, std::string& err,
              bool strict = false);

// Convenience: inflate + parse.
bool loadDmf(const uint8_t* raw, size_t len, Module& m, std::string& err,
             bool strict = false);

} // namespace dmfcr
