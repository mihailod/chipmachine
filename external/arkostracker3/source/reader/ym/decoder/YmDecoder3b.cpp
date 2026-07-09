#include "YmDecoder3b.h"

namespace arkostracker 
{

YmDecoder3b::YmDecoder3b(juce::InputStream& pInputStream) noexcept :
        YmDecoder123(pInputStream),
        iterationCount(-1),
        firstFrameDataIndex(-1)
{
}

bool YmDecoder3b::acceptFormatHeader() noexcept
{
    return checkTag("YM3b");
}

YmMetadata YmDecoder3b::parseHeader(bool& success) noexcept
{
    const auto size = static_cast<int>(inputStream.getTotalLength());

    // There are no metadata in this format, so we use default ones.
    YmMetadata ymMetadata;

    // The iteration count is the size of the stream, minus the tag, minus the 4 bytes of the loop index at the end.
    const auto iterationCountDouble = (static_cast<int>(inputStream.getTotalLength()) - tagSize - 4) / static_cast<double>(registerCount);
    iterationCount = static_cast<int>(iterationCountDouble);
    // Makes sure there are no extra data! There shouldn't!
    success = juce::exactlyEqual(static_cast<double>(iterationCount), iterationCountDouble);

    ymMetadata.iterationCount = iterationCount;
    // The data is just after the tag.
    firstFrameDataIndex = tagSize;

    // Reads the last 4 bytes (little-endian!) for the loop index.
    success &= inputStream.setPosition(size - 4);
    const auto loopIterationIndex = readLittleEndian4Bytes();
    ymMetadata.loopIterationIndex = loopIterationIndex;

    // Non-interleaved is default, no need to set it (v0r0, v0r1, v0r2, etc.).
    return ymMetadata;
}

int YmDecoder3b::getFirstFrameDataIndex() const noexcept
{
    return firstFrameDataIndex;
}

int YmDecoder3b::getIterationCount() const noexcept
{
    return iterationCount;
}

}   // namespace arkostracker
