#include "z80asm.h"

#include "z80_opcodes.h"

#include <cstring>

namespace musix::bbsong {

namespace {

bool labelCharAllowed(int c)
{
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' ||
           c == '.';
}

int hexChar(int c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    return c - 'a' + 10;
}

constexpr int NOT_A_NUMBER = 0x7fffffff;

} // namespace

void Z80Assembler::buildTable()
{
    if (tableBuilt_) {
        return;
    }
    int n = static_cast<int>(sizeof(Z80_OPCODES) / sizeof(Z80_OPCODES[0]));
    opLen_.resize(n);
    for (int i = 0; i < n; i++) {
        const char* bar = std::strchr(Z80_OPCODES[i], '|');
        opLen_[i] = static_cast<int>(bar - Z80_OPCODES[i]);
    }
    for (int i = 0; i < 256; i++) {
        letterStartA_[i] = letterStartB_[i] = -1;
        letterLenA_[i] = letterLenB_[i] = 0;
    }
    for (int i = 0; i < n; i++) {
        int c = static_cast<unsigned char>(Z80_OPCODES[i][0]);
        if (c != ':') {
            if (letterStartA_[c] < 0) {
                letterStartA_[c] = i;
            }
            letterLenA_[c]++;
        } else {
            c = static_cast<unsigned char>(Z80_OPCODES[i][1]);
            if (letterStartB_[c] < 0) {
                letterStartB_[c] = i;
            }
            letterLenB_[c]++;
        }
    }
    tableBuilt_ = true;
}

void Z80Assembler::err(const std::string& msg)
{
    if (stop_) {
        return;
    }
    errorMsg_ = "Z80 assembler error: " + msg;
    stop_ = true;
}

bool Z80Assembler::compareSrc(const char* str) const
{
    size_t len = std::strlen(str);
    if (ptr_ + len > src_.size()) {
        return false;
    }
    return src_.compare(ptr_, len, str) == 0;
}

void Z80Assembler::emit(int value)
{
    mem_[pc_ & 0xFFFF] = static_cast<uint8_t>(value & 0xFF);
    pc_++;
}

void Z80Assembler::emitByte(int value)
{
    if (value < -255 || value > 255) {
        err("truncated to 8-bit!");
    }
    emit(value);
}

void Z80Assembler::emitWord(int value)
{
    if (value < -65535 || value > 65535) {
        err("truncated to 16-bit!");
    }
    emit(value & 0xFF);
    emit((value >> 8) & 0xFF);
}

int Z80Assembler::relativeOffset(int value)
{
    if (value < -128 || value > 127) {
        err("relative offset out of range -128..127!");
    }
    if (value < 0) {
        value = ((0 - value - 1) ^ 0xff) & 0xff;
    }
    return value;
}

int Z80Assembler::parseNumber()
{
    int base = 10;
    int c = src_[ptr_];
    if (!(c >= '0' && c <= '9') && c != '#' && c != '%') {
        return NOT_A_NUMBER;
    }
    if (c == '#') {
        base = 16;
    }
    if (c == '%') {
        base = 2;
    }
    if (base != 10) {
        ptr_++;
    }
    int value = 0;
    while (ptr_ < src_.size()) {
        c = src_[ptr_];
        if (c == '\n') {
            break;
        }
        if (c >= '0' && c <= '9') {
            c = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            c = c - 'a' + 10;
        }
        if (c < 0 || c >= base) {
            break;
        }
        value = value * base + c;
        ptr_++;
    }
    return value;
}

// Strict left-to-right expression evaluator: $ = current address, numbers
// (#hex/%bin/decimal), and labels (.-prefixed = scoped local). Operators
// + - * / & | ^ with no precedence.
int Z80Assembler::parseEquation()
{
    int result = 0;
    int value = 0;
    char alu = 0;
    char nalu = '='; // first value seeds the result

    while (ptr_ < src_.size()) {
        char c = src_[ptr_];
        if (c == '\n' || c == ' ' || c == ')' || c == ',') {
            break;
        }

        if (c == '$') { // current address
            ptr_++;
            value = pc_;
            alu = nalu;
            nalu = 0;
        } else {
            int n = parseNumber();
            if (n != NOT_A_NUMBER) {
                value = n;
                alu = nalu;
                nalu = 0;
            } else if ((c >= 'a' && c <= 'z') || c == '_' || c == '.') {
                // label
                std::string label = (c != '.') ? "" : labelScope_;
                while (ptr_ < src_.size()) {
                    c = src_[ptr_];
                    if (!labelCharAllowed(c)) {
                        break;
                    }
                    label += static_cast<char>(c);
                    ptr_++;
                }
                if (pass_ == 1) {
                    value = 0;
                    lines_[lineIdx_].compile = true;
                } else {
                    bool found = false;
                    for (const auto& l : labelList_) {
                        if (l.name == label) {
                            value = l.adr;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        err("label '" + label + "' is not defined!");
                        return 0;
                    }
                    alu = nalu;
                    nalu = 0;
                }
            } else {
                // operator
                if (nalu != 0 && nalu != '=') {
                    err("unexpected operation in an equation!");
                    return 0;
                }
                nalu = c;
                ptr_++;
            }
        }

        if (alu != 0) {
            switch (alu) {
            case '=':
            case '+': result += value; break;
            case '-': result -= value; break;
            case '*': result *= value; break;
            case '/': result /= value; break;
            case '&': result &= value; break;
            case '|': result |= value; break;
            case '^': result ^= value; break;
            default: err("unexpected character in an equation!"); return 0;
            }
            alu = 0;
        }
    }
    return result;
}

void Z80Assembler::parseDataArray(int unitSize)
{
    ptr_ += 3; // skip "db " / "dw "
    while (ptr_ < src_.size()) {
        if (src_[ptr_] == '\n') {
            return;
        }
        int value = parseEquation();
        if (unitSize == 1) {
            emitByte(value);
        } else {
            emitWord(value);
        }
        if (ptr_ < src_.size() && src_[ptr_] == ',') {
            ptr_++;
        }
    }
}

bool Z80Assembler::parseDirective()
{
    if (compareSrc("db ")) {
        parseDataArray(1);
        return true;
    }
    if (compareSrc("dw ")) {
        parseDataArray(2);
        return true;
    }
    if (compareSrc("ds ")) {
        ptr_ += 3;
        int len = parseEquation();
        int value = 0;
        if (ptr_ < src_.size() && src_[ptr_] == ',') {
            ptr_++;
            value = parseEquation();
        }
        for (int i = 0; i < len; i++) {
            emitByte(value);
        }
        return true;
    }
    if (compareSrc("org ")) {
        ptr_ += 4;
        pc_ = parseEquation();
        return true;
    }
    if (compareSrc("align ")) {
        ptr_ += 6;
        int value = parseEquation();
        if (value > 0) {
            pc_ = (pc_ + value - 1) & ~(value - 1);
        }
        return true;
    }
    if (compareSrc("module ") || compareSrc("endmodule")) {
        return true; // no-ops
    }
    if (compareSrc("if ")) {
        ptr_ += 3;
        int value = parseEquation(); // always evaluate (pass-independent)
        if (value == 0) {            // skip to matching endif
            while (lineIdx_ < linesAll_) {
                lineIdx_++;
                ptr_ = lines_[lineIdx_].off;
                parseLabel();
                if (compareSrc("endif")) {
                    break;
                }
            }
        }
        return true;
    }
    if (compareSrc("endif")) {
        return true;
    }
    return false;
}

void Z80Assembler::parseLabel()
{
    if (src_[ptr_] != '@') {
        return;
    }
    lines_[lineIdx_].compile = true; // label lines recompile in pass 2
    ptr_++;

    int lt = src_[ptr_];
    if (lt >= '0' && lt <= '9') {
        err("label can't start with a number!");
        return;
    }
    std::string label = (lt != '.') ? "" : labelScope_;
    int value = pc_;

    while (ptr_ < src_.size()) {
        int c = src_[ptr_];
        if (c == '\n') {
            break;
        }
        ptr_++;
        if (c == ' ' || c == ':') {
            if (compareSrc("equ ")) {
                ptr_ += 4;
                value = parseEquation();
                break;
            }
            if (ptr_ < src_.size() && src_[ptr_] == ' ') {
                ptr_++;
            }
            break;
        }
        if (c == '=') {
            value = parseEquation();
            break;
        }
        if (labelCharAllowed(c)) {
            label += static_cast<char>(c);
        } else {
            err("wrong character in label name!");
            return;
        }
    }

    if (lt != '.') {
        labelScope_ = label; // global label sets the scope
    }

    if (pass_ == 1) {
        for (const auto& l : labelList_) {
            if (l.name == label) {
                err("label '" + label + "' already defined!");
                return;
            }
        }
        labelList_.push_back({label, value});
    }
}

int Z80Assembler::parseOpcodePart(int start, int count, int rowLen)
{
    int end = start + count;
    for (int opcode = start; opcode < end; opcode++) {
        int oplen = opLen_[opcode];
        const char* pat = Z80_OPCODES[opcode];
        int match;
        if (pat[0] != ':') {
            if (rowLen != oplen) {
                continue;
            }
            match = 0;
        } else {
            if (rowLen + 2 < oplen) {
                continue;
            }
            match = 1;
        }
        int paramOff = 0;
        ptr_ = ptrOld_;
        int j;
        for (j = match; j < oplen; j++) {
            char c = pat[j];
            if (c == '#' || c == '$' || c == '*' || c == '@') {
                paramValue_[paramOff++] = parseEquation();
                if (stop_) {
                    break;
                }
            } else {
                if (c != src_[ptr_++]) {
                    break;
                }
            }
            match++;
        }
        if (match == oplen) {
            return opcode;
        }
    }
    return -1;
}

void Z80Assembler::parseOpcode()
{
    size_t nl = src_.find('\n', ptr_);
    int rowLen = static_cast<int>((nl == std::string::npos ? src_.size() : nl) -
                                  ptr_);
    if (rowLen == 0) {
        return; // empty
    }
    ptrOld_ = ptr_;
    int c = static_cast<unsigned char>(src_[ptr_]);

    int opcode = -1;
    if (letterStartA_[c] >= 0) {
        opcode = parseOpcodePart(letterStartA_[c], letterLenA_[c], rowLen);
    }
    if (opcode < 0) {
        opcode = parseOpcodePart(letterStartB_[c], letterLenB_[c], rowLen);
    }
    if (opcode < 0) {
        err(std::string("unknown opcode or directive: '") +
            src_.substr(ptr_, rowLen) + "'");
        return;
    }

    const char* pat = Z80_OPCODES[opcode];
    int len = static_cast<int>(std::strlen(pat));
    int ptr = opLen_[opcode] + 1; // start of emit part (after '|')
    int paramOff = 0;
    while (ptr < len) {
        char c2 = pat[ptr++];
        if ((c2 >= '0' && c2 <= '9') || (c2 >= 'a' && c2 <= 'f')) {
            emit((hexChar(c2) << 4) | hexChar(pat[ptr++]));
            continue;
        }
        int value = paramValue_[paramOff++];
        switch (c2) {
        case '#': emitByte(value); break;
        case '@': emitWord(value); break;
        case '$':
            // '$' relative: large values are absolute addresses, convert to
            // an offset; small values are literal offsets.
            if (value > 127) {
                value -= pc_ + 1;
            }
            [[fallthrough]];
        case '*': emit(relativeOffset(value)); break;
        }
    }
}

void Z80Assembler::cleanup(const std::string& src)
{
    // Pass 0: lowercase, strip comments, collapse runs of spaces, mark a
    // column-0 token with '@' (label), keep only the op/operand and label/op
    // separating spaces, and index line offsets.
    origSource_ = src;
    src_.clear();
    lines_.clear();
    Line line;

    bool newline = true;
    bool label = false;
    size_t sptr = 0;
    int spaces = 0;
    int prevc = 255;
    size_t slen = src.size();

    while (sptr < slen) {
        int c = static_cast<unsigned char>(src[sptr++]);
        if (c == 0x09 || c == 0x0d) {
            c = 0x20;
        }
        if (c == 0x20 && c == prevc) {
            continue;
        }
        prevc = c;
        if (c >= 'A' && c <= 'Z') {
            c += 0x20;
        }
        if (c == ';') {
            size_t nl = src.find('\n', sptr);
            sptr = (nl == std::string::npos) ? slen : nl;
            continue;
        }
        if (newline && c > 0x20) {
            src_ += '@';
            label = true;
        }
        if (c == 0x20) {
            ++spaces;
        }
        size_t len = src_.size();
        if (c == '\n' && len != 0) {
            // strip trailing spaces
            size_t i = len;
            while (i > 0 && src_[i - 1] == 0x20) {
                i--;
            }
            src_.resize(i);
        }
        if (c != 0x20 || (label && spaces == 1) || spaces == 2) {
            src_ += static_cast<char>(c);
        }
        if (c == '\n') {
            lines_.push_back(line);
            line.off = src_.size();
            newline = true;
            label = false;
            spaces = 0;
        } else {
            newline = false;
        }
    }
    lines_.push_back(line);
    src_ += '\n';
}

int Z80Assembler::label(const std::string& name) const
{
    for (const auto& l : labelList_) {
        if (l.name == name) {
            return l.adr;
        }
    }
    return -1;
}

int Z80Assembler::assemble(const std::string& source, uint16_t org)
{
    buildTable();
    stop_ = false;
    errorMsg_.clear();
    std::fill(mem_.begin(), mem_.end(), 0);

    cleanup(source);
    linesAll_ = static_cast<int>(lines_.size());

    // Pass 1: emit with zeros for forward labels, recording label addresses.
    labelList_.clear();
    pass_ = 1;
    labelScope_.clear();
    org_ = org;
    pc_ = org;
    for (lineIdx_ = 0; lineIdx_ < linesAll_; lineIdx_++) {
        ptr_ = lines_[lineIdx_].off;
        lines_[lineIdx_].adr = pc_;
        parseLabel();
        if (!parseDirective()) {
            parseOpcode();
        }
        if (stop_) {
            return -1;
        }
    }
    int size = pc_ - org;

    // Pass 2: re-emit only the lines that referenced not-yet-known labels.
    pass_ = 2;
    labelScope_.clear();
    for (lineIdx_ = 0; lineIdx_ < linesAll_; lineIdx_++) {
        if (!lines_[lineIdx_].compile) {
            continue;
        }
        ptr_ = lines_[lineIdx_].off;
        pc_ = lines_[lineIdx_].adr;
        parseLabel();
        if (!parseDirective()) {
            parseOpcode();
        }
        if (stop_) {
            return -1;
        }
    }
    return size;
}

} // namespace musix::bbsong
