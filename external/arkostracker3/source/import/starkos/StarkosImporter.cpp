#include "StarkosImporter.h"

#include "../../song/subsong/SubsongConstants.h"
#include "../../utils/MemoryBlockUtil.h"
#include "InstrumentReader.h"
#include "TrackReader.h"
#include "../common/AmsdosHeaderUtil.h"

namespace arkostracker 
{

const juce::String StarkosImporter::headerTag = "STK1.0SONG";               // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

StarkosImporter::StarkosImporter() noexcept :
        songBuilder()
{
}


// SongImporter method implementations.
// =======================================

bool StarkosImporter::doesFormatMatch(juce::InputStream& inputStream, const juce::String& /*extension*/) const noexcept
{
    // Enough data?
    if (inputStream.getTotalLength() < songMinimumSize) {
        return false;
    }

    // Checks the header. Removes the possible Amsdos header.
    const auto rawMusicData = MemoryBlockUtil::fromInputStream(inputStream);
    const auto musicData = AmsdosHeaderUtil::removeHeaderIfNeeded(rawMusicData);
    bool success; // NOLINT(*-init-variables)
    const auto readTag = MemoryBlockUtil::extractString(musicData, 0, headerTagSize, success);

    return success && (readTag == headerTag);
}

std::unique_ptr<SongImporter::Result> StarkosImporter::loadSong(juce::InputStream& inputStream, const ImportConfiguration& /*configuration*/) noexcept
{
    songBuilder = std::make_unique<SongBuilder>();

    // Parses the Song. Removes the possible Amsdos header.
    const auto rawMusicData = MemoryBlockUtil::fromInputStream(inputStream);
    const auto musicData = AmsdosHeaderUtil::removeHeaderIfNeeded(rawMusicData);
    parse(musicData);

    // Gets the result from the Builder and returns it.
    auto songAndReport = songBuilder->buildSong();

    return std::make_unique<SongImporter::Result>(std::move(songAndReport.first), std::move(songAndReport.second));
}


// =======================================

void StarkosImporter::parse(const juce::MemoryBlock& musicData) const noexcept
{
    // Extracts some data first.
    auto success = true;
    const auto author = MemoryBlockUtil::extractString(musicData, offsetAuthor, sizeAuthor, success);
    const auto comments = MemoryBlockUtil::extractString(musicData, offsetComments, sizeComments, success);
    juce::int64 metadataOffset = offsetDigichannel;
    const auto digiChannel = MemoryBlockUtil::extractUnsignedChar(musicData, metadataOffset++, success) - 1;         // SKS: 1-3. With AT, from 0 to 2.
    const auto endIndex = MemoryBlockUtil::extractUnsignedChar(musicData, metadataOffset++, success);
    const auto startLoopIndex = MemoryBlockUtil::extractUnsignedChar(musicData, metadataOffset++, success);
    ++metadataOffset;       // Transposition ignored.
    const auto initialSpeed = MemoryBlockUtil::extractUnsignedChar(musicData, metadataOffset++, success);
    auto playerFrequencyIndex = MemoryBlockUtil::extractUnsignedChar(musicData, metadataOffset++, success);
    if (!success) {         // Only the last one is taken in account, but if the previous didn't work, this one shouldn't!
        songBuilder->addError("Unable to read the data of the header.");
        return;
    }
    if (playerFrequencyIndex > 5) {
        songBuilder->addWarning("Invalid player frequency index: " + juce::String(playerFrequencyIndex));
        playerFrequencyIndex = 2;
    }
    const auto playerFrequencyHz = convertPlayerFrequencyIndexToHz(playerFrequencyIndex);

    // Sets the Song metadata.
    songBuilder->setSongMetaData("Untitled", author, juce::String(), juce::translate("Converted with Arkos Tracker."));

    // Builds the subsong.
    auto& subsongBuilder = songBuilder->startNewSubsong(0);

    subsongBuilder.setMetadata(SubsongConstants::defaultFirstSubsongName, initialSpeed, playerFrequencyHz, digiChannel);
    subsongBuilder.addPsg(Psg());
    subsongBuilder.setLoop(startLoopIndex, endIndex);

    // Reads the data proper. In THAT order.
    juce::int64 readOffset = offsetLinker;
    readLinker(subsongBuilder, musicData, readOffset);
    readInstruments(musicData, readOffset);
    readSpecialTracks(subsongBuilder, musicData, readOffset);
    readTracks(subsongBuilder, musicData, readOffset);

    songBuilder->finishCurrentSubsong();
}

float StarkosImporter::convertPlayerFrequencyIndexToHz(const int playerFrequencyIndex) noexcept
{
    switch (playerFrequencyIndex) {
        case 0:
            return 12.5F;
        case 1:
            return 25;
        default:
            jassertfalse;         // Unknown frequency index. Shouldn't happen.
        case 2:
            return 50;
        case 3:
            return 100;
        case 4:
            return 150;
        case 5:
            return 300;
    }
}

void StarkosImporter::readLinker(SubsongBuilder& subsongBuilder, const juce::MemoryBlock& musicData, juce::int64& readOffset) const noexcept
{
    bool success;    // NOLINT(*-init-variables)
    const auto lastPositionIndex = MemoryBlockUtil::extractUnsignedChar(musicData, readOffset++, success);
    ++readOffset;           // Skips useless byte.

    // Reads the linker items.
    for (auto positionIndex = 0; positionIndex <= lastPositionIndex; ++positionIndex) {
        std::vector<int> trackIndexes;
        std::vector<int> transpositions;

        // Extracts the 3 Track indexes and transpositions.
        constexpr auto channelCount = 3;
        for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
            const auto byte1 = MemoryBlockUtil::extractUnsignedChar(musicData, readOffset++, success);
            const auto byte2 = MemoryBlockUtil::extractUnsignedChar(musicData, readOffset++, success);

            auto trackNumber = (byte1 | ((byte2 & 1) << 8));
            auto transposition = static_cast<signed char>(byte2) >> 1;

            trackIndexes.emplace_back(trackNumber);
            transpositions.emplace_back(transposition);
        }

        const auto patternLastCellIndex = MemoryBlockUtil::extractUnsignedChar(musicData, readOffset++, success);
        const auto specialTrackIndex = MemoryBlockUtil::extractUnsignedChar(musicData, readOffset++, success);

        if (patternLastCellIndex > 127) {
            songBuilder->addError("Invalid patternHeight (" + juce::String(patternLastCellIndex + 1) + ") for position " + juce::String(positionIndex));
            return;
        }

        // Creates a Pattern object. It is only to know if it is already known. It is NOT encoded.
        // The speed/event Track is fake! What matters is that the Pattern is the same in the source!
        const Pattern pattern(trackIndexes, specialTrackIndex, specialTrackIndex);
        const auto isNewAndPatternIndex = subsongBuilder.checkRawOriginalPatternAndStoreIfDifferent(pattern);
        const auto patternIndex = isNewAndPatternIndex.second;
        if (isNewAndPatternIndex.first) {
            // New pattern. Encodes it. The Tracks indexes can be used directly.
            // The Speed and Event Tracks are generated with the same indexes (for each Special Tracks, there is a Speed, AND an Event Track).
            subsongBuilder.startNewPattern(patternIndex);
            for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
                subsongBuilder.setCurrentPatternTrackIndex(channelIndex, pattern.getCurrentTrackIndex(channelIndex));
                subsongBuilder.setCurrentPatternSpeedTrackIndex(specialTrackIndex);
                subsongBuilder.setCurrentPatternEventTrackIndex(specialTrackIndex);
            }
            subsongBuilder.finishCurrentPattern();
        }

