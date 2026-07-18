#pragma once

namespace arkostracker 
{

class Song;
class Location;

/** Class that determines the speed at a specific location. */
class DetermineSpeed
{
public:
    /**
     * @return the speed at the given location. If it couldn't be found (out of bounds?), returns the latest known value and asserts.
     * @param song the Song.
     * @param location the Location.
     */
    static int determineSpeed(const Song& song, const Location& location) noexcept;
};

}   // namespace arkostracker
