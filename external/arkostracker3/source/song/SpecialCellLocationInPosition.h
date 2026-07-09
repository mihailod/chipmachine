#pragma once

#include "../utils/Id.h"

namespace arkostracker 
{

/** The location of a SpecialCell, in a Position on a Subsong. */
class SpecialCellLocationInPosition
{
public:
    /**
     * Constructor.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param subsongId the Subsong id. Must be valid.
     * @param positionIndex the position index. Must be valid.
     * @param lineIndex the line index in the Position. Must be valid.
     */
    SpecialCellLocationInPosition(bool isSpeedTrack, Id subsongId, int positionIndex, int lineIndex) noexcept;

    bool isSpeedTrack() const noexcept;
    Id getSubsongId() const noexcept;
    int getPositionIndex() const noexcept;
    int getLineIndex() const noexcept;

private:
    bool speedTrack;
    Id subsongId;
    int positionIndex;
    int lineIndex;
};

}   // namespace arkostracker

