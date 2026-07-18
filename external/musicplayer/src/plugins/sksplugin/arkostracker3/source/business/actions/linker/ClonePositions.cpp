#include "ClonePositions.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/LinkerObserver.h"
#include "../../../utils/SetUtil.h"

namespace arkostracker 
{

ClonePositions::ClonePositions(SongController& pSongController, Id pSubsongId, std::set<int> pPositions,
                               const int pDestinationPosition, const bool pInsertAfter) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        sourcePositionIndexes(std::move(pPositions)),
        destinationPosition(pDestinationPosition),
        insertAfter(pInsertAfter),
        addedTrackCount(0),
        addedSpeedTrackCount(0),
        addedEventTrackCount(0),
        addedPatternCount(0)
{
}

bool ClonePositions::perform()
{
    // Anything to do?
    if (sourcePositionIndexes.empty()) {
        return false;
    }

    // If same Pattern in Positions, the *new* same Pattern must be reused.
    std::unordered_map<int, int> originalPatternIndexToNewPatternIndex;
    addedTrackCount = 0;
    addedSpeedTrackCount = 0;
    addedEventTrackCount = 0;
    addedPatternCount = 0;

    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        // To simplify the algorithm, gets all the Position Data in a list, to be able to use the "addPositions" method.
        std::vector<Position> positionsData;
        positionsData.reserve(sourcePositionIndexes.size());
        for (const auto positionIndex : sourcePositionIndexes) {
            auto readPosition = subsong.getPosition(positionIndex);
            readPosition.setMarkerNameAndColour({ }, readPosition.getMarkerColor());        // Removes the marker.

            const auto originalPatternIndex = readPosition.getPatternIndex();
            // Has a new pattern has been created for the original Pattern? If yes, it must be reused.
            if (const auto it = originalPatternIndexToNewPatternIndex.find(originalPatternIndex); it != originalPatternIndexToNewPatternIndex.cend()) {
                const auto newPosition = readPosition.withPatternIndex(it->second);
                positionsData.push_back(newPosition);
                continue;
            }

            // Clones the non-linked track no matter what into new tracks, but keep the linked tracks.
            // The linked-to track are linked in the new position.
            const auto originalPattern = subsong.getPatternFromIndex(originalPatternIndex);
            // Creates an empty Pattern and fills it.
            Pattern newPattern(originalPattern.getArgbColor());
            auto channelIndex = 0;
            for (const auto& originalTrackIdAndLinkedTrackId : originalPattern.getTrackIndexesAndLinkedTrackIndexes()) {
                const auto linkState = subsong.getTrackLinkState(positionIndex, channelIndex);

                // If "linked-to", creates an empty track, else duplicates it.
                Track newTrack;
                if (linkState != LinkState::linkedTo) {
                    newTrack = subsong.getTrackRefFromIndex(originalTrackIdAndLinkedTrackId.getUnlinkedTrackIndex());
                }

                //newTrack.setName({ });        // Was the behavior... But with the color, it was strange. So removed.
                const auto addedTrackIndex = subsong.addTrack(newTrack);
                newPattern.addTrackIndex(addedTrackIndex,
                                         // If linked-to, the new pattern will link to the original track.
                                         (linkState == LinkState::linkedTo)
                                             ? originalPattern.getCurrentTrackIndex(channelIndex)
                                             : originalTrackIdAndLinkedTrackId.getLinkedTrackIndex()
                );

                ++addedTrackCount;
                ++channelIndex;
            }

            // Clones the non-linked Speed Track.
            cloneSpecialTrack(true, subsong, positionIndex, originalPattern, newPattern);
            cloneSpecialTrack(false, subsong, positionIndex, originalPattern, newPattern);

            // Adds the new Pattern.
            const auto newPatternIndex = subsong.addPattern(newPattern);
            ++addedPatternCount;
            const auto newPosition = readPosition.withPatternIndex(newPatternIndex);

            positionsData.push_back(newPosition);

            // Stores the new pattern in the map, for later retrieval if the Position is used several times.
            originalPatternIndexToNewPatternIndex.insert(std::make_pair(originalPatternIndex, newPatternIndex));
        }

        // Inserts the new data.
        subsong.addPositions(destinationPosition, insertAfter, positionsData);
    });

    // Notifies.
    // The highlighted items are the ones starting at the destination position, as they are added linearly.
    const auto highlightedItems = SetUtil::createLinearItems<int>(destinationPosition, insertAfter, static_cast<int>(sourcePositionIndexes.size()));
    notifyListeners(highlightedItems);

    return true;
}

void ClonePositions::cloneSpecialTrack(const bool isSpeedTrack, Subsong& subsong, const int positionIndex, const Pattern& originalPattern, Pattern& newPattern) noexcept
{
    const auto& originalSpecialTrackIdAndLinkedTrackId = originalPattern.getSpecialTrackIndexAndLinkedTrackIndex(isSpeedTrack);

    // If "linked-to", creates an empty track, else duplicates it.
    SpecialTrack newSpecialTrack;
    const auto linkState = subsong.getSpecialTrackLinkState(isSpeedTrack, positionIndex);
    if (linkState != LinkState::linkedTo) {
        newSpecialTrack = subsong.getSpecialTrackRefFromIndex(originalSpecialTrackIdAndLinkedTrackId.getUnlinkedTrackIndex(), isSpeedTrack);
    }

    newSpecialTrack.setName({ });
    const auto newSpecialTrackIndex = subsong.addSpecialTrack(isSpeedTrack, newSpecialTrack);
    newPattern.setSpecialTrackIndex(isSpeedTrack, newSpecialTrackIndex,
        // If linked-to, the new pattern will link to the original special track.
        (linkState == LinkState::linkedTo)
            ? originalPattern.getCurrentSpecialTrackIndex(isSpeedTrack)
            : originalSpecialTrackIdAndLinkedTrackId.getLinkedTrackIndex());

    if (isSpeedTrack) {
        ++addedSpeedTrackCount;
    } else {
        ++addedEventTrackCount;
    }
}

bool ClonePositions::undo()
{
    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        // Removes the added Positions.
        subsong.removePositions(destinationPosition, insertAfter, static_cast<int>(sourcePositionIndexes.size()));
        // Removes the added Patterns, Tracks/event/speed Tracks. They are at the end!
        subsong.removeLastPatterns(addedPatternCount);
        subsong.removeLastTracks(addedTrackCount);
        subsong.removeLastSpecialTracks(true, addedSpeedTrackCount);
        subsong.removeLastSpecialTracks(false, addedEventTrackCount);
    });

    // Notifies.
    notifyListeners(sourcePositionIndexes);

    return true;
}

void ClonePositions::notifyListeners(const std::set<int>& highlightedItems) const noexcept
{
    songController.getLinkerObservers().applyOnObservers([&](LinkerObserver* observer) noexcept {
        observer->onLinkerPositionsInvalidated(subsongId, highlightedItems);
    });
}

}   // namespace arkostracker
