#pragma once

#include "../../../../song/cells/SpecialCell.h"
#include "../../../../song/SpecialCellLocationInTrack.h"

namespace arkostracker 
{

/** Holder of a SpecialCell and its location. */
class SpecialCellAndLocation
{
public:
    /** Default constructor, required by Sets. */
    SpecialCellAndLocation() noexcept;

    /**
     * Constructor.
     * @param speedTrack true if Speed Track, false if Event Track.
     * @param cell the Cell.
     * @param cellLocation the location of the Cell, in the tracks.
     */
    SpecialCellAndLocation(bool speedTrack, SpecialCell cell, SpecialCellLocationInTrack cellLocation);

    /** @return the Cell. */
    const SpecialCell& getCell() const noexcept;
    /** @return the Cell location in the Tracks. */
    const SpecialCellLocationInTrack& getCellLocation() const noexcept;

private:
    bool speedTrack;
    SpecialCell cell;
    SpecialCellLocationInTrack cellLocation;
};


}   // namespace arkostracker

