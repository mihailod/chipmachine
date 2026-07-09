#include "CellLocationInPosition.h"

namespace arkostracker
{
CellLocationInPosition::CellLocationInPosition() noexcept : subsongId(Id()),
                                                            positionIndex(-1),
                                                            lineIndex(-1),
                                                            channelIndex(-1)
{
}

CellLocationInPosition::CellLocationInPosition(Id pSubsongId, const int pPositionIndex, const int pLineIndex, const int pChannelIndex) noexcept : subsongId(std::move(pSubsongId)),
    positionIndex(pPositionIndex),
    lineIndex(pLineIndex),
    channelIndex(pChannelIndex)
{
}

Id CellLocationInPosition::getSubsongId() const noexcept
{
    return subsongId;
}

int CellLocationInPosition::getPositionIndex() const noexcept
{
    return positionIndex;
}

int CellLocationInPosition::getLineIndex() const noexcept
{
    return lineIndex;
}

int CellLocationInPosition::getChannelIndex() const noexcept
{
    return channelIndex;
}

bool CellLocationInPosition::operator==(const CellLocationInPosition& other) const
{
    return subsongId == other.subsongId
           && positionIndex == other.positionIndex
           && lineIndex == other.lineIndex
           && channelIndex == other.channelIndex;
}

bool CellLocationInPosition::operator!=(const CellLocationInPosition& other) const
{
    return !(*this == other);
}

} // namespace arkostracker
