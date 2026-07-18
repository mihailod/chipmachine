#include "InsertPositions.h"

#include <utility>

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/LinkerObserver.h"
#include "../../../utils/SetUtil.h"

namespace arkostracker 
{

InsertPositions::InsertPositions(SongController& pSongController, Id pSubsongId, std::vector<Position> pPositions, int pDestinationPosition, bool pInsertAfter,
                                 bool pAlsoDuplicateMarker) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        originalPositions(std::move(pPositions)),
        destinationPosition(pDestinationPosition),
        insertAfter(pInsertAfter)
{
    // If we don't want to duplicate the Marker, simply remove them from the Positions.
    if (!pAlsoDuplicateMarker) {
        for (auto& position : originalPositions) {
            position.removeMarker();
        }
    }
}

bool InsertPositions::perform()
{
    // Anything to do?
    if (originalPositions.empty()) {
        return false;
    }

    auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        // Inserts the new data.
        subsong.addPositions(destinationPosition, insertAfter, originalPositions);
    });

    // Notifies.
    // The highlighted items are the ones starting at the destination position, as they are added linearly.
    const auto highlightedItems = SetUtil::createLinearItems<int>(destinationPosition, insertAfter, static_cast<int>(originalPositions.size()));
    notifyListeners(highlightedItems);

    return true;
}

bool InsertPositions::undo()
{
    // Deletes the added positions.
    auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
       subsong.removePositions(destinationPosition, insertAfter, static_cast<int>(originalPositions.size()));
    });

    // Notifies.
    notifyListeners({ destinationPosition });       // This is the best we can do, the indexes are not known.

    return true;
}

void InsertPositions::notifyListeners(const std::set<int>& highlightedItems) const noexcept
{
    songController.getLinkerObservers().applyOnObservers([&](LinkerObserver* observer) noexcept {
        observer->onLinkerPositionsInvalidated(subsongId, highlightedItems);
    });
}


}   // namespace arkostracker

