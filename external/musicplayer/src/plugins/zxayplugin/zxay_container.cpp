// The ZXAY container.
//
// Two of this plugin's formats arrive wrapped in it rather than raw: AY
// Amadeus (".amad", type tag "AMAD") and Sound Tracker 1.1 (".st11", type tag
// "ST11"). The container is unusual enough to be worth one implementation
// instead of two:
//
//   * everything is BIG-endian, on a format for a little-endian CPU;
//   * every pointer is a SIGNED 16-bit offset relative to ITS OWN position in
//     the file, not to the start;
//   * the song table's pointers are relative to the position AFTER the 4-byte
//     entry has been read, which is where the off-by-a-few mistakes live.
//
// Layout: "ZXAY", a 4-byte type tag, file/player version bytes, then pointers
// to the special player, the author string and a misc string, the song count,
// the first song index, and a pointer to the song table. Each song table entry
// is two pointers: name, then data.

#include "zxay_native.h"

#include <cstring>

namespace musix::zxay {

namespace {

uint16_t be16(const uint8_t* p)
{
    return static_cast<uint16_t>((p[0] << 8) | p[1]);
}

} // namespace

bool parseZxayContainer(const std::vector<uint8_t>& d, const char* typeTag,
                        ZxayEntry* out)
{
    if (d.size() < 24 || std::memcmp(d.data(), "ZXAY", 4) != 0 ||
        std::memcmp(d.data() + 4, typeTag, 4) != 0) {
        return false;
    }
    // A pointer stored at `at`, resolved against `at` itself.
    auto rel = [&](size_t at) -> long {
        return static_cast<long>(at) +
               static_cast<int16_t>(be16(d.data() + at));
    };

    const long songs = rel(18);
    if (songs < 0 || songs + 4 > static_cast<long>(d.size())) {
        return false;
    }
    // AY_Emul reads the 4-byte entry and only then resolves, so both pointers
    // are relative to the position past it.
    const long cursor = songs + 4;
    out->author = rel(12);
    out->songName = cursor - 4 + static_cast<int16_t>(be16(&d[songs]));
    out->songData = cursor - 2 + static_cast<int16_t>(be16(&d[songs + 2]));
    return out->songData >= 0 && out->songData < static_cast<long>(d.size());
}

std::string zxayString(const std::vector<uint8_t>& d, long at)
{
    std::string s;
    while (at >= 0 && at < static_cast<long>(d.size()) && d[at] != 0) {
        s.push_back(static_cast<char>(d[at++]));
    }
    return s;
}

} // namespace musix::zxay
