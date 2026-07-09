#include "Location.h"

#include <juce_core/juce_core.h>

#include <utility>

namespace arkostracker 
{

Location::Location(Id pSubsongId, const int pPosition, const int pLine) noexcept :
        subsongId(std::move(pSubsongId)),
        position(pPosition),
        line(pLine >= 0 ? pLine : 0)
{
}

Location::Location() noexcept :
        subsongId(),
        position(0),
        line(0)
{
}

Id Location::getSubsongId() const noexcept
{
    return subsongId;
}

int Location::getPosition() const noexcept
{
    return position;
}

int Location::getLine() const noexcept
{
    return line;
}

bool Location::hasSamePosition(const Location& other) const noexcept
{
    return (subsongId == other.subsongId)
        && (position == other.position);
}

bool Location::operator==(const Location& rhs) const            // NOLINT(fuchsia-overloaded-operator)
{
    return (subsongId == rhs.subsongId)
            && (position == rhs.position)
            && (line == rhs.line);
}

bool Location::operator!=(const Location& rhs) const            // NOLINT(fuchsia-overloaded-operator)
{
    return !(rhs == *this);
}

bool Location::operator<(const Location& rhs) const
{
    jassert(subsongId == rhs.subsongId);                  // Else, doesn't make any sense!

    // If same position, compares the line.
    if (position == rhs.position) {
        return (line < rhs.line);
    }

    // Else, the position comparison is enough.
    return (position < rhs.position);
}

Location Location::withLine(int newLine) const noexcept
{
    if (newLine < 0) {
        newLine = 0;
    }
    return { subsongId, position, newLine };
}

Location Location::withPosition(int newPosition) const noexcept
{
    return { subsongId, newPosition, line };
}


}   // namespace arkostracker

