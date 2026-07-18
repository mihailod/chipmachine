#pragma once

#include "../../../model/Loop.h"

namespace arkostracker 
{

/** Generic class to optimize an item (Instrument, Expression). */
template<typename ITEM, typename CELL>
class BaseItemOptimizer
{
public:
    explicit BaseItemOptimizer(ITEM& item) noexcept :
            internalItem(item)
    {
    }

    virtual ~BaseItemOptimizer() = default;

    virtual Loop getLoop() noexcept = 0;
    virtual void setLoop(const Loop& loop) noexcept = 0;
    virtual int getLength() noexcept = 0;
    virtual int getSpeed() noexcept = 0;
    virtual void setSpeed(int speed) noexcept = 0;
    virtual void removeCells(int cellIndex, int cellCountToRemove) noexcept = 0;
    virtual CELL getCell(int cellIndex) noexcept = 0;
    virtual bool areCellsMusicallyEqual(const CELL& left, const CELL& right) noexcept = 0;

    /** Removes the past-end Cells, if any. */
    void removePastEndCells() noexcept
    {
        const auto loop = getLoop();
        const auto endIndex = loop.getEndIndex();
        const auto length = getLength();

        const auto cellCountToRemove = length - 1 - endIndex;
        removeCells(endIndex + 1, cellCountToRemove);
    }

    /** If the cells are all duplicates on X steps, increase the speed accordingly and remove the duplicates. */
    void removeDuplicateCells() noexcept
    {
        const auto loop = getLoop();
        const auto isLooping = loop.isLooping();
        const auto loopStartIndex = loop.getStartIndex();
        const auto endIndex = loop.getEndIndex();
        const auto length = getLength();
        (void)length;
        jassert(endIndex == length - 1);       // No past-end cells must be present.
        jassert(length > 0);

        // First finds a first series of duplicate.
        auto mustContinue = true;
        auto duplicatedSeriesLength = 1;
        auto cellIndex = 1;
        {
            const auto firstCell = getCell(0);
            while (mustContinue && (cellIndex <= endIndex)) {
                const auto readCell = getCell(cellIndex);
                if (areCellsMusicallyEqual(firstCell, readCell)) {
                    ++duplicatedSeriesLength;
                    ++cellIndex;
                } else {
                    mustContinue = false;
                }
            }
        }

        // Stops of nothing worth has been found.
        if (duplicatedSeriesLength <= 1) {
            return;
        }
        auto seriesCount = 1;

        // Watch out for max speed. If reached, cannot be done.
        const auto originalSpeed = getSpeed();
        constexpr auto newSeriesLength = 1;     // Always 1, since we remove duplicates.
        // To calculate the new speed, we "shift" the speed of 1 because multiplication won't work with speed at 0. Then we shift back.
        const auto newSpeed = (originalSpeed + 1) * (duplicatedSeriesLength - newSeriesLength + 1) - 1;
        if (newSpeed > 255) {
            return;
        }

        auto loopStartValidated = (loopStartIndex == 0);        // If the loop start at the beginning, there will be no problem with it.
        auto newLoopStart = 0;                                  // For now. Set for real once we are sure it is validated.

        // Checks for a possible series.
        while (cellIndex <= endIndex) {
            // Enough room for the whole sequence? If not, nothing more to do.
            if ((cellIndex + duplicatedSeriesLength) > (endIndex + 1)) {
                return;
            }

            const auto firstCell = getCell(cellIndex);
            // If not validated yet, the loop start must be at the beginning of a series.
            // We don't test if the loop is on, we don't really care at this point.
            if (!loopStartValidated) {
                loopStartValidated = (loopStartIndex == cellIndex);
                if (loopStartValidated) {
                    newLoopStart = (cellIndex / duplicatedSeriesLength);
                }
            }

            ++cellIndex;
            auto remainingCellsToFind = duplicatedSeriesLength - 1;
            while ((cellIndex <= endIndex) && (remainingCellsToFind > 0)) {
                const auto readCell = getCell(cellIndex);
                if (areCellsMusicallyEqual(firstCell, readCell)) {
                    --remainingCellsToFind;
                } else {
                    break;
                }
                ++cellIndex;
            }

            // All the cells of the series could be found? If not, nothing more to do.
            if (remainingCellsToFind != 0) {
                return;
            }
            ++seriesCount;
        }

        // One series is enough to allow a change (same cell duplicated).
        if (seriesCount < 1) {
            return;
        }
        // If looping and the loopStart isn't valid, idem.
        if (isLooping && !loopStartValidated) {
            return;
        }

        // Instead of having an illogical loop start (if used), resets it.
        if (!isLooping) {
            newLoopStart = 0;
        }

        // Now all the tests have been done, we can modify the instrument.
        setSpeed(newSpeed);

        // Removes the duplicates.
        for (auto seriesIteration = 0; seriesIteration < seriesCount; ++seriesIteration) {
            const auto iterationStartIndex = seriesIteration;
            // Removes the cells that are past the first Cell of each iteration.
            removeCells(iterationStartIndex + 1, duplicatedSeriesLength - 1);
        }

        const auto newEndIndex = seriesCount - 1;
        jassert(newEndIndex == getLength() - 1);
        setLoop(Loop(newLoopStart, newEndIndex, loop.isLooping()));
    }

