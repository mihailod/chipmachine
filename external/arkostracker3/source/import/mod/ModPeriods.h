#pragma once

#include <array>

namespace arkostracker
{

/** Holds the amige periods for MODules. */
class ModPeriods
{
public:
    /** Prevents instantiation. */
    ModPeriods() = delete;

    static constexpr auto amigaOctaveCount = 5;
    static constexpr auto amigaPeriodCount = 12 * amigaOctaveCount;
    static const std::array<int, amigaPeriodCount> amigaPeriods;            // The periods of the Amiga modules.
};

}   // namespace arkostracker
