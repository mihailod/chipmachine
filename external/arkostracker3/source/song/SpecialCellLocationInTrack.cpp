#include "SpecialCellLocationInTrack.h"

#include <utility>

namespace arkostracker 
{

SpecialCellLocationInTrack::SpecialCellLocationInTrack(bool pSpeedTrack, Id pSubsongId, int pTrackIndex, int pLineIndex) noexcept :
        subsongId(std::move(pSubsongId)),
        speedTrack(pSpeedTrack),
        trackIndex(pTrackIndex),
        lineIndex(pLineIndex)
{
}

Id SpecialCellLocationInTrack::getSubsongId() const noexcept
{
    return subsongId;
}

bool SpecialCellLocationInTrack::isSpeedTrack() const noexcept
{
    return speedTrack;
}

int SpecialCellLocationInTrack::getTrackIndex() const noexcept
{
    return trackIndex;
}

int SpecialCellLocationInTrack::getLineIndex() const noexcept
{
    return lineIndex;
}


}   // namespace arkostracker