    /** Removes cycles inside the loop (0,1,2,0,1,2 -> 0,1,2). */
    void removeCycles() noexcept
    {
        const auto loop = getLoop();
        if (!loop.isLooping()) {
            return;
        }
        const auto loopLength = loop.getLength();
        if (loopLength <= 1) {
            return;
        }
        const auto loopStartIndex = loop.getStartIndex();

        // Tries the smallest cycle to the largest.
        auto cycleLength = 1;
        auto foundCycle = false;
        while (cycleLength < loopLength) {
            // The cycle must be complete, else no need to even try.
            if ((loopLength % cycleLength) == 0) {
                // How many cycles inside the loop?
                const auto cycleCount = loopLength / cycleLength;       // We know it's an integer.
                jassert(cycleCount > 1);
                foundCycle = true;
                for (auto valueIndexInCycle = 0; foundCycle && (valueIndexInCycle < cycleLength); ++valueIndexInCycle) {
                    const auto firstCycleStart = loop.getStartIndex();
                    const auto& cycleValue = getCell(valueIndexInCycle + firstCycleStart);
                    // All the other cycles must have the same value.
                    for (auto cycleIndex = 1; cycleIndex < cycleCount; ++cycleIndex) {
                        const auto& nextCycleValue = getCell(valueIndexInCycle + firstCycleStart + cycleIndex * cycleLength);
                        if (!areCellsMusicallyEqual(cycleValue, nextCycleValue)) {
                            foundCycle = false;
                            break;
                        }
                    }
                }

                if (foundCycle) {
                    break;
                }
            }

            ++cycleLength;
        }

        if (!foundCycle) {
            return;
        }

        const auto endIndex = loopStartIndex + cycleLength - 1;
        setLoop(Loop(loopStartIndex, endIndex, true));

        // We can remove the unused items at the end.
        removeCells(endIndex + 1, getLength() - (endIndex + 1));
    }

    /** Includes the data before the loop, if the same. (0,1,2,3,[4,3] -> 0,1,2,[3,4]). */
    void optimizePreLoop() noexcept
    {
        const auto originalLoop = getLoop();
        if (!originalLoop.isLooping()) {
            return;
        }

        // Loops as long as we can optimize.
        auto success = true;
        while (success) {
            const auto loop = getLoop();
            const auto loopStartIndex = loop.getStartIndex();
            const auto loopEndIndex = loop.getEndIndex();

            const auto preLoopStartIndex = loopStartIndex - 1;
            if (preLoopStartIndex < 0) {
                break;     // Not enough room before the loop, no cycle is possible.
            }

            // Is the cell just before the loop is the same as the end of the loop?
            const auto left = getCell(preLoopStartIndex);
            const auto right = getCell(loopEndIndex);
            if (!areCellsMusicallyEqual(left, right)) {
                success = false;
            } else {
                // Removes the last item, shifts the loop.
                removeCells(loopEndIndex, 1);
                setLoop(Loop(preLoopStartIndex, loopEndIndex - 1, true));
            }
        }
    }

    /** @return the item. */
    ITEM& getItem() const noexcept
    {
        return internalItem;
    }

private:
    ITEM& internalItem;
};

}   // namespace arkostracker

