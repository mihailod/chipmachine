#include "SpecialCellLocationInPosition.h"

#include <utility>

namespace arkostracker 
{

SpecialCellLocationInPosition::SpecialCellLocationInPosition(bool pIsSpeedTrack, Id pSubsongId, int pPositionIndex, int pLineIndex) noexcept :
        speedTrack(pIsSpeedTrack),
        subsongId(std::move(pSubsongId)),
        positionIndex(pPositionIndex),
        lineIndex(pLineIndex)
{
}

bool SpecialCellLocationInPosition::isSpeedTrack() const noexcept
{
    return speedTrack;
}

Id SpecialCellLocationInPosition::getSubsongId() const noexcept
{
    return subsongId;
}

int SpecialCellLocationInPosition::getPositionIndex() const noexcept
{
    return positionIndex;
}

int SpecialCellLocationInPosition::getLineIndex() const noexcept
{
    return lineIndex;
}


}   // namespace arkostracker

