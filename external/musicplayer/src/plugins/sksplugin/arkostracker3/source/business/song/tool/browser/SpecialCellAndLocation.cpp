#include "SpecialCellAndLocation.h"

namespace arkostracker 
{

/** Default constructor, required by Sets. */
SpecialCellAndLocation::SpecialCellAndLocation() noexcept :
        speedTrack(),
        cell(),
        cellLocation(speedTrack, Id(), 0, 0)
{
}

SpecialCellAndLocation::SpecialCellAndLocation(bool pSpeedTrack, SpecialCell pCell, SpecialCellLocationInTrack pCellLocation) :
        speedTrack(pSpeedTrack),
        cell(pCell),
        cellLocation(std::move(pCellLocation))
{
}

const SpecialCell& SpecialCellAndLocation::getCell() const noexcept
{
    return cell;
}

const SpecialCellLocationInTrack& SpecialCellAndLocation::getCellLocation() const noexcept
{
    return cellLocation;
}

}   // namespace arkostracker

