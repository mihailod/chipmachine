#pragma once

#include "../utils/Id.h"

namespace arkostracker 
{

/** Locates a line in a Subsong. */
class Location
{
public:
    /**
     * Constructor.
     * @param subsongId the ID of the Subsong.
     * @param position the position.
     * @param line the line in the pattern. If <0, corrected.
     */
    Location(Id subsongId, int position, int line = 0) noexcept;

    /** Default constructor with 0 values, should not be used except to use for OptionalValue, which requires a default constructor. */
    Location() noexcept;

    /**
     * @return a copy of this object, with a new line.
     * @param line the new line. If <0, it is corrected.
     */
    Location withLine(int line = 0) const noexcept;

    /**
     * @return a copy of this object, with a new position.
     * @param position the new position.
     */
    Location withPosition(int position) const noexcept;

    /**
     * @return true if the other Location has the same Position AND Subsong.
     * @param other the other Location.
     */
    bool hasSamePosition(const Location& other) const noexcept;

    /** @return the Subsong ID. */
    Id getSubsongId() const noexcept;
    /** @return the position. */
    int getPosition() const noexcept;
    /** @return the line. */
    int getLine() const noexcept;

    bool operator==(const Location& rhs) const;             // NOLINT(fuchsia-overloaded-operator)
    bool operator!=(const Location& rhs) const;             // NOLINT(fuchsia-overloaded-operator)
    bool operator<(const Location& rhs) const;

private:
    Id subsongId;
    int position;
    int line;
};

}   // namespace arkostracker

