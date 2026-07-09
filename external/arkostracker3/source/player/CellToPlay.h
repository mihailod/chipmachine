#pragma once

#include "../song/CellLocationInPosition.h"
#include "../song/cells/Cell.h"

namespace arkostracker
{

/**
 * Represents a Cell to play (for example, a Cell from the test area, or from the PV triggered by the user).
 * It can also have a location for the Effect Context feature.
 */
class CellToPlay
{
public:
    CellToPlay(Cell pCell, const int pChannelIndex, const OptionalValue<CellLocationInPosition>& pLocation) noexcept :
            cell(std::move(pCell)),
            channelIndex(pChannelIndex),
            location(pLocation)
    {
    }

    const Cell& getCell() const noexcept;
    int getChannelIndex() const noexcept;
    const OptionalValue<CellLocationInPosition>& getLocation() const noexcept;

private:
    Cell cell;
    int channelIndex;
    OptionalValue<CellLocationInPosition> location;
};

}   // namespace arkostracker
