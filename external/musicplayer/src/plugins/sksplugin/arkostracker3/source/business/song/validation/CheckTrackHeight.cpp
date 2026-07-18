#include "CheckTrackHeight.h"

#include "../../../song/subsong/Position.h"
#include "../../../utils/NumberUtil.h"

namespace arkostracker 
{

int CheckTrackHeight::correctHeight(int trackHeight) noexcept
{
    const auto minHeight = Position::minimumPositionHeight;
    const auto maxHeight = Position::maximumPositionHeight;
    return NumberUtil::correctNumber(trackHeight, minHeight, maxHeight);
}


}   // namespace arkostracker

