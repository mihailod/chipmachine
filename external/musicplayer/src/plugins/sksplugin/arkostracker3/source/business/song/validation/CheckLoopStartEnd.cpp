#include "CheckLoopStartEnd.h"

#include <algorithm>

#include "../../../utils/NumberUtil.h"

namespace arkostracker 
{

Loop CheckLoopStartEnd::checkLoopStartEnd(const Loop& loop, const int length) noexcept
{
    const auto [start, end] = checkLoopStartEnd(loop.getStartIndex(), loop.getEndIndex(), length);
    return Loop(start, end, loop.isLooping());
}

std::pair<int, int> CheckLoopStartEnd::checkLoopStartEnd(int loopStartIndex, int endIndex, const int length) noexcept
{
    if (length <= 0) {
        jassertfalse;            // Problem...
        return { 0, 0 };
    }

    // Checks the end first.
    endIndex = NumberUtil::correctNumber(endIndex, 0, length - 1);

    // Then the start.
    loopStartIndex = NumberUtil::correctNumber(loopStartIndex, 0, endIndex);

    return { loopStartIndex, endIndex };
}

std::pair<int, int> CheckLoopStartEnd::checkLoopStartEnd(int currentLoopStartPosition, int currentEndPosition, const int length,
                                                         const OptionalInt optionalNewLoopStartPosition, const OptionalInt optionalNewEndPosition) noexcept
{
    // No new values? Don't do anything.
    if (optionalNewLoopStartPosition.isAbsent() && optionalNewEndPosition.isAbsent()) {
        return { currentLoopStartPosition, currentEndPosition };
    }

    // Gets the current loop.
    const auto lastPosition = length - 1;

    // The simplest case first: both start/end are present.
    if (optionalNewLoopStartPosition.isPresent() && optionalNewEndPosition.isPresent()) {
        auto newEndPosition = NumberUtil::correctNumber(optionalNewEndPosition.getValue(), 0, lastPosition);
        auto newLoopStartPosition = NumberUtil::correctNumber(optionalNewLoopStartPosition.getValue(), 0, newEndPosition);

        return { newLoopStartPosition, newEndPosition };
    }

    // Other case: start OR end is set. What is set has priority over the other.

    // What must change? Corrects the positions one from another.
    if (optionalNewLoopStartPosition.isPresent()) {
        // Corrects the input within boundaries.
        const auto newLoopStartPosition = NumberUtil::correctNumber(optionalNewLoopStartPosition.getValue(), 0, lastPosition);

        currentLoopStartPosition = newLoopStartPosition;
        currentEndPosition = std::max(currentEndPosition, currentLoopStartPosition);
    } else if (optionalNewEndPosition.isPresent()) {
        // Corrects the input within boundaries.
        const auto newEndPosition = NumberUtil::correctNumber(optionalNewEndPosition.getValue(), 0, lastPosition);

        currentEndPosition = newEndPosition;
        currentLoopStartPosition = std::min(currentLoopStartPosition, currentEndPosition);
    }

    return { currentLoopStartPosition, currentEndPosition };
}

}   // namespace arkostracker
