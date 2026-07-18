#pragma once

namespace arkostracker 
{

/** The type of a Track (normal, special, etc.). */
enum class TrackType : unsigned char
{
    // There are stored in ORDER, from the left to right. This simplifies the comparison.
    // Do NOT modify the order!

    line,
    /** Or "music" type. */
    normal,
    speed,
    event
};

}   // namespace arkostracker
