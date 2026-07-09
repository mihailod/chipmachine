#include "PatternOptimizer.h"

#include "../../../../utils/TaggedObject.h"
#include "TrackOptimizer.h"

namespace arkostracker 
{

using patternAndIndex = TaggedObject<int, Pattern>;

PatternOptimizer::PatternOptimizer(const Song& pOriginalSong, std::vector<Id> pSubsongIds) noexcept :
        originalSong(pOriginalSong),        // Performs a copy.
        originalSubsongIds(std::move(pSubsongIds)),
        originalPositions(),
        originalPatterns(),
        originalTracks(),
        originalSpeedTracks(),
        originalEventTracks(),
        sortedTracks(),
        sortedSpeedTracks(),
        sortedEventTracks()
{
}

std::unique_ptr<Song> PatternOptimizer::optimize(const bool keepOnlyTillSubsongLoopEnd, const bool normalizeCellsAndOptimizeTracks) noexcept
{
    if (!originalPositions.empty()) {
        jassertfalse;           // This method must only be called once!
        return nullptr;
    }

    auto newSong = std::make_unique<Song>(originalSong.getTitle(), originalSong.getAuthor(), originalSong.getComposer(), originalSong.getComments());

    for (const auto& originalSubsongId : originalSubsongIds) {
        // Tries to remove speed that does not change, yet is declared throughout the speed tracks.
        originalSong.performOnSubsong(originalSubsongId, [&] (Subsong& subsong) noexcept {
            optimizeSpeedTracks(subsong, keepOnlyTillSubsongLoopEnd);
        });

        // First, gets the Tracks that are used, stores them from highest to smallest according to the pattern height, removing their content if needed.
        std::unordered_map<int, int> sortedTrackIndexToNew;     // Links the index of the original Track into the index of the new one.
        std::vector<Track> finalTracks;                         // The Tracks to use in the final Song, in the right order.
        std::vector<SpecialTrack> finalSpeedTracks;             // The SpeedTracks to use in the final Song, in the right order.
        std::unordered_map<int, int> sortedSpeedTrackIndexToNew;     // Links the index of the original SpeedTrack into the index of the new one.
        std::vector<SpecialTrack> finalEventTracks;             // The SpeedTracks to use in the final Song, in the right order.
        std::unordered_map<int, int> sortedEventTrackIndexToNew;     // Links the index of the original EventTrack into the index of the new one.
        std::vector<Pattern> finalPatterns;                     // The Patterns to use in the final Song, in the right order.

        originalSong.performOnConstSubsong(originalSubsongId, [&](const Subsong& subsong) noexcept {
            originalPositions = subsong.getPositions(keepOnlyTillSubsongLoopEnd);
            originalPatterns = subsong.getPatterns();
            originalTracks = subsong.getTracks();
            originalSpeedTracks = subsong.getSpecialTracks(true);
            originalEventTracks = subsong.getSpecialTracks(false);
        });

        createSortedAndCorrectedAndReducedTracks(normalizeCellsAndOptimizeTracks);

        // Creates new Patterns using the new Tracks, if their original Track were included to any of them.
        std::vector<Position> finalPositions;
        finalPositions.reserve(originalPositions.size());

        std::vector<patternAndIndex> newPatternsAndIndexes;

        for (const auto& originalPosition : originalPositions) {
            std::vector<int> newPatternTrackIndexes;

            const auto positionHeight = originalPosition.getHeight();
            const auto& originalPattern = originalPatterns.at(static_cast<size_t>(originalPosition.getPatternIndex()));
            for (const auto originalTrackIndex : originalPattern.getCurrentTrackIndexes()) {
                const auto finalTrackIndex = findOptimizedTrack(sortedTracks, sortedTrackIndexToNew, originalTrackIndex, originalTracks,
                                                                normalizeCellsAndOptimizeTracks, positionHeight, finalTracks);
                if (finalTrackIndex.isAbsent()) {
                    jassertfalse;       // Abnormal!! Should never happen!
                    return nullptr;
                }

                newPatternTrackIndexes.emplace_back(finalTrackIndex.getValue());
            }
            // The same for the Speed Track.
            const auto originalSpeedTrackIndex = originalPattern.getCurrentSpecialTrackIndex(true);
            const auto newSpeedTrackIndex = findOptimizedTrack(sortedSpeedTracks, sortedSpeedTrackIndexToNew, originalSpeedTrackIndex,
                                                               originalSpeedTracks, normalizeCellsAndOptimizeTracks, positionHeight, finalSpeedTracks);
            // The same for the Event Track. It is NOT normalized.
            const auto originalEventTrackIndex = originalPattern.getCurrentSpecialTrackIndex(false);
            const auto newEventTrackIndex = findOptimizedTrack(sortedEventTracks, sortedEventTrackIndexToNew, originalEventTrackIndex,
                                                               originalEventTracks, false, positionHeight, finalEventTracks);
            if (newSpeedTrackIndex.isAbsent() || newEventTrackIndex.isAbsent()) {
                jassertfalse;       // Abnormal!! Should never happen!
                return nullptr;
            }

            Pattern newPattern(newPatternTrackIndexes, newSpeedTrackIndex.getValue(), newEventTrackIndex.getValue(), originalPattern.getArgbColor());

            // Is the Pattern new?
            int newPatternIndex;        // NOLINT(*-init-variables)
            if (auto it = std::find_if(newPatternsAndIndexes.begin(), newPatternsAndIndexes.end(), [&](const patternAndIndex& item) {
                return (item.getObjectRef().areEqualMusically(newPattern));     // Don't compare the color!
            }); it == newPatternsAndIndexes.end()) {
                // New pattern. Its "id" is its index, which is the size.
                newPatternIndex = static_cast<int>(newPatternsAndIndexes.size());
                newPatternsAndIndexes.emplace_back(newPattern, newPatternIndex);

                finalPatterns.push_back(newPattern);
            } else {
                // Already known Pattern. The tag is its index.
                newPatternIndex = it->getTag();
            }

            // Creates a new Position which references the Pattern, old or new.
            finalPositions.emplace_back(newPatternIndex, positionHeight, originalPosition.getMarkerName(), originalPosition.getMarkerColor(),
                                      originalPosition.getChannelToTransposition());
        }
        jassert(finalPositions.size() == originalPositions.size());

        // Creates the Subsong.
        const auto originalSubsongMetadata = originalSong.getSubsongMetadata(originalSubsongId);
        const auto originalSubsongPsgs = originalSong.getSubsongPsgs(originalSubsongId);
        auto newSubsong = std::make_unique<Subsong>(originalSubsongMetadata.getName(), originalSubsongMetadata.getInitialSpeed(),
                                                    originalSubsongMetadata.getReplayFrequencyHz(),
                                                    originalSubsongMetadata.getDigiChannel(), originalSubsongMetadata.getHighlightSpacing(),
                                                    originalSubsongMetadata.getSecondaryHighlight(), 0, 0, // See why below.
                                                    originalSubsongPsgs, originalSubsongMetadata.getSidPlayerCapability(), false);

        // Encodes the Tracks/Patterns/Positions in the output Song.
        for (const auto& track : finalTracks) {
            newSubsong->addTrack(track);
        }
        for (const auto& specialTrack : finalSpeedTracks) {
            newSubsong->addSpecialTrack(true, specialTrack);
        }
        for (const auto& specialTrack : finalEventTracks) {
            newSubsong->addSpecialTrack(false, specialTrack);
        }

        for (const auto& pattern : finalPatterns) {
            newSubsong->addPattern(pattern);
        }
        // This DOES change the loop, so it is set after.
        for (const auto& position : finalPositions) {
            newSubsong->addPosition(position);
        }
        newSubsong->setLoopAndEndStartPosition(originalSubsongMetadata.getLoopStartPosition(), originalSubsongMetadata.getEndPosition());

        newSong->addSubsong(std::move(newSubsong));
    }

    // Finally, duplicates all the Instruments/Expressions from the original song.
    originalSong.performOnConstInstruments([&](const std::vector<std::unique_ptr<Instrument>>& instruments) {
        for (const auto& instrument : instruments) {
            auto newInstrument = std::make_unique<Instrument>(*instrument);
            newSong->addInstrument(std::move(newInstrument));
        }
    });
    originalSong.getConstExpressionHandler(true).performOnConstExpressions([&](const std::vector<std::unique_ptr<Expression>>& expressions) {
        for (const auto& expression : expressions) {
            newSong->getExpressionHandler(true).addExpression(*expression);
        }
    });
    originalSong.getConstExpressionHandler(false).performOnConstExpressions([&](const std::vector<std::unique_ptr<Expression>>& expressions) {
        for (const auto& expression : expressions) {
            newSong->getExpressionHandler(false).addExpression(*expression);
        }
    });

    return newSong;
}

void PatternOptimizer::createSortedAndCorrectedAndReducedTracks(const bool normalizeCells) noexcept
{
    for (const auto& position : originalPositions) {
        const auto positionHeight = position.getHeight();
        const auto patternIndex = static_cast<size_t>(position.getPatternIndex());
        const auto& pattern = originalPatterns.at(patternIndex);
        for (const auto trackIndex : pattern.getCurrentTrackIndexes()) {
            // Duplicates the Track, resizing it to what the Position requires.
            const auto& originalTrack = originalTracks.at(static_cast<size_t>(trackIndex));
            auto normalizedTrack = normalizeTrack(originalTrack, normalizeCells, positionHeight);

            sortedTracks.push_back(normalizedTrack);
        }
        // The same for the Special Tracks.
        const auto speedTrackIndex = pattern.getCurrentSpecialTrackIndex(true);
        const auto reducedSpeedTrack = normalizeTrack(originalSpeedTracks.at(static_cast<size_t>(speedTrackIndex)), normalizeCells, positionHeight);
        sortedSpeedTracks.push_back(reducedSpeedTrack);
        // The event Track is NOT normalized.
        const auto eventTrackIndex = pattern.getCurrentSpecialTrackIndex(false);
        const auto reducedEventTrack = normalizeTrack(originalEventTracks.at(static_cast<size_t>(eventTrackIndex)), false, positionHeight);
        sortedEventTracks.push_back(reducedEventTrack);
    }

    sortTracks(sortedTracks);
    sortTracks(sortedSpeedTracks);
    sortTracks(sortedEventTracks);
}

Track PatternOptimizer::normalizeTrack(const Track& originalTrack, const bool normalizeCellsAndOptimizeTracks, const int positionHeight) noexcept
{
    auto reducedTrack = originalTrack.reducedTo(positionHeight);

    // Optimizes the Track, if wanted.
    if (normalizeCellsAndOptimizeTracks) {
        TrackOptimizer::optimizeTrack(reducedTrack);
    }

    return reducedTrack;
}

SpecialTrack PatternOptimizer::normalizeTrack(const SpecialTrack& originalTrack, const bool normalizeCells, const int positionHeight) noexcept
{
    auto reducedTrack = originalTrack.reducedTo(positionHeight);

    // This should ONLY be true for Speed Tracks!
    if (normalizeCells) {
        auto currentValue = -1;
        for (auto cellIndex = 0; cellIndex < Track::getSize(); ++cellIndex) {
            const auto readValueCell = reducedTrack.getCellRefConst(cellIndex);
            if (readValueCell.isEmpty()) {
                continue;
            }

            // Is the read value the same as before? If yes, removes it.
            const auto readValue = readValueCell.getValue();
            if (readValue == currentValue) {
                reducedTrack.setCell(cellIndex, SpecialCell::buildEmptySpecialCell());
            } else {
                currentValue = readValue;
            }
        }
    }

    return reducedTrack;
}

void PatternOptimizer::optimizeSpeedTracks(Subsong& subsong, const bool keepOnlyTillSubsongLoopEnd) noexcept
{
    if (subsong.getLength() == 0) {
        jassertfalse;
        return;     // Security, abnormal.
    }

    auto initialSpeed = subsong.getMetadata().getInitialSpeed();

    // Checks whether the first speed track used has a speed track declared, and it is different.
    const auto firstSpeedTrackIndex = subsong.getPatternRef(0).getCurrentSpecialTrackIndex(true);
    const auto& firstSpeedTrack = subsong.getSpecialTrackRefFromIndex(firstSpeedTrackIndex, true);
    if (const auto readFirstSpeedValue = firstSpeedTrack.getCellRefConst(0); !readFirstSpeedValue.isEmpty() && readFirstSpeedValue.getValue() != initialSpeed) {
        // The first speed is different, so it overrides the initial speed.
        initialSpeed = readFirstSpeedValue.getValue();
    }

    auto canOptimize = true;

    // Browses through all the speed tracks, no different speed tracks should be found in order for the optimization to work.
    const auto lastPositionIndex = keepOnlyTillSubsongLoopEnd ? subsong.getEndPosition() : subsong.getLength() - 1;

    for (auto positionIndex = 0; canOptimize && (positionIndex <= lastPositionIndex); ++positionIndex) {
        const auto positionHeight = subsong.getPositionRef(positionIndex).getHeight();
        const auto& pattern = subsong.getPatternRef(positionIndex);
        const auto speedTrackIndex = pattern.getCurrentSpecialTrackIndex(true);

        auto& speedTrack = subsong.getSpecialTrackRefFromIndex(speedTrackIndex, true);
        for (auto cellIndex = 0; cellIndex < positionHeight; ++cellIndex) {
            const auto readSpeed = speedTrack.getCellRefConst(cellIndex);
            if (!readSpeed.isEmpty() && (readSpeed.getValue() != initialSpeed)) {
                // A different speed is found. Cannot optimize.
                canOptimize = false;
                break;
            }
        }
    }

    if (canOptimize) {
        // Browses through all the speed tracks to remove the speeds.
        for (auto positionIndex = 0; positionIndex <= lastPositionIndex; ++positionIndex) {
            const auto& pattern = subsong.getPatternRef(positionIndex);
            const auto speedTrackIndex = pattern.getCurrentSpecialTrackIndex(true);

            auto& speedTrack = subsong.getSpecialTrackRefFromIndex(speedTrackIndex, true);
            speedTrack.clearCells();
        }

        subsong.setInitialSpeed(initialSpeed);
    }
}

}   // namespace arkostracker
