#pragma once

#include <map>
#include <unordered_set>

#include "../../../../utils/OptionalValue.h"
#include "../../../../song/tracks/SpecialTrack.h"
#include "../../../../song/tracks/Track.h"

namespace arkostracker 
{

class Pattern;

/**
 * Helps "unoptimize" Patterns. That is, find any duplicate Track/Special Track **indexes** and create new duplicated Tracks and make the Pattern refer to them.
 * The tracks referenced in the Patterns MUST exist.
 */
class PatternUnoptimizer
{
public:
    /** Prevents instantiation. */
    ~PatternUnoptimizer() = delete;

    /**
     * Modifies the Pattern so that each Pattern has only one instance of the each Track/Special Track.
     * The pattern will be modified, and new Tracks may be added to all the related maps.
     * @param indexToPattern the index to each Pattern. The Patterns be modified.
     * @param indexToTrack links an index to a Track. May be modified (new entries).
     * @param indexToSpeedTrack links an index to a Speed Track. May be modified (new entries).
     * @param indexToEventTrack links an index to a Event Track. May be modified (new entries).
     */
    static void unoptimize(std::map<int, Pattern>& indexToPattern, std::map<int, Track>& indexToTrack,
                           std::map<int, SpecialTrack>& indexToSpeedTrack, std::map<int, SpecialTrack>& indexToEventTrack) noexcept;

private:
    /**
     * Unoptimizes the Special Tracks.
     * @param isSpeedTrack true if Speed Tracks, false if Event Tracks.
     * @param indexToSpecialTrack links an index to a SpecialTrack. May be modified (new entries).
     * @param browsedSpecialTrackIndexes set indicating what special Track has already been browsed.
     * @param originalPattern the pattern.
     * @return if present, the index of the new special Track.
     */
    static OptionalInt manageSpecialTracks(bool isSpeedTrack, std::map<int, SpecialTrack>& indexToSpecialTrack, std::unordered_set<int>& browsedSpecialTrackIndexes,
                                           const Pattern& originalPattern);
};

}   // namespace arkostracker
