#include "DuplicatePositions.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/LinkerObserver.h"
#include "../../../utils/SetUtil.h"

namespace arkostracker 
{

DuplicatePositions::DuplicatePositions(SongController& pSongController, Id pSubsongId, std::set<int> pPositions,
                                       const int pDestinationPosition, const bool pInsertAfter, const bool pAlsoDuplicateMarker) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        sourcePositions(std::move(pPositions)),
        destinationPosition(pDestinationPosition),
        insertAfter(pInsertAfter),
        alsoDuplicateMarker(pAlsoDuplicateMarker)
{
}

bool DuplicatePositions::perform()
{
    // Anything to do?
    if (sourcePositions.empty()) {
        return false;
    }

    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        // To simplify the algorithm, gets all the Position Data in a list.
        std::vector<Position> positionsData;
        positionsData.reserve(sourcePositions.size());
        for (const auto positionIndex : sourcePositions) {
            auto readPosition = subsong.getPosition(positionIndex);
            // Keeps the marker?
            if (!alsoDuplicateMarker) {
                readPosition.setMarkerNameAndColour({ }, readPosition.getMarkerColor());
            }

            positionsData.push_back(readPosition);
        }

        // Inserts the new data.
        subsong.addPositions(destinationPosition, insertAfter, positionsData);
    });

    // Notifies.
    // The highlighted items are the ones starting at the destination position, as they are added linearly.
    const auto highlightedItems = SetUtil::createLinearItems<int>(destinationPosition, insertAfter, static_cast<int>(sourcePositions.size()));
    notifyListeners(highlightedItems);

    return true;
}

bool DuplicatePositions::undo()
{
    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        // Simply removes the added Positions.
        subsong.removePositions(destinationPosition, insertAfter, static_cast<int>(sourcePositions.size()));
    });

    // Notifies.
    notifyListeners(sourcePositions);

    return true;
}

void DuplicatePositions::notifyListeners(const std::set<int>& highlightedItems) const noexcept
{
    songController.getLinkerObservers().applyOnObservers([&](LinkerObserver* observer) noexcept {
        observer->onLinkerPositionsInvalidated(subsongId, highlightedItems);
    });
}

}   // namespace arkostracker
