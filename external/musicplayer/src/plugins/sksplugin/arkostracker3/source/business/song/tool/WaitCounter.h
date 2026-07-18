#pragma once

#include "../../../utils/OccurrenceMap.h"

namespace arkostracker 
{

class Id;
class Song;

/**
 * Class to count all the waits from a Song, allowing to know which are the most used for example.
 * The end of the Tracks are not accounted for.
 */
class WaitCounter
{
public:
    /** Prevents instantiation. */
    WaitCounter() = delete;

    /**
     * @return an occurrence map linking a wait to how many times it was used.
     * @param song the Song.
     * @param subsongId the ID of the Subsong.
     */
    static OccurrenceMap<int> countWait(const Song& song, const Id& subsongId) noexcept;
};

}   // namespace arkostracker
