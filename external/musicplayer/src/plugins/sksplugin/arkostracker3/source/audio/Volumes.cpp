#include "Volumes.h"

namespace arkostracker
{

const std::array<uint16_t, Volumes::volumeCount> Volumes::baseVolumesAy = {
    0U, 0U, 231U, 231U, 695U, 695U, 1158U, 1158U, 2084U, 2084U, 2779U, 2779U, 4168U, 4168U,       // Doubled to match YM volume count.
    6716U, 6716U, 8105U, 8105U, 13200U, 13200U, 18294U, 18294U, 24315U, 24315U, 32189U, 32189U,
    40757U, 40757U, 52799U, 52799U, 65535U, 65535U
};

const std::array<uint16_t, Volumes::volumeCount> Volumes::baseVolumesYm = {
    0U, 369U, 438U, 521U, 619U, 735U, 874U, 1039U, 1234U, 1467U, 1744U, 2072U, 2463U, 2927U,      // From Benjamin Gerard in SC68.
    3479U, 4135U, 4914U, 5841U, 6942U, 8250U, 9806U, 11654U, 13851U, 16462U,
    19565U, 23253U, 27636U, 32845U, 39037U, 46395U, 55141U, 65535U
};

}   // namespace arkostracker
