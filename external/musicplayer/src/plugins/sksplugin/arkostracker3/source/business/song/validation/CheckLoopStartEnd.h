#pragma once

#include <utility>

#include "../../../utils/OptionalValue.h"
#include "../../model/Loop.h"

namespace arkostracker 
{

/** Object to check and correct the loop start and end. */              // TODO TU this.
class CheckLoopStartEnd
{
public:
    /** Prevents instantiation. */
    CheckLoopStartEnd() = delete;

    /**
     * @return a corrected loop, from the given length. Contrary to the other method, the start is stuck between 0 and the end.
     * @param loop the loop.
     * @param length the length of the item.
     */
    static Loop checkLoopStartEnd(const Loop& loop, const int length) noexcept;

    /**
     * Corrects the loop start and end, from the length. Contrary to the other method, the start is stuck between 0 and the end.
     * @param loopStartIndex the loop start index.
     * @param endIndex the end index.
     * @param length the length. Must be >0.
     * @return the corrected loop start/end.
     */
    static std::pair<int, int> checkLoopStartEnd(int loopStartIndex, int endIndex, int length) noexcept;

    /**
     * Corrects the loop start and end. At least one new start or end must be given. If not, nothing happens, as a convenience.
     * This method is useful to set the new start after the end, so that the end is changed accordingly.
     * @param currentLoopStartPosition the current loop start. Must be valid.
     * @param currentEndPosition the current end start. Must be valid.
     * @param length the length of the song. Must be valid.
     * @param optionalNewLoopStartPosition the possible new loop start.
     * @param optionalNewEndPosition the possible new end position.
     * @return the corrected loop start/end.
     */
    static std::pair<int, int> checkLoopStartEnd(int currentLoopStartPosition, int currentEndPosition, int length,
                           OptionalInt optionalNewLoopStartPosition, OptionalInt optionalNewEndPosition) noexcept;
};

}   // namespace arkostracker
