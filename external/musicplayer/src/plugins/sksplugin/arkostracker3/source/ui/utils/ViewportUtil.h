#pragma once

namespace arkostracker
{

class ViewportUtil
{
public:
    /** Prevents instantiation. */
    ViewportUtil() = delete;

    /**
     * Generic method to calculate the new "position" (meaning, X or Y) of a viewport from an item to be seen.
     * If the item is not visible, the new "position" is returned. Else, the same one.
     * Call this method twice for X, and Y if needed.
     * @param itemPosition the item position (X or Y).
     * @param itemSize the item size (width or height).
     * @param viewportPosition the viewport view offset (X or Y).
     * @param viewportSize the viewport size (width or height).
     * @return the new position to scroll to in the viewport (may be the same).
     */
    static int calculateIfScrollNeeded(int itemPosition, int itemSize, int viewportPosition, int viewportSize) noexcept;
};

}   // namespace arkostracker
