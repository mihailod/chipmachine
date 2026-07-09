#include "YmStreamEncoderNonInterleaved.h"

#include "../../business/song/tool/frameCounter/FrameCounter.h"
#include "../../player/PsgRegistersConverter.h"
#include "../../player/SongPlayer.h"

namespace arkostracker 
{

YmStreamEncoderNonInterleaved::YmStreamEncoderNonInterleaved(std::shared_ptr<const Song> pSong, Id pSubsongId, const int pPsgIndex, const bool pIsYm3,
                                                             const OptionalInt pTargetPsgFrequency) noexcept :
        YmStreamEncoder(std::move(pSong), std::move(pSubsongId), pPsgIndex, pIsYm3, pTargetPsgFrequency)
{
}


// YmStreamEncoder method implementations.
// ==========================================

void YmStreamEncoderNonInterleaved::generateStream(juce::OutputStream& outputStream, const int iterationCount, const Location& startLocation, const Location& pastEndLocation,
                                                   const int digiChannel) noexcept
{
    auto previousHardwareEnvelope = 16;        // Sentinel value.

    // Initializes the player.
    SongPlayer songPlayer(song);
    songPlayer.play(startLocation, startLocation, pastEndLocation, true, true);
    const auto psgCount = song->getPsgCount(startLocation.getSubsongId());
    const auto sourcePsgFrequencyHz = song->getSubsongPsgs(startLocation.getSubsongId()).at(static_cast<size_t>(psgIndex)).getPsgFrequency();
    const auto localTargetPsgFrequencyHz = targetPsgFrequencyHz.isPresent() ? targetPsgFrequencyHz.getValue() : sourcePsgFrequencyHz;

    // Reads all the ticks and encodes them. One pass is required.
    for (auto iterationIndex = 0; iterationIndex < iterationCount; ++iterationIndex) {
        // Only one PSG is used, but all must be read.
        for (auto browsedPsgIndex = 0; browsedPsgIndex < psgCount; ++browsedPsgIndex) {
            auto [psgRegistersPtr, _] = songPlayer.getNextRegisters(browsedPsgIndex);
            // Only process the targeted PSG.
            if (browsedPsgIndex != psgIndex) {
                continue;
            }

            auto psgRegisters = PsgRegistersConverter::convertPeriods(*psgRegistersPtr, sourcePsgFrequencyHz, localTargetPsgFrequencyHz);

            // Modifies the PSG registers before encoding them, to have digidrums.
            fillPsgRegistersWithDigidrumData(psgRegisters, digiChannel);
            encodeRegisters(outputStream, psgRegisters, previousHardwareEnvelope);

            previousHardwareEnvelope = psgRegisters.getHardwareEnvelopeAndRetrig().getEnvelope();
        }
    }
}

}   // namespace arkostracker
