#pragma once

// A small two-pass Z80 assembler, ported from 1tracker's z80ass.1tl (Shiru).
// It exists so the beeper-engine player sources (vendored .1te assembly) can be
// assembled in-repo with no external toolchain. Scope matches what those
// engines use: org/equ/db/dw/ds/align/if-endif, dotted local labels, '#' hex /
// '%' binary numbers, and left-to-right expressions -- plus the full Z80
// instruction table (z80_opcodes.h).
//
// Dialect notes (from z80ass.1tl): numbers are decimal by default, '#' prefix
// = hex, '%' = binary; '$' in an expression is the current address; labels may
// contain a-z 0-9 _ and '.', where a leading '.' makes the label local to the
// most recent global label's scope. Expressions have NO operator precedence
// (strict left-to-right) with + - * / & | ^.

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace musix::bbsong {

class Z80Assembler
{
public:
    // Assembles `source` with the origin at `org`. On success returns the number
    // of bytes emitted and fills code()/labels(); on error returns -1 and sets
    // error()/errorMessage(). The emitted bytes land in a 64K image at their
    // absolute addresses; code(addr) reads that image.
    int assemble(const std::string& source, uint16_t org);

    const std::vector<uint8_t>& image() const { return mem_; } // full 64K
    // Address of a label, or -1 if undefined. Used to find the player entry
    // point and the music_data slot.
    int label(const std::string& name) const;

    bool error() const { return stop_; }
    const std::string& errorMessage() const { return errorMsg_; }

private:
    // --- normalized-source cursor ---
    std::string src_;       // cleaned, lowercased, label-marked source
    size_t ptr_ = 0;
    size_t ptrOld_ = 0;
    std::string origSource_; // for error messages

    struct Line
    {
        size_t off = 0;  // offset into src_
        int adr = 0;     // PC at this line (set in pass 1)
        bool compile = false; // needs recompiling in pass 2 (had fwd label ref)
    };
    std::vector<Line> lines_;
    int lineIdx_ = 0;
    int linesAll_ = 0;

    struct Label
    {
        std::string name;
        int adr = 0;
    };
    std::vector<Label> labelList_;
    std::string labelScope_;

    std::vector<uint8_t> mem_{std::vector<uint8_t>(0x10000, 0)};
    int pc_ = 0;
    int org_ = 0;
    int pass_ = 1;
    bool stop_ = false;
    std::string errorMsg_;
    int paramValue_[2] = {0, 0};

    // opcode-table acceleration (built once)
    std::vector<int> opLen_;       // index of '|' per entry
    int letterStartA_[256];
    int letterStartB_[256];
    int letterLenA_[256];
    int letterLenB_[256];
    bool tableBuilt_ = false;

    void buildTable();
    void cleanup(const std::string& src); // pass 0: build src_ + lines_

    void err(const std::string& msg);
    bool compareSrc(const char* str) const;
    void emit(int value);
    void emitByte(int value);
    void emitWord(int value);
    int relativeOffset(int value);
    int parseNumber();
    int parseEquation();
    void parseDataArray(int unitSize);
    bool parseDirective();
    void parseLabel();
    int parseOpcodePart(int start, int count, int rowLen);
    void parseOpcode();
};

} // namespace musix::bbsong
