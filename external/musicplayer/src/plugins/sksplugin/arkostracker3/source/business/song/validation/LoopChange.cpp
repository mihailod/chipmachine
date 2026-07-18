#include "LoopChange.h"

#include "CheckLoopStartEnd.h"

namespace arkostracker 
{

Loop LoopChange::remove(const int cellIndex, const Loop& originalLoop, int originalLength) noexcept
{
    if (originalLength < 1) {
        jassertfalse;           // Cannot remove anything!
        return originalLoop;
    }

    auto newStartIndex = originalLoop.getStartIndex();
    auto newEndIndex = originalLoop.getEndIndex();

    if (cellIndex < newStartIndex) {
        --newStartIndex;
    }
    if ((cellIndex <= newEndIndex) && (newEndIndex > newStartIndex)) {
        --newEndIndex;
    }
    --originalLength;

    // Finally, corrects the loop according to the end.
    const auto [correctedStartIndex, correctedEndIndex] = CheckLoopStartEnd::checkLoopStartEnd(newStartIndex, newEndIndex, originalLength);

    return Loop(correctedStartIndex, correctedEndIndex, originalLoop.isLooping());
}

Loop LoopChange::remove(const std::set<int>& cellIndexes, const Loop& originalLoop, int currentLength) noexcept
{
    auto newLoop = originalLoop;
    // Must remove backwards!
    for (auto it = cellIndexes.crbegin(); it != cellIndexes.crend(); ++it) {
        const auto cellIndexToDelete = *it;
        newLoop = remove(cellIndexToDelete, newLoop, currentLength);
        --currentLength;
    }

    return newLoop;
}

Loop LoopChange::insert(const int cellIndex, const Loop& originalLoop) noexcept
{
    auto newStartIndex = originalLoop.getStartIndex();
    auto newEndIndex = originalLoop.getEndIndex();

    if (cellIndex < newStartIndex) {
        ++newStartIndex;
    }
    if (cellIndex <= newEndIndex) {
        ++newEndIndex;
    }

    return Loop(newStartIndex, newEndIndex, originalLoop.isLooping());
}

Loop LoopChange::insert(const std::set<int>& cellIndexes, const Loop& originalLoop) noexcept
{
    auto newLoop = originalLoop;
    // Must insert backwards!
    for (auto it = cellIndexes.crbegin(); it != cellIndexes.crend(); ++it) {
        const auto cellIndexToDelete = *it;
        newLoop = insert(cellIndexToDelete, newLoop);
    }

    return newLoop;
}

}   // namespace arkostracker
