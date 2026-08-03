// GSF container loader, written from the published PSF/GSF specification
// (Neill Corlett's psf_format.txt and the GSF addendum). No code is taken from
// any GPL player -- in particular NOT from the VisualBoyAdvance-based playgsf
// that this plugin used before 2026-08-01. Keep it that way: the whole point of
// the mGBA swap is that gsfplugin ships in the App Store build.
//
// Layout of a PSF file:
//   0x00  "PSF"
//   0x03  version byte (0x22 = GSF)
//   0x04  u32 reserved-area size R
//   0x08  u32 compressed program size N
//   0x0C  u32 CRC-32 of the compressed program
//   0x10  reserved area, R bytes
//         compressed program, N bytes, zlib "deflate"
//         optional "[TAG]" followed by "name=value\n" lines
//
// The decompressed GSF program section is:
//   0x00  u32 entry point
//   0x04  u32 load address (0x08000000-based, i.e. the cartridge)
//   0x08  u32 length of the data that follows
//   0x0C  data
//
// A ".minigsf" carries only the few bytes that differ between tracks and names
// a shared ".gsflib" in its "_lib" tag. Per the spec the chain is applied
// depth-first: this file's "_lib" (and recursively ITS libs) go down first, then
// this file's own program overlays them, then "_lib2", "_lib3", ... in order.
// Getting that order wrong silently plays the wrong track, so it is spelled out
// in applyFile() below rather than folded into one loop.

#include "GsfRom.h"

#include <coreutils/log.h>

#include <zlib.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace musix::gsf {

