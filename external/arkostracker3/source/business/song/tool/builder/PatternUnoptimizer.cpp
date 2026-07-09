#include "PatternUnoptimizer.h"

#include <unordered_set>

#include "../../../../song/subsong/Pattern.h"
#include "../../../../utils/MapUtil.h"

namespace arkostracker 
{

void PatternUnoptimizer::unoptimize(std::map<int, Pattern>& indexToPattern, std::map<int, Track>& indexToTrack, std::map<int, SpecialTrack>& indexToSpeedTrack,
                                  std::map<int, SpecialTrack>& indexToEventTrack) noexcept
{
    std::unordered_set<int> browsedTrackIndexes;
    std::unordered_set<int> browsedSpeedTrackIndexes;
    std::unordered_set<int> browsedEventTrackIndexes;

    // Browses the patterns. A copy is made, so that the browsing is easier (no need to use iterator, as items are going to be added).
    const std::map<int, Pattern> originalIndexToPattern = indexToPattern;
    for (const auto& [patternIndex, originalPattern] : originalIndexToPattern) {

        std::vector<int> channelIndexToNewTrackIndexesInPattern;        // Filled with either the old or new Track indexes.
        // The following items are empty if nothing has tracksHaveChanged in the Pattern.
        auto tracksHaveChanged = false;

        // Browses each track of the Pattern.
        for (auto channelIndex = 0, channelCount = originalPattern.getChannelCount(); channelIndex < channelCount; ++channelIndex) {
            const auto originalTrackIndex = originalPattern.getCurrentTrackIndex(channelIndex);
            jassert(indexToTrack.find(originalTrackIndex) != indexToTrack.cend());      // Abnormal! The Track is in the Pattern but not in the Track map!
            auto trackIndexToWrite = originalTrackIndex;
            // Does the read track already browsed?
            if (browsedTrackIndexes.find(originalTrackIndex) == browsedTrackIndexes.cend()) {
                // Track not browsed yet. Simply notes the index.
                browsedTrackIndexes.insert(originalTrackIndex);
            } else {
                // The Track has already been browsed. So we must create a unique Track from it!
                trackIndexToWrite = MapUtil::findNextHoleInKeys(indexToTrack, 0);
                // Gets a duplication of the Track, and stores it. It MUST exist!
                const auto& duplicatedTrack = indexToTrack.at(originalTrackIndex);
                indexToTrack.insert(std::make_pair(trackIndexToWrite, duplicatedTrack));

                tracksHaveChanged = true;
            }

            // Stores the Track index, tracksHaveChanged or not.
            channelIndexToNewTrackIndexesInPattern.push_back(trackIndexToWrite);
        }

        // Manages the special Tracks.
        const auto newSpeedTrackIndexesInPattern = manageSpecialTracks(true, indexToSpeedTrack, browsedSpeedTrackIndexes, originalPattern);
        const auto newEventTrackIndexesInPattern = manageSpecialTracks(false, indexToEventTrack, browsedEventTrackIndexes, originalPattern);

        // Has anything tracksHaveChanged?
        if (!tracksHaveChanged && newSpeedTrackIndexesInPattern.isAbsent() && newEventTrackIndexesInPattern.isAbsent()) {
            continue;
        }

        // Creates a new Pattern.
        const auto speedTrackIndex = newSpeedTrackIndexesInPattern.isPresent() ? newSpeedTrackIndexesInPattern.getValue() : originalPattern.getCurrentSpecialTrackIndex(true);
        const auto eventTrackIndex = newEventTrackIndexesInPattern.isPresent() ? newEventTrackIndexesInPattern.getValue() : originalPattern.getCurrentSpecialTrackIndex(false);
        const Pattern newPattern(channelIndexToNewTrackIndexesInPattern, speedTrackIndex, eventTrackIndex);

        // Overwrites the previous one.
        indexToPattern[patternIndex] = newPattern;
    }
}

OptionalInt PatternUnoptimizer::manageSpecialTracks(const bool isSpeedTrack, std::map<int, SpecialTrack>& indexToSpecialTrack, std::unordered_set<int>& browsedSpecialTrackIndexes,
                                                    const Pattern& originalPattern)
{
    // Has the SpecialTrack already been browsed?
    const auto originalSpecialTrackIndex = originalPattern.getCurrentSpecialTrackIndex(isSpeedTrack);
    jassert(indexToSpecialTrack.find(originalSpecialTrackIndex) != indexToSpecialTrack.cend());      // Abnormal! The Track is in the Pattern but not in the Track map!

    if (browsedSpecialTrackIndexes.find(originalSpecialTrackIndex) == browsedSpecialTrackIndexes.cend()) {
        // Not browsed, nothing to do except mark it as "browsed".
        browsedSpecialTrackIndexes.insert(originalSpecialTrackIndex);
        return { };
    }

    // Special Track already browsed. What new index for the new duplicate Special Track?
    const auto specialTrackIndexToWrite = MapUtil::findNextHoleInKeys(indexToSpecialTrack, 0);

    // Gets a duplication of the Special Track, and stores it. It MUST exist!
    const auto& duplicatedSpecialTrack = indexToSpecialTrack.at(originalSpecialTrackIndex);
    indexToSpecialTrack.insert(std::make_pair(specialTrackIndexToWrite, duplicatedSpecialTrack));

    return specialTrackIndexToWrite;
}

}   // namespace arkostracker
