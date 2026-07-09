#include "YmDecoder123.h"

namespace arkostracker 
{

YmDecoder123::YmDecoder123(juce::InputStream& pInputStream) noexcept :
        YmDecoder(pInputStream),
        iterationCount(-1),
        firstFrameDataIndex(-1)
{
}

bool YmDecoder123::acceptFormatHeader() noexcept
{
    return (checkTag("YM1!") || checkTag("YM2!") || checkTag("YM3!"));
}

YmMetadata YmDecoder123::parseHeader(bool& success) noexcept
{
    // There are no metadata in this format, so we use default ones.
    YmMetadata ymMetadata;

    // The iteration count is the size of the stream, minus the tag. No other way!
    const auto iterationCountDouble = (static_cast<int>(inputStream.getTotalLength()) - tagSize) / static_cast<double>(registerCount);
    iterationCount = static_cast<int>(iterationCountDouble);
    // Makes sure there are no extra data! There shouldn't!
    success = juce::exactlyEqual(static_cast<double>(iterationCount), iterationCountDouble);

    ymMetadata.iterationCount = iterationCount;

    // The data starts right after the tag.
    firstFrameDataIndex = tagSize;

    return ymMetadata;
}

bool YmDecoder123::isInterleaved() const noexcept
{
    return true;            // Contrary to what the documentation states!!
}

int YmDecoder123::getEncodedRegisterCountInFrame() const noexcept
{
    return 14;              // Only 14 registers encoded.
}

int YmDecoder123::getPostMusicFramesDataSize() const noexcept
{
    return 0;
}

int YmDecoder123::getIterationCount() const noexcept
{
    return iterationCount;
}

int YmDecoder123::getFirstFrameDataIndex() const noexcept
{
    return firstFrameDataIndex;
}


}   // namespace arkostracker

