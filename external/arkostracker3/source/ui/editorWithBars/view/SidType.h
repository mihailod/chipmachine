#pragma once

#include <cstdint>

namespace arkostracker
{

/** Sid "type" for view, because the on/off and the ratio are mixed into one same bar. */
enum class SidType : std::uint8_t
{
    none,
    ratio0,
    ratio1,
    ratio2,
    ratio3,
};

}   // namespace arkostracker
