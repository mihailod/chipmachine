#include "bbsong_parser.h"

#include <cstring>
#include <stdexcept>

namespace musix::bbsong {

namespace {

constexpr char MAGIC[] = "BBSONG"; // followed by a NUL (7 bytes total)
const std::string END_MARKER = ":END";
const std::string PATTERN_MARKER = "PatternName=";

// A forward cursor over the file bytes with the small primitives the format
// needs: NUL-terminated strings, little-endian u32, and "scan to a marker".
class Cursor
{
public:
    Cursor(const uint8_t* data, size_t size) : data_(data), size_(size) {}

    size_t pos() const { return pos_; }
    bool atEnd() const { return pos_ >= size_; }
    size_t remaining() const { return size_ - pos_; }

    // Reads a NUL-terminated string and advances past the NUL.
    std::string readCStr()
    {
        size_t start = pos_;
        while (pos_ < size_ && data_[pos_] != 0) {
            pos_++;
        }
        if (pos_ >= size_) {
            throw std::runtime_error("bbsong: unterminated string");
        }
        std::string s(reinterpret_cast<const char*>(data_ + start), pos_ - start);
        pos_++; // skip NUL
        return s;
    }

    uint32_t readU32()
    {
        if (pos_ + 4 > size_) {
            throw std::runtime_error("bbsong: truncated u32");
        }
        uint32_t v = static_cast<uint32_t>(data_[pos_]) |
                     (static_cast<uint32_t>(data_[pos_ + 1]) << 8) |
                     (static_cast<uint32_t>(data_[pos_ + 2]) << 16) |
                     (static_cast<uint32_t>(data_[pos_ + 3]) << 24);
        pos_ += 4;
        return v;
    }

    // Index of the next occurrence of `marker` at or after the current
    // position, or size_ if not found.
    size_t find(const std::string& marker) const
    {
        if (marker.empty() || marker.size() > remaining()) {
            return size_;
        }
        for (size_t i = pos_; i + marker.size() <= size_; i++) {
            if (memcmp(data_ + i, marker.data(), marker.size()) == 0) {
                return i;
            }
        }
        return size_;
    }

    std::vector<uint8_t> readBytes(size_t n)
    {
        if (pos_ + n > size_) {
            throw std::runtime_error("bbsong: truncated chunk payload");
        }
        std::vector<uint8_t> out(data_ + pos_, data_ + pos_ + n);
        pos_ += n;
        return out;
    }

    void seek(size_t p) { pos_ = p; }
    const uint8_t* ptr() const { return data_ + pos_; }

private:
    const uint8_t* data_;
    size_t size_;
    size_t pos_ = 0;
};

// Splits a payload into its leading NUL-terminated "Key=Value" properties,
// returning a map and the byte offset at which the first non-property byte
// begins (used by :LAYOUT to locate the raw order list that follows).
std::map<std::string, std::string> parseProps(const std::vector<uint8_t>& p,
                                               size_t* binaryStart)
{
    std::map<std::string, std::string> props;
    size_t i = 0;
    while (i < p.size()) {
        // A property is a NUL-terminated string containing '=' whose key is
        // alphabetic. Anything else marks the start of binary data.
        size_t nul = i;
        while (nul < p.size() && p[nul] != 0) {
            nul++;
        }
        if (nul >= p.size()) {
            break; // no terminator -> not a property
        }
        std::string s(reinterpret_cast<const char*>(p.data() + i), nul - i);
        auto eq = s.find('=');
        bool looksLikeProp = eq != std::string::npos && eq > 0 &&
                             std::isalpha(static_cast<unsigned char>(s[0]));
        if (!looksLikeProp) {
            break;
        }
        props[s.substr(0, eq)] = s.substr(eq + 1);
        i = nul + 1;
    }
    if (binaryStart != nullptr) {
        *binaryStart = i;
    }
    return props;
}

uint32_t toU32(const std::string& s, uint32_t def = 0)
{
    try {
        return static_cast<uint32_t>(std::stoul(s));
    } catch (...) {
        return def;
    }
}

void parsePatternData(const std::vector<uint8_t>& payload, Song& song)
{
    Cursor c(payload.data(), payload.size());
    // Leading PatternCount= property.
    if (!c.atEnd()) {
        std::string first = c.readCStr();
        auto eq = first.find('=');
        if (eq != std::string::npos && first.substr(0, eq) == "PatternCount") {
            song.patternCount = toU32(first.substr(eq + 1));
        } else {
            c.seek(0); // not the expected prop; treat all as patterns
        }
    }

    while (!c.atEnd()) {
        // Each pattern starts with "PatternName=<name>".
        if (c.find(PATTERN_MARKER) != c.pos()) {
            break; // no more patterns
        }
        std::string nameProp = c.readCStr(); // "PatternName=..."
        Pattern pat;
        auto eq = nameProp.find('=');
        pat.name = (eq == std::string::npos) ? "" : nameProp.substr(eq + 1);
        pat.length = c.readU32();
        pat.tempo = c.readU32();
        // Channel data runs until the next pattern header or end of payload.
        size_t next = c.find(PATTERN_MARKER);
        pat.channelData = c.readBytes(next - c.pos());
        song.patterns.push_back(std::move(pat));
    }
}

} // namespace

const std::vector<uint8_t>* Song::chunk(const std::string& type) const
{
    for (const auto& ch : chunks) {
        if (ch.type == type) {
            return &ch.payload;
        }
    }
    return nullptr;
}

Song parse(const uint8_t* data, size_t size)
{
    Cursor c(data, size);

    // Header: "BBSONG\0" then a version string ("0001").
    std::string magic = c.readCStr();
    if (magic != MAGIC) {
        throw std::runtime_error("bbsong: bad magic");
    }
    Song song;
    song.version = c.readCStr();

    // Chunks until the bytes run out.
    while (!c.atEnd()) {
        std::string type = c.readCStr(); // e.g. ":INFO"
        if (type.empty() || type[0] != ':') {
            throw std::runtime_error("bbsong: expected chunk type, got '" +
                                     type + "'");
        }
        // Payload is everything up to the ":END" marker.
        size_t endPos = c.find(END_MARKER);
        if (endPos == size) {
            throw std::runtime_error("bbsong: chunk '" + type +
                                     "' missing :END");
        }
        Chunk chunk;
        chunk.type = type;
        chunk.payload = c.readBytes(endPos - c.pos());
        c.seek(endPos);
        std::string end = c.readCStr(); // consume ":END"
        (void)end;

        if (type == ":INFO") {
            song.info = parseProps(chunk.payload, nullptr);
            song.engine = song.info.count("Engine") ? song.info["Engine"] : "";
            song.title = song.info.count("Title") ? song.info["Title"] : "";
            song.author = song.info.count("Author") ? song.info["Author"] : "";
        } else if (type == ":LAYOUT") {
            size_t binStart = 0;
            auto props = parseProps(chunk.payload, &binStart);
            song.loopStart = toU32(props.count("LoopStart") ? props["LoopStart"]
                                                            : "0");
            song.length = toU32(props.count("Length") ? props["Length"] : "0");
            // The order list is the trailing `length` bytes of the payload.
            if (song.length > 0 && song.length <= chunk.payload.size()) {
                song.order.assign(chunk.payload.end() - song.length,
                                   chunk.payload.end());
            } else {
                song.order.assign(chunk.payload.begin() + binStart,
                                   chunk.payload.end());
            }
        } else if (type == ":PATTERNDATA") {
            parsePatternData(chunk.payload, song);
        }

        song.chunks.push_back(std::move(chunk));
    }

    return song;
}

} // namespace musix::bbsong
