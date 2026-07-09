#pragma once

#include "../../utils/OptionalValue.h"

namespace arkostracker 
{

/** Represents a start/end index, and a flag if looping. */
class Loop
{
public:
    /**
     * Constructor.
     * @param startIndex the start index (>=0).
     * @param endIndex the end index. Must be >= startIndex.
     * @param looping true if looping.
     */
    explicit Loop(int startIndex = 0, int endIndex = 0, bool looping = false) noexcept;

    /** @return the start index (>=0). */
    int getStartIndex() const noexcept;
    /** @return the end index (>=0). */
    int getEndIndex() const noexcept;

    /** @return true if looping. */
    bool isLooping() const noexcept;

    /** @return the length of the loop (>0). */
    int getLength() const noexcept;

    /**
     * @return a copy of the object, with a new looping value.
     * @param looping the new looping value.
     */
    Loop withLooping(bool looping) const noexcept;

    /**
     * @return a copy of the object, with a new start value.
     * @param start the new start value.
     */
    Loop withStart(int start) const noexcept;

    /**
     * @return a copy of the object, with a new end value.
     * @param end the new end value.
     */
    Loop withEnd(int end) const noexcept;

    /**
     * @return a copy of the object, with a new start and/or a new end.
     * @param start the possible start.
     * @param end the possible end.
     */
    Loop withPossibleStartAndEnd(OptionalInt start, OptionalInt end) const noexcept;

    bool operator==(const Loop& rhs) const noexcept;
    bool operator!=(const Loop& rhs) const noexcept;

private:
    int startIndex;
    int endIndex;
    bool looping;
};

}   // namespace arkostracker
