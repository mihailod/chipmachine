#include "ViewportUtil.h"

namespace arkostracker
{

int ViewportUtil::calculateIfScrollNeeded(const int itemPosition, const int itemSize, const int viewportPosition, const int viewportSize) noexcept
{
    const auto marginStart = itemSize;          // Empiric, to be sure we're not so close to the border.
    const auto marginEnd = itemSize * 2;        // For some reasons, works better this way.

    const auto itemEnd = itemPosition + itemSize;
    const auto viewportEnd = viewportPosition + viewportSize;
    auto viewportNewPosition = viewportPosition;

    // If the item is not fully visible, centers on it.
    if (((itemPosition - marginStart) < viewportPosition) || ((itemEnd + marginEnd) >= viewportEnd)) {
        viewportNewPosition = itemPosition - ((viewportSize - itemSize) / 2);
    }

    return viewportNewPosition;
}

}   // namespace arkostracker
