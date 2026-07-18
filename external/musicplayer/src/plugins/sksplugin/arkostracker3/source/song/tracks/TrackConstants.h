#pragma once

namespace arkostracker 
{

/** Some constants about Tracks. */
class TrackConstants
{
public:
    /** Prevents instantiation. */
    TrackConstants() = delete;

    static const int defaultPositionHeight;

    static const int maximumCapacity;                     // Maximum Cells a Track can hold.
    static const int lastPossibleIndex;
};

}   // namespace arkostracker

