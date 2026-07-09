#include "YmStreamEncoderInterleaved.h"

#include "../../player/PsgRegistersConverter.h"
#include "../../player/SongPlayer.h"
#include "YmStreamEncoderNonInterleaved.h"

namespace arkostracker 
{

YmStreamEncoderInterleaved::YmStreamEncoderInterleaved(const std::shared_ptr<const Song>& pSong, const Id& pSubsongId, const int pPsgIndex, const bool pIsYm3,
                                                       const OptionalInt pTargetPsgFrequency) noexcept :
        YmStreamEncoder(pSong, pSubsongId, pPsgIndex, pIsYm3, pTargetPsgFrequency),
        ymStreamEncoderNonInterleaved(pSong, pSubsongId, pPsgIndex, pIsYm3, pTargetPsgFrequency)
{
}

void YmStreamEncoderInterleaved::generateStream(juce::OutputStream& outputStream, const int iterationCount, const Location& startLocation, const Location& pastEndLocation,
                                                const int digiChannel) noexcept
{
    // Generates a non-interleaved output locally.
    juce::MemoryOutputStream nonInterleavedOutputStream;
    ymStreamEncoderNonInterleaved.generateStream(nonInterleavedOutputStream, iterationCount, startLocation, pastEndLocation, digiChannel);
    const auto* rawData = static_cast<const unsigned char*>(nonInterleavedOutputStream.getData());

    const auto psgRegisterCount = isYm3 ? 14 : PsgRegisters::registerCount;

    // Creates a linear output, one full register after the other.
    for (auto registerIndex = 0; registerIndex < psgRegisterCount; ++registerIndex) {
        for (auto iterationIndex = 0; iterationIndex < iterationCount; ++iterationIndex) {
            const auto value = rawData[(iterationIndex * PsgRegisters::registerCount) + registerIndex];     // Always 16 registers in the non-interleaved.
            encodeByte(outputStream, value);
        }
    }
}

}   // namespace arkostracker
