#include "IceDepack.h"

#include "atariaudio/external/ice_24.h"

namespace musix::sndh {

namespace {
// ice_24_header reads a 4-byte magic; origsize/packedsize read the 12-byte
// header that follows. Never hand it a buffer shorter than that.
constexpr size_t kMinIceHeader = 12;
} // namespace

bool isIcePacked(const uint8_t* data, size_t size)
{
    if (data == nullptr || size < kMinIceHeader) { return false; }
    return ice_24_header(const_cast<unsigned char*>(data)) != 0;
}

std::vector<uint8_t> iceDepack(const uint8_t* data, size_t size)
{
    if (!isIcePacked(data, size)) { return {}; }

    long const origSize = ice_24_origsize(const_cast<unsigned char*>(data));
    if (origSize <= 0) { return {}; }

    std::vector<uint8_t> out(static_cast<size_t>(origSize));
    long const got =
        ice_24_depack(const_cast<unsigned char*>(data), out.data());
    // A short or failed depack (-1) means the stream was truncated or is not
    // really ICE! 2.4; report nothing rather than a half-filled buffer.
    if (got != origSize) { return {}; }
    return out;
}

} // namespace musix::sndh
