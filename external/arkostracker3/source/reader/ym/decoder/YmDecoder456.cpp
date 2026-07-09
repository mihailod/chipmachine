#include "YmDecoder456.h"

#include "../../../song/instrument/sample/Sample.h"

namespace arkostracker 
{

YmDecoder456::YmDecoder456(juce::InputStream& pInputStream) noexcept :
        YmDecoder(pInputStream),
        interleaved(false),
        iterationCount(-1),
        firstFrameDataIndex(-1)
{
}

bool YmDecoder456::acceptFormatHeader() noexcept
{
    return (checkTag("YM4!LeOnArD!") || checkTag("YM5!LeOnArD!") || checkTag("YM6!LeOnArD!"));
}

YmMetadata YmDecoder456::parseHeader(bool& success) noexcept
{
    // If not YM4, then YM5/6 (the validity of the file has been checked before, so it is safe).
    const auto ym5Or6 = !checkTag("YM4!LeOnArD!");

    YmMetadata ymMetadata;

    const auto totalLength = inputStream.getTotalLength();
    success = (totalLength > 100);         // Empirical... Just to make sure there is data.

    // Reads the frame count.
    success &= inputStream.setPosition(tagSize);
    iterationCount = readBigEndian4Bytes();
    if (!success || (iterationCount <= 0)) {
        return ymMetadata;
    }
    ymMetadata.iterationCount = iterationCount;

    // Reads the song attributes.
    const auto songAttributes = static_cast<unsigned int>(readBigEndian4Bytes());
    interleaved = ((songAttributes & 0b001U) != 0);
    const auto areSignedSamples = ((songAttributes & 0b010U) != 0);
    const auto are4BitsSamples = ((songAttributes & 0b100U) != 0);

    // Reads the digidrum count.
    const auto digidrumCount = inputStream.readShortBigEndian();

    // On YM5/6, some more data.
    if (ym5Or6) {
        ymMetadata.psgMasterClockHz = readBigEndian4Bytes();
        ymMetadata.playerReplayHz = inputStream.readShortBigEndian();
        ymMetadata.loopIterationIndex = readBigEndian4Bytes();
        inputStream.readShortBigEndian();           // Skips additional data.
    }

    success = readDigidrums(digidrumCount, ymMetadata, areSignedSamples, are4BitsSamples);
    if (!success) {
        return ymMetadata;
    }

    // Title, author, comments.
    // Warning! The doc is not clear. It is said the following data is only present in YM5+, but they
    // are still described in the YM4 format, so... I put them anyway. The YM Player sources simply don't support YM4 anymore, so...
    ymMetadata.title = readNTString();
    ymMetadata.author = readNTString();
    ymMetadata.comments = readNTString();

    // The first frame is just after.
    firstFrameDataIndex = static_cast<int>(inputStream.getPosition());

    success &= (firstFrameDataIndex < totalLength);       // Small security.

    return ymMetadata;
}

bool YmDecoder456::isInterleaved() const noexcept
{
    return interleaved;
}

int YmDecoder456::getEncodedRegisterCountInFrame() const noexcept
{
    return 16;
}

int YmDecoder456::getPostMusicFramesDataSize() const noexcept
{
    return 4;           // Marker "End!" at the end.
}

int YmDecoder456::getIterationCount() const noexcept
{
    return iterationCount;
}

int YmDecoder456::getFirstFrameDataIndex() const noexcept
{
    return firstFrameDataIndex;
}

bool YmDecoder456::readDigidrums(const int digidrumCount, YmMetadata& ymMetadata, const bool areSignedSamples, const bool are4BitsSamples) const noexcept
{
    std::vector<std::shared_ptr<Sample>> samples;
    auto success = true;

    // Reads all the digidrums.
    for (auto digidrumIndex = 0; success && (digidrumIndex < digidrumCount); ++digidrumIndex) {
        const auto digidrumSize = inputStream.readIntBigEndian();

        auto rawMemoryBlock = juce::MemoryBlock();
        const auto readSize = inputStream.readIntoMemoryBlock(rawMemoryBlock, digidrumSize);
        if (static_cast<size_t>(digidrumSize) != readSize) {
            jassertfalse;   // Unable to read all the digidrum!
            success = false;
        }

        // Treats the samples if necessary.
        if (areSignedSamples || are4BitsSamples) {
            for (auto i = 0U; i < readSize; ++i) {
                auto readValue = static_cast<unsigned char>(rawMemoryBlock[i]);
                if (are4BitsSamples) {
                    readValue = static_cast<unsigned char>(static_cast<unsigned int>(readValue) << 4U);
                } else /*if (areSignedSamples)*/ {
                    readValue = readValue - 128;
                }
                rawMemoryBlock[i] = static_cast<char>(readValue);
            }
        }

        samples.emplace_back(std::make_shared<Sample>(rawMemoryBlock));
    }

    ymMetadata.digidrums = samples;

    return success;
}

}   // namespace arkostracker
