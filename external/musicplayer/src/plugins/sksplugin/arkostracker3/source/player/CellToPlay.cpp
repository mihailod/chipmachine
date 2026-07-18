#include "CellToPlay.h"

namespace arkostracker
{

const Cell& CellToPlay::getCell() const noexcept
{
    return cell;
}

int CellToPlay::getChannelIndex() const noexcept
{
    return channelIndex;
}

const OptionalValue<CellLocationInPosition>& CellToPlay::getLocation() const noexcept
{
    return location;
}

}   // namespace arkostracker
