#pragma once

#include "../utils/Id.h"

namespace arkostracker 
{

/** The location of a SpecialCell, in a Track on a Subsong. */
class SpecialCellLocationInTrack
{
public:
    /**
     * Constructor.
     * @param speedTrack true if Speed Track, false if Event Track.
     * @param subsongId the Subsong id. Must be valid.
     * @param trackIndex the track index. Must be valid.
     * @param lineIndex the line index in the Track. Must be valid.
     */
    SpecialCellLocationInTrack(bool speedTrack, Id subsongId, int trackIndex, int lineIndex) noexcept;

    Id getSubsongId() const noexcept;
    bool isSpeedTrack() const noexcept;
    int getTrackIndex() const noexcept;
    int getLineIndex() const noexcept;

private:
    Id subsongId;
    bool speedTrack;
    int trackIndex;
    int lineIndex;
};

}   // namespace arkostracker

