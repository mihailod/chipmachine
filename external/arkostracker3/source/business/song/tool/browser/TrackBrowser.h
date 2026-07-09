#pragma once

#include <set>
#include <vector>

#include "../../../../song/TrackLocation.h"
#include "../../../../song/subsong/LinkType.h"

namespace arkostracker
{

class Id;
class Song;

/** Helper to find specific Tracks. */
class TrackBrowser
{
public:
    /** Prevents instantiation. */
    TrackBrowser() = delete;

    /**
     * @return all the used Tracks from a Subsong, in the order they are found, but each unique.
     * The Positions are browsed, so Tracks from unused Positions are not taken in account.
     * @param song the Song.
     * @param subsongId the ID of the Subsong. Must be valid.
     * @param stopAtLoopEnd true to stop at the loop end, false to continue till the end.
     */
    static std::vector<int> getAllUsedTrackIndexes(const Song& song, const Id& subsongId, bool stopAtLoopEnd) noexcept;

    /**
     * @return all the Tracks from a Subsong that have a name, in the order they are found.
     * @param song the Song.
     * @param subsongId the ID of the Subsong. Must be valid.
     */
    static std::set<int> getAllNamedTrackIndexes(const Song& song, const Id& subsongId) noexcept;

    /**
     * @return where a Track is used in the current Position. By default, only the visible Tracks are reported (Linked have priority).
     * @param song the Song.
     * @param subsongId the subsong ID.
     * @param trackIndex the track index.
     * @param linkType what link to look for.
     */
    static std::vector<TrackLocation> findTrackUsage(const Song& song, const Id& subsongId, int trackIndex, LinkType linkType = LinkType::unlinkedIfLinkedNotPresent) noexcept;
};

}   // namespace arkostracker