namespace {

// The GBA cartridge window is 0x08000000..0x09FFFFFF, so a load address folds
// to an image offset with this mask.
constexpr uint32_t kCartMask = 0x01FFFFFFu;
constexpr size_t kRomMax = 32 * 1024 * 1024;
// A "multiboot" rip is linked for EWRAM (0x02000000) instead of the cartridge:
// the tune was ripped from a game that copies its driver into RAM and runs it
// from there. mGBA has a separate load path for those.
constexpr uint32_t kEwramBase = 0x02000000;
constexpr size_t kEwramSize = 256 * 1024;
// The smallest real GBA mask ROM is 4 Mbit. See the note on kRomFloor's use.
constexpr size_t kCartFloor = 512 * 1024;
// The spec allows arbitrarily deep _lib nesting; every real rip is 1 deep.
// A cap is what keeps a maliciously self-referencing pair from recursing away.
constexpr int kMaxDepth = 10;

uint32_t rd32(const uint8_t* p)
{
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

std::vector<uint8_t> readFile(const std::string& path)
{
    std::ifstream f(path, std::ios::in | std::ios::binary);
    if (!f) { return {}; }
    f.seekg(0, std::ios::end);
    auto end = f.tellg();
    if (end <= 0) { return {}; }
    // A PSF that big is corrupt, and we would rather not allocate it.
    if (static_cast<uint64_t>(end) > kRomMax * 2) { return {}; }
    f.seekg(0, std::ios::beg);
    std::vector<uint8_t> data(static_cast<size_t>(end));
    f.read(reinterpret_cast<char*>(data.data()), end);
    if (!f) { return {}; }
    return data;
}

// zlib inflate with an unknown output size. The header does not record the
// decompressed length, so grow rather than guess; bail past kRomMax + header.
std::vector<uint8_t> inflateAll(const uint8_t* src, size_t srcLen)
{
    std::vector<uint8_t> out;
    if (srcLen == 0) { return out; }

    z_stream zs{};
    if (inflateInit(&zs) != Z_OK) { return out; }
    zs.next_in = const_cast<Bytef*>(src);
    zs.avail_in = static_cast<uInt>(srcLen);

    std::vector<uint8_t> chunk(256 * 1024);
    int rc = Z_OK;
    while (rc != Z_STREAM_END) {
        zs.next_out = chunk.data();
        zs.avail_out = static_cast<uInt>(chunk.size());
        rc = inflate(&zs, Z_NO_FLUSH);
        if (rc != Z_OK && rc != Z_STREAM_END && rc != Z_BUF_ERROR) {
            out.clear();
            break;
        }
        size_t got = chunk.size() - zs.avail_out;
        out.insert(out.end(), chunk.begin(),
                   chunk.begin() + static_cast<ptrdiff_t>(got));
        if (out.size() > kRomMax + 16) {
            out.clear();
            break;
        }
        if (got == 0 && rc != Z_STREAM_END) { break; } // truncated input
    }
    inflateEnd(&zs);
    return out;
}

std::string dirOf(const std::string& path)
{
    auto slash = path.find_last_of("/\\");
    return slash == std::string::npos ? std::string{}
                                      : path.substr(0, slash + 1);
}

bool iEquals(const std::string& a, const std::string& b)
{
    if (a.size() != b.size()) { return false; }
    for (size_t i = 0; i < a.size(); i++) {
        if (tolower(static_cast<unsigned char>(a[i])) !=
            tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

// Tag variable lookup. Names are case-insensitive per the spec; values keep
// their inner spacing but lose trailing CR/space, because the value is used as
// a file name.
std::string findTag(const std::string& tags, const std::string& key)
{
    size_t pos = 0;
    while (pos < tags.size()) {
        size_t eol = tags.find('\n', pos);
        if (eol == std::string::npos) { eol = tags.size(); }
        size_t eq = tags.find('=', pos);
        if (eq != std::string::npos && eq < eol) {
            if (iEquals(tags.substr(pos, eq - pos), key)) {
                std::string v = tags.substr(eq + 1, eol - eq - 1);
                while (!v.empty() && (v.back() == '\r' || v.back() == ' ')) {
                    v.pop_back();
                }
                return v;
            }
        }
        pos = eol + 1;
    }
    return {};
}

struct Loader
{
    std::vector<uint8_t> rom;
    size_t high = 0;
    uint32_t entryPoint = 0;
    bool anyData = false;
    bool multiboot = false;

    // Overlay one decompressed program section onto the cartridge image.
    void applyProgram(const std::vector<uint8_t>& exe)
    {
        if (exe.size() < 12) { return; }
        uint32_t entry = rd32(exe.data());
        uint32_t rawLoad = rd32(exe.data() + 4);
        // Load addresses are written as absolute GBA addresses; masking folds
        // 0x08xxxxxx (cartridge) to an image offset, and also copes with a rip
        // that stored a bare offset. EWRAM loads fold the same way -- the image
        // is still built from offset 0, it just gets handed to a different mGBA
        // entry point later.
        uint32_t load = rawLoad & kCartMask;
        uint32_t size = rd32(exe.data() + 8);

        if ((rawLoad & 0x0F000000u) == kEwramBase) { multiboot = true; }

        if (size > exe.size() - 12) {
            size = static_cast<uint32_t>(exe.size() - 12);
        }
        if (load > kRomMax || size > kRomMax - load) { return; }

        size_t end = static_cast<size_t>(load) + size;
        if (end > rom.size()) { rom.resize(end, 0); }
        std::memcpy(rom.data() + load, exe.data() + 12, size);
        high = std::max(high, end);
        // Last writer wins: the outermost file is applied last, so its entry
        // point is the one that counts.
        entryPoint = entry;
        anyData = true;
    }

    bool applyFile(const std::string& path, int depth)
    {
        if (depth > kMaxDepth) { return false; }
        auto buf = readFile(path);
        if (buf.size() < 16 || std::memcmp(buf.data(), "PSF", 3) != 0) {
            return false;
        }

        uint32_t reservedSize = rd32(buf.data() + 4);
        uint32_t programSize = rd32(buf.data() + 8);
        // Both sizes are attacker-controlled; check them against the real file
        // before using them as offsets.
        if (static_cast<uint64_t>(16) + reservedSize + programSize >
            buf.size()) {
            return false;
        }

        std::string tags;
        size_t tagOff = size_t{16} + reservedSize + programSize;
        if (tagOff + 5 <= buf.size() &&
            std::memcmp(buf.data() + tagOff, "[TAG]", 5) == 0) {
            tags.assign(reinterpret_cast<const char*>(buf.data() + tagOff + 5),
                        buf.size() - tagOff - 5);
        }

        const std::string dir = dirOf(path);

        // 1. "_lib" and everything it pulls in, deepest first.
        std::string lib = findTag(tags, "_lib");
        if (!lib.empty()) { applyFile(dir + lib, depth + 1); }

        // 2. this file's own program, overlaying the libs.
        if (programSize != 0) {
            auto exe = inflateAll(buf.data() + 16 + reservedSize, programSize);
            applyProgram(exe);
        }

        // 3. "_lib2".."_libN", in order, on top. Numbering starts at 2 and the
        // spec says to stop at the first gap.
        for (int i = 2;; i++) {
            std::string more = findTag(tags, "_lib" + std::to_string(i));
            if (more.empty()) { break; }
            applyFile(dir + more, depth + 1);
        }
        return true;
    }
};

// Real cartridges are a power of two, and mGBA derives its ROM address mask
// with toPow2(romSize). Rounding up (zero-filled) keeps reads just past the
// rip's own data reading as 0 rather than falling into mGBA's out-of-bounds
// path, which is what the old VBA core did with its flat buffer.
size_t roundUpPow2(size_t n, size_t floor)
{
    size_t p = floor;
    while (p < n && p < kRomMax) {
        p <<= 1;
    }
    return p;
}

} // namespace

RomImage loadRom(const std::string& path)
{
    RomImage image;
    Loader loader;
    if (!loader.applyFile(path, 0) || !loader.anyData || loader.high == 0) {
        return image;
    }

    // The final size is not cosmetic -- it is what selects mGBA's load path.
    //
    // mCore::loadROM dispatches on GBAIsMB(), a HEURISTIC: "no bigger than
    // EWRAM, and the word at the multiboot magic offset decodes to a branch
    // into EWRAM". Measured against a 86-file corpus on 2026-08-01, that
    // misfires on ordinary cartridge rips that happen to be small -- Radar
    // Mission (128 KB) and the GHX player samples (128 KB) were both being run
    // out of EWRAM, which played but rendered at 2.3x and 0.47x the level the
    // old VBA core produced.
    //
    // The GSF program header states which one it is, so use that instead of
    // letting mGBA guess, and express the answer in the only currency
    // GBAIsMB() reads: size.
    //   cartridge -> at least 512 KB (the smallest real GBA mask ROM, and
    //                comfortably over EWRAM) so GBAIsMB can never fire;
    //   multiboot -> never padded past EWRAM, so it always does.
    if (loader.multiboot) {
        size_t size = roundUpPow2(loader.high, 0x1000);
        if (size > kEwramSize) { size = kEwramSize; }
        loader.rom.resize(size, 0);
    } else {
        loader.rom.resize(roundUpPow2(loader.high, kCartFloor), 0);
    }

    image.data = std::move(loader.rom);
    image.entryPoint = loader.entryPoint;
    image.multiboot = loader.multiboot;
    image.valid = true;
    return image;
}

} // namespace musix::gsf
