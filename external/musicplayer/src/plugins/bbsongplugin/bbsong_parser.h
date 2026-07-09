#pragma once

// Parser for Beepola .bbsong files (ZX Spectrum 1-bit beeper music).
//
// The format (documented at freestuff.grok.co.uk/beepola/help/bbsong-fileformat.html)
// is a 12-byte header -- "BBSONG\0" + "0001\0" -- followed by a sequence of
// named chunks. Each chunk begins with a null-terminated type string (e.g.
// ":INFO", ":LAYOUT", ":PATTERNDATA") and ends with the null-terminated marker
// ":END". Between those, chunk content is a mix of null-terminated
// "Name=Value" string properties and raw binary blobs whose layout depends on
// the chunk (and, for note data, on the song's engine).
//
// This parser extracts the *engine-independent* structure: the header/version,
// the :INFO key/value metadata (including the engine id), the :LAYOUT order
// list, and a split of :PATTERNDATA into per-pattern records (name, length,
// tempo, and the raw channel bytes). Interpreting those raw channel bytes into
// notes is the job of each engine's data packer, since the column layout and
// per-row stride differ per engine. Every chunk's raw payload is also retained
// so packers can reach engine-specific chunks (e.g. :EXTPATTERNDATA,
// :P1INSTR) directly.

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace musix::bbsong {

// One pattern as stored in :PATTERNDATA. `channelData` is the raw bytes between
// this pattern's header and the next pattern (or the chunk's :END); a packer
// slices it using its engine's known per-row stride and `length`.
struct Pattern
{
    std::string name;
    uint32_t length = 0; // number of rows
    uint32_t tempo = 0;
    std::vector<uint8_t> channelData;
};

// A raw chunk: its type string (without the trailing ':END') and the bytes of
// its payload (everything between the type string's terminator and ':END').
struct Chunk
{
    std::string type; // e.g. ":INFO", ":PATTERNDATA"
    std::vector<uint8_t> payload;
};

struct Song
{
    std::string version;                       // e.g. "0001"
    std::map<std::string, std::string> info;   // :INFO properties
    std::string engine;                        // :INFO "Engine" (e.g. "TRI", "QCN", "P1D")
    std::string title;
    std::string author;

    uint32_t loopStart = 0;
    uint32_t length = 0;          // number of entries in the order list
    std::vector<uint8_t> order;   // pattern indices, in play order

    uint32_t patternCount = 0;
    std::vector<Pattern> patterns;

    std::vector<Chunk> chunks;    // every chunk, raw, in file order

    // Returns the raw payload of the first chunk with the given type, or
    // nullptr if absent. Lets engine packers reach chunks this struct doesn't
    // model explicitly (e.g. ":EXTPATTERNDATA", ":P1INSTR", ":SVGORNAMENTS").
    const std::vector<uint8_t>* chunk(const std::string& type) const;
};

// Parses a .bbsong image. Throws std::runtime_error on a malformed file
// (bad magic, truncated chunk, etc).
Song parse(const uint8_t* data, size_t size);

} // namespace musix::bbsong
