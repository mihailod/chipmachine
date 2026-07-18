#pragma once

#include "../../model/Loop.h"

namespace arkostracker 
{

/** Object to correct a loop when an insertion/deletion is done before/inside/after. */
class LoopChange
{
public:
    /** Prevents instantiation. */
    LoopChange() = delete;

    /**
     * @return the new loop after a cell is removed. Asserts if the length is 1 from the start, and returns the same loop.
     * @param cellIndex where to insert.
     * @param originalLoop the original loop.
     * @param originalLength the original length of the item, so shift the loop if necessary.
     */
    static Loop remove(int cellIndex, const Loop& originalLoop, int originalLength) noexcept;

    /**
     * @return the new loop after the cells are removed. Asserts if the length is 1 from the start, and returns the same loop. If the length falls to 1, ends.
     * @param cellIndexes the cell indexes to remove.
     * @param originalLoop the original loop.
     * @param originalLength the original length of the item, to shift the loop if necessary.
     */
    static Loop remove(const std::set<int>& cellIndexes, const Loop& originalLoop, int originalLength) noexcept;

    /**
     * @return the new loop after a cell is inserted.
     * @param cellIndex where to insert.
     * @param originalLoop the original loop.
     */
    static Loop insert(int cellIndex, const Loop& originalLoop) noexcept;

    /**
     * @return the new loop after the cell are inserted.
     * @param cellIndexes where to insert.
     * @param originalLoop the original loop.
     */
    static Loop insert(const std::set<int>& cellIndexes, const Loop& originalLoop) noexcept;
};

}   // namespace arkostracker
