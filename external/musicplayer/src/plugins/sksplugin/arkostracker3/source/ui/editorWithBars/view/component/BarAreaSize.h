#pragma once

namespace arkostracker 
{

/** The size for each bar area. */
enum class BarAreaSize : int
{
    // Values MUST be contiguous!

    first = 0,

    small = first,              // Important to start at 0, a comparison is made. A static assert assures that.
    /** Should be enough for most of the values. */
    normal,
    /** Unusual values, but not all. */
    extended,
    /** Used to show more than usual values (rare hardware envelope for example). */
    allUnusual,

    last = allUnusual,
};

}   // namespace arkostracker
