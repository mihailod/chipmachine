#include "YmDecoder.h"

#include "../../../utils/StreamUtil.h"

namespace arkostracker 
{

YmDecoder::YmDecoder(juce::InputStream& pInputStream) noexcept :
        inputStream(pInputStream)
{
}

bool YmDecoder::checkTag(const juce::String& tag) const noexcept
{
    const auto tagSize = tag.length();
    if (inputStream.getTotalLength() <= tagSize) {
        return false;
    }

    // Checks the tag.
    auto success = inputStream.setPosition(0);
    success = success && StreamUtil::readAndCompareString(inputStream, tag);

    return success;
}

unsigned char YmDecoder::getValue(const int iterationIndex, const int reg, bool& successOut) const noexcept
{
    const auto iterationCount = getIterationCount();
    const auto firstFrameDataIndex = getFirstFrameDataIndex();

    jassert(firstFrameDataIndex > 0);
    jassert(iterationCount > 0);

    juce::int64 position; // NOLINT(*-init-variables)
    if (isInterleaved()) {
        // Interleaved: registers are stored by registers: all R0 first, all R1 first, etc.
        position = firstFrameDataIndex + iterationIndex + reg * iterationCount;
    } else {
        // Non-interleaved: registers are stored linearly, frame by frame.
        position = firstFrameDataIndex + getEncodedRegisterCountInFrame() * iterationIndex + reg;
    }
    successOut = (position < inputStream.getTotalLength() - getPostMusicFramesDataSize());     // Takes in account the possible "End!" marker at the end.
    successOut &= inputStream.setPosition(position);

    // Reads and stores the data.
    return static_cast<unsigned char>(inputStream.readByte());
}

int YmDecoder::readBigEndian4Bytes() const noexcept
{
    return (static_cast<unsigned char>(inputStream.readByte()) << 24U) + (static_cast<unsigned char>(inputStream.readByte()) << 16U)
           + (static_cast<unsigned char>(inputStream.readByte()) << 8U) + static_cast<unsigned char>(inputStream.readByte());
}

int YmDecoder::readLittleEndian4Bytes() const noexcept
{
    return static_cast<unsigned char>(inputStream.readByte()) + (static_cast<unsigned char>(inputStream.readByte()) << 8U)
           + (static_cast<unsigned char>(inputStream.readByte()) << 16U) + (static_cast<unsigned char>(inputStream.readByte()) << 24U);

}

juce::String YmDecoder::readNTString() const noexcept
{
    juce::String string;

    auto c = inputStream.readByte();
    while (c != 0) {
        string += c;                // Not efficient... Who cares?

        c = inputStream.readByte();
    }

    return string;
}

}   // namespace arkostracker