        // Encodes the Position.
        subsongBuilder.startNewPosition();
        subsongBuilder.setCurrentPositionPatternIndex(patternIndex);
        for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
            const auto transposition = transpositions.at(static_cast<size_t>(channelIndex));
            subsongBuilder.setCurrentPositionTransposition(channelIndex, transposition);
        }
        subsongBuilder.setCurrentPositionHeight(patternLastCellIndex + 1);
        subsongBuilder.finishCurrentPosition();
    }
}

void StarkosImporter::readTracks(SubsongBuilder& subsongBuilder, const juce::MemoryBlock& musicData, juce::int64& readOffset) noexcept
{
    TrackReader::readTracks(subsongBuilder, musicData, readOffset);
}

void StarkosImporter::readSpecialTracks(SubsongBuilder& subsongBuilder, const juce::MemoryBlock& musicData, juce::int64& readOffset) noexcept
{
    TrackReader::readSpecialTracks(subsongBuilder, musicData, readOffset);
}

void StarkosImporter::readInstruments(const juce::MemoryBlock& musicData, juce::int64& readOffset) const noexcept
{
    InstrumentReader::readInstruments(*songBuilder, musicData, readOffset);
}

ImportedFormat StarkosImporter::getFormat() noexcept
{
    return ImportedFormat::starkos;
}

}   // namespace arkostracker
