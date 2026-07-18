#pragma once

#include "../../../../song/CellLocationInTrack.h"
#include "../../../../song/cells/Cell.h"

namespace arkostracker
{

/** Holder of a Cell and its location. */
class CellAndLocation
{
public:
    /** Default constructor, required by Sets. */
    CellAndLocation() noexcept;

    /**
     * Constructor.
     * @param cell the Cell.
     * @param cellLocation the location of the Cell, in the tracks.
     */
    CellAndLocation(Cell cell, CellLocationInTrack cellLocation);

    /** @return the Cell. */
    const Cell& getCell() const noexcept;
    /** @return the Cell location in the Tracks. */
    const CellLocationInTrack& getCellLocation() const noexcept;

private:
    Cell cell;
    CellLocationInTrack cellLocation;
};

}   // namespace arkostracker
