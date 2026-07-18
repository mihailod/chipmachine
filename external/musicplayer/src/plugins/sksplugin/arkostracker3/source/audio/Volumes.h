#pragma once

#include <array>
#include <cstdint>

namespace arkostracker
{

/** Holds the volume table for AY/YM PSGs. */
class Volumes
{
public:
    /** Prevents instantiation. */
    Volumes() = delete;

    static constexpr auto volumeCount = 32U;                      // How many volumes are available for the PSG. Doubled to manage both AY/YM.
    static constexpr auto maximumValue = 65535;                  // The maximum value the tables can hold.

    static const std::array<uint16_t, volumeCount> baseVolumesAy;       // Volume table for the AY, from 0 to 65535.
    static const std::array<uint16_t, volumeCount> baseVolumesYm;       // Volume table for the YM, from 0 to 65535.
};

}   // namespace arkostracker
