#pragma once

#include "../utils/Id.h"

namespace arkostracker
{

/** The location of a Cell, in a Track on a Subsong. */
class CellLocationInTrack
{
public:
    /**
     * Constructor.
     * @param subsongId the Subsong id. Must be valid.
     * @param trackIndex the track index. Must be valid.
     * @param lineIndex the line index in the Track. Must be valid.
     */
    CellLocationInTrack(Id subsongId, int trackIndex, int lineIndex) noexcept;

    Id getSubsongId() const noexcept;
    int getTrackIndex() const noexcept;
    int getLineIndex() const noexcept;

private:
    Id subsongId;
    int trackIndex;
    int lineIndex;
};

}   // namespace arkostracker
