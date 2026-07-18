#include "CellAndLocation.h"

namespace arkostracker
{

/** Default constructor, required by Sets. */
CellAndLocation::CellAndLocation() noexcept :
        cell(),
        cellLocation(Id(), 0, 0)
{
}

CellAndLocation::CellAndLocation(Cell pCell, CellLocationInTrack pCellLocation) :
        cell(std::move(pCell)),
        cellLocation(std::move(pCellLocation))
{
}

const Cell& CellAndLocation::getCell() const noexcept
{
    return cell;
}

const CellLocationInTrack& CellAndLocation::getCellLocation() const noexcept
{
    return cellLocation;
}

}   // namespace arkostracker
