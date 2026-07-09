#include "ModPeriods.h"

namespace arkostracker
{

// The periods MUST be sorted from the lowest to highest!
const std::array<int, static_cast<std::size_t>(ModPeriods::amigaPeriodCount)> ModPeriods::amigaPeriods = {
    1712, 1616, 1525, 1440, 1357, 1281, 1209, 1141, 1077, 1017, 961, 907,       // Octave 0 - NOT standard.
    856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
    428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
    214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
    107, 101,  95,  90,  85,  80,  76,  71,  67,  64,  60,  57                  // Octave 4 - NOT standard.
};

}   // namespace arkostracker
