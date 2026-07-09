#pragma once

#include <utility>

namespace arkostracker
{

/** Calculates the periods for ST SIDs. */
class StSidPeriodCalculator
{
public:
    /** Prevents instantiation. */
    StSidPeriodCalculator() = delete;

    /** @return the Timer Control (3 bits) and Timer Data (8 bits). */
    static std::pair<int, int> calculate(int period) noexcept;
};

}   // namespace arkostracker
