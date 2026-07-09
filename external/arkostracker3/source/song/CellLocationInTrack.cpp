#include "CellLocationInTrack.h"

namespace arkostracker
{

CellLocationInTrack::CellLocationInTrack(Id pSubsongId, const int pTrackIndex, const int pLineIndex) noexcept :
        subsongId(std::move(pSubsongId)),
        trackIndex(pTrackIndex),
        lineIndex(pLineIndex)
{
}

Id CellLocationInTrack::getSubsongId() const noexcept
{
    return subsongId;
}

int CellLocationInTrack::getTrackIndex() const noexcept
{
    return trackIndex;
}

int CellLocationInTrack::getLineIndex() const noexcept
{
    return lineIndex;
}

}   // namespace arkostracker
