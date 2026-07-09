#pragma once

#include <utility>

#include "../../../../utils/OptionalValue.h"
#include "../../../../song/Location.h"
#include "../../../../song/tracks/SpecialTrack.h"

namespace arkostracker 
{

class Song;

/**  Counts how many frames there are in a Song, and/or up to a certain location in the Song. */
class FrameCounter
{
public:
    /** Prevents instantiation. */
    ~FrameCounter() = delete;

    /**
     * @return how many frames there are up to the end of the song, and up to the given location (same value as First if not given).
     * The third is the LoopTo counter frame.
     * The Subsong is locked during the operation.
     * @param song the Song.
     * @param subsongId the subsong ID. It must be the same as the location, if provided!
     * @param upToExcluded the location up to where count (excluded).
     */
    static std::tuple<int, int, int> count(const Song& song, const Id& subsongId, const OptionalValue<Location>& upToExcluded = { }) noexcept;

private:
    /**
     * @return how many frames there are in this Special Track. The current speed IS modified if a speed is found.
     * @param speedTrack the Speed Track.
     * @param currentSpeed the current speed.
     * @param positionHeight the position height, or the (excluded) line to reach.
     */
    static int countFramesInSpeedTrack(const SpecialTrack& speedTrack, int& currentSpeed, int positionHeight) noexcept;
};

}   // namespace arkostracker
