#pragma once

namespace arkostracker
{

/** How much to transpose. */
enum class TransposeRate : unsigned char
{
    upNormal,
    upFast,

    downNormal,
    downFast,
};

}   // namespace arkostracker
