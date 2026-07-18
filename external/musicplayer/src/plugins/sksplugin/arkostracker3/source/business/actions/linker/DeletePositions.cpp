#include "DeletePositions.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/LinkerObserver.h"
#include "../../../utils/SetUtil.h"
#include "../../song/validation/CheckLoopStartEnd.h"
#include "../../song/validation/LoopChange.h"

namespace arkostracker 
{

DeletePositions::DeletePositions(SongController& pSongController, Id pSubsongId, std::set<int> pPositions) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        positionsToDelete(std::move(pPositions)),
        deletedPositions(),
        originalLoop()
{
}

bool DeletePositions::perform()
{
    // Anything to do?
    if (positionsToDelete.empty()) {
        return false;
    }

    deletedPositions.clear();
    deletedPositions.reserve(positionsToDelete.size());

    const auto song = songController.getSong();
    std::set<int> highlightedIndexes;
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        // Deleting and undoing by adding does not work well in some case (deleting on the loop end)
        // because it is not symmetric. Much easier to simply store them.
        originalLoop = subsong.getLoop();
        const auto originalLength = subsong.getLength();

        // Stores all the Positions to remove, for the Redo.
        for (const auto positionIndex : positionsToDelete) {
            auto readPosition = subsong.getPosition(positionIndex);
            deletedPositions.push_back(readPosition);
        }

        // Deletes the Positions.
        subsong.removePositions(positionsToDelete);

        // The highlighted item is the first position that is deleted.
        // However, we have to make sure it is still within bounds.
        const auto newLength = subsong.getLength();
        const auto highlightedIndex = std::min(newLength - 1, SetUtil::min(positionsToDelete));
        if (highlightedIndex >= 0) {                // Shouldn't happen, but maybe the last item were deleted.
            highlightedIndexes.insert(highlightedIndex);
        }

        // Updates the loop. As the deleted positions are not linear, removes them one by one.
        const auto newLoop = LoopChange::remove(positionsToDelete, originalLoop, originalLength);
        subsong.setLoop(newLoop);
    });

    jassert(positionsToDelete.size() == deletedPositions.size());

    // Notifies.
    // The highlighted position is the first one, if still the song is still large enough.
    notifyListeners(highlightedIndexes);

    return true;
}

bool DeletePositions::undo()
{
    jassert(positionsToDelete.size() == deletedPositions.size());

    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        // Adds the deleted Positions.
        size_t i = 0;
        for (const auto& insertionIndex : positionsToDelete) {
            const auto& positionData = deletedPositions.at(i);
            subsong.addPosition(insertionIndex, false, positionData);

            ++i;
        }

        subsong.setLoop(originalLoop);
    });

    // Notifies.
    notifyListeners(positionsToDelete);

    return true;
}

void DeletePositions::notifyListeners(const std::set<int>& highlightedItems) const noexcept
{
    songController.getLinkerObservers().applyOnObservers([&](LinkerObserver* observer) noexcept {
        observer->onLinkerPositionsInvalidated(subsongId, highlightedItems);
    });
}

}   // namespace arkostracker
