#pragma once

#include <set>
#include <vector>

#include "../../../../song/SpecialTrackLocation.h"
#include "../../../../song/subsong/LinkType.h"

namespace arkostracker
{

class Id;
class Song;

/** Helper to find specific Tracks. */
class SpecialTrackBrowser
{
public:
    /** Prevents instantiation. */
    SpecialTrackBrowser() = delete;

    /**
     * @return all the used Special Tracks from a Subsong, in the order they are found, but each unique.
     * @param song the Song.
     * @param subsongId the ID of the Subsong. Must be valid.
     * @param isSpeedTrack true to target the Speed Tracks, false for Event Tracks.
     * @param stopAtLoopEnd true to stop at the loop end, false to continue till the end.
     */
    static std::vector<int> getAllUsedSpecialTrackIndexes(const Song& song, const Id& subsongId, bool isSpeedTrack, bool stopAtLoopEnd) noexcept;

    /**
     * @return all the Special Tracks from a Subsong that have a name, in the order they are found.
     * @param song the Song.
     * @param subsongId the ID of the Subsong. Must be valid.
     * @param isSpeedTrack true to target the Speed Tracks, false for Event Tracks.
     */
    static std::set<int> getAllNamedSpecialTrackIndexes(const Song& song, const Id& subsongId, bool isSpeedTrack) noexcept;

    /**
     * @return where a Special Track is used in the current Position. By default, only the visible Special Tracks are reported (Linked have priority).
     * @param song the Song.
     * @param subsongId the subsong ID.
     * @param isSpeedTrack true to target the Speed Tracks, false for Event Tracks.
     * @param specialTrackIndex the special track index.
     * @param linkType what link to look for.
     */
    static std::vector<SpecialTrackLocation> findSpecialTrackUsage(const Song& song, const Id& subsongId, bool isSpeedTrack, int specialTrackIndex,
        LinkType linkType = LinkType::unlinkedIfLinkedNotPresent) noexcept;
};

}   // namespace arkostracker
