#include "LinkerEncoder.h"

#include "../../../song/Song.h"
#include "../../../utils/BitNumber.h"
#include "../../../utils/CollectionUtil.h"
#include "../../../utils/OccurrenceMap.h"
#include "../../SongExportResult.h"
#include "../../sourceGenerator/SourceGenerator.h"

namespace arkostracker 
{

const unsigned int LinkerEncoder::minimumTrackCountForReference = 3;
const juce::String LinkerEncoder::loopLabelSuffix = "_Loop";                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

LinkerEncoder::LinkerEncoder(SourceGenerator& pSourceGenerator, SongExportResult& pSongExportResult, const Song& pSong, Id pSubsongId,
                             juce::String pSubsongLabel, juce::String pTrackIndexLabel,
                             std::function<juce::String(int)> pGetTrackLabel) noexcept:
        sourceGenerator(pSourceGenerator),
        songExportResult(pSongExportResult),
        song(pSong),
        subsongId(std::move(pSubsongId)),
        subsongLabel(std::move(pSubsongLabel)),
        trackIndexLabel(std::move(pTrackIndexLabel)),
        getTrackLabel(std::move(pGetTrackLabel)),
        sortedMostUsedTrackIndexesToOccurrence()
{
}

void LinkerEncoder::encodeLinker() noexcept
{
    sourceGenerator.declareComment("The Linker.");
    sourceGenerator.declareByteRegionStart();

    const auto subsongMetadata = song.getSubsongMetadata(subsongId);

    const auto loopStartIndex = subsongMetadata.getLoopStartPosition();

    std::vector<Pattern> patterns;
    std::vector<Position> positions;
    song.performOnConstSubsong(subsongId, [&patterns, &positions](const Subsong& subsong) {
        patterns = subsong.getPatterns();
        positions = subsong.getPositions(true);
    });
    calculateMostUsedTrackIndexes(patterns);

    const auto positionCount = positions.size();
    jassert(!positions.empty());

    // Encodes each Positions. The transpositions are only encoded if necessary.
    auto currentSpeed = subsongMetadata.getInitialSpeed();
    auto previousSpeed = currentSpeed;
    auto previousHeight = -1;
    const auto& lastPosition = positions.at(static_cast<size_t>(positionCount - 1));
    const auto& lastPattern = patterns.at(static_cast<size_t>(lastPosition.getPatternIndex()));
    Pattern previousPattern(patterns.at(0));
    // Important: the default transposition is 0. This allows not encoding the transpositions if there are none.
    auto previousTranspositionsChannel1 = 0;
    auto previousTranspositionsChannel2 = 0;
    auto previousTranspositionsChannel3 = 0;
    auto firstPattern = true;

    size_t positionIndex = 0;
    for (const auto& currentPosition : positions) {
        const auto& currentPattern = patterns.at(static_cast<size_t>(currentPosition.getPatternIndex()));

        sourceGenerator.declareComment("Position " + juce::String(positionIndex));

        // Encodes the loop label, if the Subsong loops here.
        const auto loopHere = (static_cast<int>(positionIndex) == loopStartIndex);
        if (loopHere) {
            sourceGenerator.declareLabel(subsongLabel + loopLabelSuffix);
        }

        // Encodes the speed. Only the first speed of the Speed Track is used.
        // It may not change!
        const auto speedTrackIndex = currentPattern.getCurrentSpecialTrackIndex(true);
        const auto foundSpeed = checkSpeedTrackAndGetSpeed(speedTrackIndex);
        if (foundSpeed > 0) {
            currentSpeed = foundSpeed;
        }
        auto differentSpeed = (currentSpeed != previousSpeed);
        previousSpeed = currentSpeed;

        // Is there a new height?
        const auto currentHeight = currentPosition.getHeight();
        auto differentHeight = (currentHeight != previousHeight);
        previousHeight = currentHeight;

        // Are the Track indexes different? If yes, they must be encoded.
        const auto encodeChannel1 = (firstPattern || (currentPattern.getCurrentTrackIndex(0) != previousPattern.getCurrentTrackIndex(0)));
        const auto encodeChannel2 = (firstPattern || (currentPattern.getCurrentTrackIndex(1) != previousPattern.getCurrentTrackIndex(1)));
        const auto encodeChannel3 = (firstPattern || (currentPattern.getCurrentTrackIndex(2) != previousPattern.getCurrentTrackIndex(2)));

        // Are the transposition different? If yes, they must be encoded.
        // Important: the first transposition is not encoded if it is 0, 0, 0.
        const auto currentTranspositionChannel1 = currentPosition.getTransposition(0);
        const auto currentTranspositionChannel2 = currentPosition.getTransposition(1);
        const auto currentTranspositionChannel3 = currentPosition.getTransposition(2);
        auto encodeTranspositionChannel1 = (currentTranspositionChannel1 != previousTranspositionsChannel1);
        auto encodeTranspositionChannel2 = (currentTranspositionChannel2 != previousTranspositionsChannel2);
        auto encodeTranspositionChannel3 = (currentTranspositionChannel3 != previousTranspositionsChannel3);

        // Maybe the Song is looping here? If yes, forces the encoding of the transposition/speed/height that have changed.
        if (loopHere) {
            // Compares the current transposition with the one from the last Pattern. If they are not the same, the current
            // one must be encoded.

            //const Transpositions lastTranspositions(lastPattern, psgIndex);
            if (!encodeTranspositionChannel1) {
                encodeTranspositionChannel1 = (currentTranspositionChannel1 != lastPosition.getTransposition(0));
            }
            if (!encodeTranspositionChannel2) {
                encodeTranspositionChannel2 = (currentTranspositionChannel2 != lastPosition.getTransposition(1));
            }
            if (!encodeTranspositionChannel3) {
                encodeTranspositionChannel3 = (currentTranspositionChannel3 != lastPosition.getTransposition(2));
            }

            if (!differentSpeed) {
                auto lastFoundSpeed = checkSpeedTrackAndGetSpeed(lastPattern.getCurrentSpecialTrackIndex(true));
                if (lastFoundSpeed > 0) {
                    differentSpeed = (currentSpeed != lastFoundSpeed);
                }
            }

            differentHeight |= (currentHeight != lastPosition.getHeight());
        }

        // Encodes the first byte.
        BitNumber number;
        number.injectBool(differentSpeed);      // This is not the end of the Song.
        number.injectBool(differentHeight);
        number.injectBool(encodeTranspositionChannel1);
        number.injectBool(encodeChannel1);
        number.injectBool(encodeTranspositionChannel2);
        number.injectBool(encodeChannel2);
        number.injectBool(encodeTranspositionChannel3);
        number.injectBool(encodeChannel3);
        sourceGenerator.declareByte(number.get(), "State byte.");

        // Encodes the speed?
        if (differentSpeed) {
            sourceGenerator.declareByte(currentSpeed, "New speed (>0).");
            songExportResult.addFlag(PlayerConfigurationFlag::speedTracks);
        }

        // Encodes the height?
        if (differentHeight) {
            sourceGenerator.declareByte(currentHeight - 1, "New height.");     // Height from 0 to 127.
        }

        // Encodes the possible transpositions, and the Tracks that have changed, as a reference or an offset.
        if (encodeTranspositionChannel1) {
            sourceGenerator.declareByte(currentTranspositionChannel1, "New transposition on channel 1.");
        }
        if (encodeChannel1) {
            encodeTrackInPattern(0, currentPattern.getCurrentTrackIndex(0));
        }

        if (encodeTranspositionChannel2) {
            sourceGenerator.declareByte(currentTranspositionChannel2, "New transposition on channel 2.");
        }
        if (encodeChannel2) {
            encodeTrackInPattern(1, currentPattern.getCurrentTrackIndex(1));
        }

        if (encodeTranspositionChannel3) {
            sourceGenerator.declareByte(currentTranspositionChannel3, "New transposition on channel 3.");
        }
        if (encodeChannel3) {
            encodeTrackInPattern(2, currentPattern.getCurrentTrackIndex(2));
        }
        sourceGenerator.addEmptyLine();

        songExportResult.addFlag(encodeTranspositionChannel1 || encodeTranspositionChannel2 || encodeTranspositionChannel3,
                                 PlayerConfigurationFlag::transpositions);

        previousTranspositionsChannel1 = currentTranspositionChannel1;
        previousTranspositionsChannel2 = currentTranspositionChannel2;
        previousTranspositionsChannel3 = currentTranspositionChannel3;
        previousPattern = currentPattern;

        firstPattern = false;
        ++positionIndex;
    }

    // Encodes the loop.
    sourceGenerator.declareByte(0b1, "End of the Song.");
    sourceGenerator.declareByte(0, "Speed to 0, meaning \"end of song\".");
    sourceGenerator.declareByteRegionEnd();
    sourceGenerator.declarePointerRegionStart();
    sourceGenerator.declareWord(subsongLabel + loopLabelSuffix).addEmptyLine();
    sourceGenerator.declarePointerRegionEnd();

    // Finally, encodes the Track Indexes.
    encodeTrackIndexes();
}

int LinkerEncoder::checkSpeedTrackAndGetSpeed(int speedTrackIndex) const noexcept
{
    SpecialTrack speedTrack;
    song.performOnConstSubsong(subsongId, [&speedTrack, &speedTrackIndex](const Subsong& subsong) {
        speedTrack = subsong.getSpecialTrackRefFromIndex(speedTrackIndex, true);
    });

    // Gets the speed of the first cell. Only it matters in this format.
    const auto speed = speedTrack.getCellRefConst(0).getValue();

    auto error = false;
    // Checks that no other values are encoded.
    for (auto i = 1; i < Position::maximumPositionHeight; ++i) {       // Skips the first value.
        if (!speedTrack.isCellEmpty(i)) {
            // A value was found. Error!
            error = true;
            break;
        }
    }

    if (error) {
        songExportResult.addWarning("Only the first cell of the Speed Track " + juce::String(speedTrackIndex)
            + " should have a speed value. The others are discarded in this format.");
    }

    return speed;
}

void LinkerEncoder::calculateMostUsedTrackIndexes(const std::vector<Pattern>& patterns) noexcept
{
    OccurrenceMap<int> occurrenceMap;

    // Builds the map linking track indexes to their occurrence.
    for (const auto& pattern : patterns) {
        for (auto channelIndex = 0, channelCount = pattern.getChannelCount(); channelIndex < channelCount; ++channelIndex) {
            const auto trackIndex = pattern.getCurrentTrackIndex(channelIndex);
            occurrenceMap.addItem(trackIndex);
        }
    }

    auto list = occurrenceMap.generateSortedMostUsedItemsAndOccurrence();

    // Removes the ones that are used only once.
    CollectionUtil::removeIfAndResize(list, [](const auto& item) {
        return (item.second < minimumTrackCountForReference);
    });
    CollectionUtil::shrinkIfBigger(list, 128);      // Limits to 128.

    sortedMostUsedTrackIndexesToOccurrence = list;
}

void LinkerEncoder::encodeTrackInPattern(int channelIndex, int trackIndex) noexcept
{
    const auto trackLabel = getTrackLabel(trackIndex);
    const auto comment = "New track (" + juce::String(trackIndex) + ") for channel " + juce::String(channelIndex + 1);

    // Encodes as a reference or an offset? Reference, only if worth it.

    auto useReference = false;
    auto encodedTrackId = 0U;
    // Finds the track index in the list. Note the use of a simple iterator instead of std::find, because
    // we use this loop to get the encoded Track ID.
    auto iterator = sortedMostUsedTrackIndexesToOccurrence.begin();
    while (!useReference && (iterator != sortedMostUsedTrackIndexesToOccurrence.cend())) {
        if (iterator->first == trackIndex) {
            useReference = true;
        } else {
            ++iterator;
            ++encodedTrackId;
        }
    }

    if (useReference) {
        // Found a reference. Encodes it. Bit 7 set: reference.
        jassert(encodedTrackId < 128);
        sourceGenerator.declareByte(static_cast<int>(encodedTrackId & 0b01111111U) + 128, comment + ", as a reference (index " +
            juce::String(encodedTrackId) + ").");
    } else {
        // No reference, encodes the offset (from the address next to the encoded word). Bit 7 not set.
        const auto msb = "((" + trackLabel + " - ($ + 2)) & #ff00) / 256";
        const auto lsb = "((" + trackLabel + " - ($ + 1)) & 255)";

        sourceGenerator.declareByte(msb, comment + ", as an offset. Offset MSB, then LSB.");
        sourceGenerator.declareByte(lsb).addEmptyLine();
    }
}

void LinkerEncoder::encodeTrackIndexes() const noexcept
{
    sourceGenerator.declareComment("The indexes of the tracks.");
    sourceGenerator.declareLabel(trackIndexLabel);

    sourceGenerator.declarePointerRegionStart();

    auto encodedTrackId = 0;
    for (const auto& iterator : sortedMostUsedTrackIndexesToOccurrence) {
        jassert(iterator.second >= minimumTrackCountForReference);
        const auto trackIndex = iterator.first;

        sourceGenerator.declareWord(getTrackLabel(trackIndex), "Track " + juce::String(trackIndex) + ", index "
            + juce::String(encodedTrackId) + ".");

        ++encodedTrackId;
    }

    sourceGenerator.declarePointerRegionEnd();
}

}   // namespace arkostracker
