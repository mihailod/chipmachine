#pragma once

#include "../../utils/OptionalValue.h"

namespace arkostracker 
{

/** A width or height, with a desired size and a minimum and maximum. */
class Dimension
{
public:
    static const int minimumValue;

    /**
     * Constructor for a possible desired size, and a min and max.
     * @param desiredSize the possible desired dimension.
     * @param minimum the minimum.
     * @param maximum the maximum.
     */
    explicit Dimension(const OptionalInt& desiredSize, int minimum = minimumValue, int maximum = 9999) noexcept;

    /** Builds a dimension without desired size. */
    static Dimension buildFree() noexcept;

    /** Builds a locked dimension. */
    static Dimension buildLocked(int height) noexcept;

    /** @return the minimum. */
    int getMinimum() const noexcept;
    /** @return the maximum. */
    int getMaximum() const noexcept;
    /** @return the desired size, if any. */
    const OptionalInt& getDesiredSize() const noexcept;

    /** @return true if the min/max/desired size are the same. */
    bool isLocked() const noexcept;

    bool operator==(const Dimension& rhs) const;
    bool operator!=(const Dimension& rhs) const;

private:
    int minimum;
    int maximum;
    OptionalInt desiredSize;
};


}   // namespace arkostracker

