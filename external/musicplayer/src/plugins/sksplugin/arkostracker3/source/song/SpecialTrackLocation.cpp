#include "SpecialTrackLocation.h"

namespace arkostracker
{

SpecialTrackLocation::SpecialTrackLocation(const bool pIsSpeedTrack, const int pPositionIndex) noexcept :
        speedTrack(pIsSpeedTrack),
        positionIndex(pPositionIndex)
{
}

int SpecialTrackLocation::getPositionIndex() const noexcept
{
    return positionIndex;
}

bool SpecialTrackLocation::isSpeedTrack() const noexcept
{
    return speedTrack;
}

bool SpecialTrackLocation::operator==(const SpecialTrackLocation& rhs) const
{
    return positionIndex == rhs.positionIndex &&
        speedTrack == rhs.speedTrack;
}

bool SpecialTrackLocation::operator!=(const SpecialTrackLocation& rhs) const
{
    return !(*this == rhs);
}

}   // namespace arkostracker
