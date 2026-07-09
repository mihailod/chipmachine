#pragma once

#include <utility>

#include "../../utils/OptionalValue.h"
#include "view/cells/cell/CellCursorRank.h"
#include "view/cells/simpleCell/SpecialCellCursorRank.h"
#include "view/tracks/TrackType.h"

namespace arkostracker 
{

/** Locates the cursor "horizontally", on a line. */
class CursorLocation
{
public:
    /** Default constructor, at the beginning of the first channel. */
    CursorLocation() noexcept;

    /**
     * Creates a location on a "music" track.
     * @param channelIndex the index of the channel (>=0).
     * @param rank the rank.
     * @return the location.
     */
    static CursorLocation onTrack(int channelIndex, CellCursorRank rank) noexcept;

    /**
     * Creates a location on a "Speed" track.
     * @param rank the rank.
     * @return the location.
     */
    static CursorLocation onSpeedTrack(SpecialCellCursorRank rank) noexcept;

    /**
     * Creates a location on an "Event" track.
     * @param rank the rank.
     * @return the location.
     */
    static CursorLocation onEventTrack(SpecialCellCursorRank rank) noexcept;

    /**
     * Creates a location on a "Line" track.
     * @return the location.
     */
    static CursorLocation onLineTrack() noexcept;

    /**
     * Creates a location on a "Special" track.
     * @param isSpeedTrack true is Speed Track, false if Event Track.
     * @param rank the rank.
     * @return the location.
     */
    static CursorLocation onSpecialTrack(bool isSpeedTrack, SpecialCellCursorRank rank) noexcept;

    /** @return the type of the track. */
    TrackType getTrackType() const noexcept;
    /** @return the index of the channel. Must be 0 for non-music Track. */
    int getChannelIndex() const noexcept;
    /** @return the Rank (to be cast into CellCursorRank, or SpecialCellCursorRank). Better use getRanksAs... if you know where you are! */
    int getRank() const noexcept;
    /** @return the CellCursorRank. This must have been checked before! */
    CellCursorRank getRankAsCellCursorRank() const noexcept;
    /** @return the SpecialCellCursorRank. This must have been checked before! */
    SpecialCellCursorRank getRankAsSpecialCursorRank() const noexcept;

    /**
     * @return the effect index (0 (left), 1, 2, 3) on which the cursor is, or absent. True if the cursor is on the effect, false if on a value.
     * If on Special Track, also returns absent.
     */
    std::pair<OptionalInt, bool> getEffectIndexIfRankOnEffect() const noexcept;

    /** @return true if the cursor if on a music track. False if on an event track. */
    bool isMusicTrack() const noexcept;

    /** @return the channel index and the Rank if the cursor is on a "normal" Track. */
    OptionalValue<std::pair<int, CellCursorRank>> getChannelIndexAndRankIfOnTrack() const noexcept;

    /**
     * @return a copy of this, with the cursor going back to the effect number if it was on an effect value.
     * Useful when going from maximize to minimize, as these digits aren't shown. A change only appears for "music" type.
     */
    CursorLocation withEffectCorrection() const noexcept;

    /**
     * @return a copy of this, with the cursor going back to left or right to areas (the effect number if it was on an effect value, first digit on an instrument).
     * It can go to the left, or the right (useful for start/end of a selection).
     */
    CursorLocation withAreaCorrection(bool goToLeft) const noexcept;

    bool operator==(const CursorLocation& rhs) const;
    bool operator!=(const CursorLocation& rhs) const;
    bool operator<(const CursorLocation& rhs) const;

private:
    /**
     * Constructor.
     * @param trackType the type of track where the cursor is.
     * @param index always 0 for special Track, else 0-2, etc.
     * @param rank the rank, to be cast into CellCursorRank, or SpecialCursorRank.
     */
    CursorLocation(TrackType trackType, int index, int rank) noexcept;

    TrackType trackType;            // The type of track where the cursor is.
    int index;                      // Is always 0 for special Track, else 0-2, etc.
    int rank;                       // The rank, to be cast into CellCursorRank, or SpecialCursorRank.
};

}   // namespace arkostracker
