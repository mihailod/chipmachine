#include "TrackLocation.h"

namespace arkostracker
{

TrackLocation::TrackLocation(const int pPositionIndex, const int pChannelIndex) noexcept :
        positionIndex(pPositionIndex),
        channelIndex(pChannelIndex)
{
}

int TrackLocation::getPositionIndex() const noexcept
{
    return positionIndex;
}

int TrackLocation::getChannelIndex() const noexcept
{
    return channelIndex;
}

bool TrackLocation::operator==(const TrackLocation& rhs) const
{
    return positionIndex == rhs.positionIndex &&
        channelIndex == rhs.channelIndex;
}

bool TrackLocation::operator!=(const TrackLocation& rhs) const
{
    return !(*this == rhs);
}

}   // namespace arkostracker
