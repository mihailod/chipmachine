#include "MovePositions.h"

#include <utility>

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/LinkerObserver.h"
#include "../../../utils/NumberUtil.h"
#include "../../../utils/SetUtil.h"
#include "../../song/validation/LoopChange.h"

namespace arkostracker 
{

MovePositions::MovePositions(SongController& pSongController, Id pSubsongId, std::set<int> pPositions, const int pDestinationPosition,
                             const bool pInsertAfter) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        positionsToMove(std::move(pPositions)),
        originalDestinationPosition(pDestinationPosition),
        originalLoop(),
        insertAfter(pInsertAfter),
        finalDestinationPosition(pDestinationPosition)
{
}

bool MovePositions::perform()
{
    // Anything to do?
    if (positionsToMove.empty()) {
        return false;
    }
    const auto song = songController.getSong();

    // Trying to move all the positions? Then don't do anything.
    auto originalLength = 0;
    song->performOnConstSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
        originalLength = subsong.getLength();
    });
    if (originalLength == static_cast<int>(positionsToMove.size())) {
        return false;
    }

    movedPositions.clear();
    movedPositions.reserve(positionsToMove.size());


    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        originalLoop = subsong.getLoop();

        // Stores them in the meantime. Also checks whether the destination should be shifted.
        auto destinationPosition = originalDestinationPosition;
        for (const auto positionIndex : positionsToMove) {
            auto readPosition = subsong.getPosition(positionIndex);
            movedPositions.push_back(readPosition);

            // If the destination was after, it goes "to the left". Note that the order of the position does not matter, no need to reserve-browse.
            if (destinationPosition >= positionIndex) {
                --destinationPosition;
            }
        }
        // Makes sure the destination is not beyond the last position.
        destinationPosition = std::max(0, std::min(destinationPosition, originalLength - 1 - static_cast<int>(movedPositions.size())));
        finalDestinationPosition = destinationPosition;

        // Removes the Positions.
        subsong.removePositions(positionsToMove);
        const auto newLoop = LoopChange::remove(positionsToMove, originalLoop, originalLength);
        subsong.setLoop(newLoop);

        jassert(movedPositions.size() == positionsToMove.size());

        // Puts the stored Positions to the new destination.
        subsong.addPositions(destinationPosition, insertAfter, movedPositions);
    });

    // Notifies.
    // The highlighted items are the ones starting at the destination position, as they are added linearly.
    const auto highlightedItems = SetUtil::createLinearItems<int>(finalDestinationPosition, insertAfter, static_cast<int>(positionsToMove.size()));
    notifyListeners(highlightedItems);

    return true;
}

bool MovePositions::undo()
{
    jassert(positionsToMove.size() == movedPositions.size());

    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        // Removes the newly moved Positions. They were put linearly.
        subsong.removePositions(finalDestinationPosition, insertAfter, static_cast<int>(movedPositions.size()));

        // Adds the original Positions in the same order.
        size_t i = 0;
        for (const auto& insertionIndex : positionsToMove) {
            const auto& positionData = movedPositions.at(i);
            subsong.addPosition(insertionIndex, false, positionData);

            ++i;
        }

        // The loop may be broken by removal/insertion. Simply restores it.
        subsong.setLoop(originalLoop);
    });

    // Notifies.
    notifyListeners(positionsToMove);

    return true;
}

void MovePositions::notifyListeners(const std::set<int>& highlightedItems) const noexcept
{
    songController.getLinkerObservers().applyOnObservers([&](LinkerObserver* observer) noexcept {
        observer->onLinkerPositionsInvalidated(subsongId, highlightedItems);
    });
}

}   // namespace arkostracker
