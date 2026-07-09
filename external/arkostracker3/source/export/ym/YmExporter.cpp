#include "YmExporter.h"

#include "../../business/song/tool/frameCounter/FrameCounter.h"
#include "../../utils/StringUtil.h"
#include "YmStreamEncoderInterleaved.h"
#include "YmStreamEncoderNonInterleaved.h"

namespace arkostracker 
{

YmExporter::YmExporter(std::shared_ptr<Song> pSong, Id pSubsongId, const int pPsgIndex, const bool pInterleaved, const bool pIsYm3,
    const OptionalInt pTargetPsgFrequency) noexcept :
        song(std::move(pSong)),
        subsongId(std::move(pSubsongId)),
        psgIndex(pPsgIndex),
        interleaved(pIsYm3 ? true : pInterleaved),      // Security: prevents wrong choice.
        isYm3(pIsYm3),
        targetPsgFrequency(pTargetPsgFrequency),
        digiChannel(song->getDigiChannel(subsongId)),
        streamEncoder()
{
}


// Task method implementations.
// ==========================================

std::pair<bool, std::unique_ptr<juce::MemoryOutputStream>> YmExporter::performTask() noexcept
{
    auto psgFrequency = 0;
    auto replayFrequency = 0.0F;
    juce::String subsongTitle;
    song->performOnConstSubsong(subsongId, [&] (const Subsong& subsong) {
        const auto& psg = subsong.getPsgRefs().at(0U);
        psgFrequency = psg.getPsgFrequency();
        replayFrequency = subsong.getReplayFrequencyHz();
        subsongTitle = subsong.getName();
    });
    if (targetPsgFrequency.isPresent()) {
        psgFrequency = targetPsgFrequency.getValue();
    }

    // Counts how many iterations are in the Song.
    const auto[counter, upTo, loopCounter] = FrameCounter::count(*song, subsongId);
    const auto iterationCount = counter;
    jassert(iterationCount > 0);

    // Instantiates the YMEncoder according to the mode: interleaved or not.
    if (interleaved) {
        streamEncoder = std::make_unique<YmStreamEncoderInterleaved>(song, subsongId, psgIndex, isYm3, psgFrequency);
    } else {
        jassert(!isYm3);    // YM3 cannot handle non-interleaved. The UI should ignore this.
        streamEncoder = std::make_unique<YmStreamEncoderNonInterleaved>(song, subsongId, psgIndex, isYm3, psgFrequency);
    }

    // The music data is encoded after, but we do it now in a specific buffer, because the digidrum count and samples are determined
    // while generating it.
    // Gets the start/end locations.
    const auto startLocation = Location(subsongId, 0);
    const auto [loopStartLocation, pastEndLocation] = song->getLoopStartAndPastEndPositions(subsongId);
    const auto musicDataOutputStream = std::make_unique<juce::MemoryOutputStream>();

    streamEncoder->generateStream(*musicDataOutputStream, iterationCount, startLocation, pastEndLocation, digiChannel);

    auto success = !isCanceled();

    // Now encodes the header in the "real" buffer.
    auto outputStream = std::make_unique<juce::MemoryOutputStream>();
    // b1 = signed samples ? --> no. b2 = 4bits? --> no.
    const auto songAttributes = interleaved ? 1 : 0;
    // Digidrums?
    const auto digidrumCount = streamEncoder->getDigidrumCount();

    if (isYm3) {
        outputStream->writeText("YM3!", false, false, nullptr);
    } else {
        outputStream->writeText("YM6!", false, false, nullptr);
        outputStream->writeText("LeOnArD!", false, false, nullptr);
        outputStream->writeIntBigEndian(iterationCount);
        outputStream->writeIntBigEndian(songAttributes);
        outputStream->writeShortBigEndian(static_cast<int16_t>(digidrumCount));
        outputStream->writeIntBigEndian(psgFrequency);
        outputStream->writeShortBigEndian(static_cast<int16_t>(replayFrequency));
        outputStream->writeIntBigEndian(loopCounter);
        outputStream->writeShortBigEndian(0);                        // Padding.

        // Encodes the digidrums.
        streamEncoder->encodeDigidrums(*outputStream);

        // The song name. If the Subsong has a title, it is concatenated.
        auto title = song->getTitle();
        if (!subsongTitle.trim().isEmpty()) {
            title = title + " - " + subsongTitle;
        }
        StringUtil::writeNtString(*outputStream, title);
        StringUtil::writeNtString(*outputStream, song->getAuthor());
        StringUtil::writeNtString(*outputStream, song->getComments());
    }

    // Appends now the music data.
    musicDataOutputStream->flush();
    success = success && outputStream->write(musicDataOutputStream->getData(), musicDataOutputStream->getDataSize());
    musicDataOutputStream->reset();         // NOLINT(*-ambiguous-smartptr-reset-call)

    // Encodes the tail.
    if (!isYm3) {
        outputStream->writeText("End!", false, false, nullptr);
    }

    // Finish!
    outputStream->flush();

    streamEncoder.reset();

    return { success, std::move(outputStream) };
}

}   // namespace arkostracker
