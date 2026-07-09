#include "Loop.h"

#include <juce_core/juce_core.h>

namespace arkostracker 
{

Loop::Loop(const int pStartIndex, const int pEndIndex, const bool pLooping) noexcept :
        startIndex(pStartIndex),
        endIndex(pEndIndex),
        looping(pLooping)
{
    jassert(startIndex >= 0);
    jassert(endIndex >= startIndex);
}

int Loop::getStartIndex() const noexcept
{
    return startIndex;
}

int Loop::getEndIndex() const noexcept
{
    return endIndex;
}

bool Loop::isLooping() const noexcept
{
    return looping;
}

int Loop::getLength() const noexcept
{
    return endIndex - startIndex + 1;
}

Loop Loop::withLooping(const bool newLooping) const noexcept
{
    return Loop(startIndex, endIndex, newLooping);
}

Loop Loop::withStart(const int newStart) const noexcept
{
    return Loop(newStart, endIndex, looping);
}

Loop Loop::withEnd(const int newEnd) const noexcept
{
    return Loop(startIndex, newEnd, looping);
}

Loop Loop::withPossibleStartAndEnd(const OptionalInt start, const OptionalInt end) const noexcept
{
    return Loop(start.isPresent() ? start.getValue() : startIndex,
                end.isPresent() ? end.getValue() : endIndex,
                looping);
}

bool Loop::operator==(const Loop& rhs) const noexcept
{
    return startIndex == rhs.startIndex &&
           endIndex == rhs.endIndex &&
           looping == rhs.looping;
}

bool Loop::operator!=(const Loop& rhs) const noexcept
{
    return !(rhs == *this);
}


}   // namespace arkostracker

