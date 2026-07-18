#pragma once

#include <set>



#include "ArrangerView.h"

namespace arkostracker 
{

/**
 * Holds the algorithm to shrink/stretch Views, used by the ContainerArranger (vertical) and LineContainer (horizontal).
 * Also holds the algorithm for the Handle drag to resize the views.
 *
 * It does not hold any Views, they are given to it when needed.
 *
 * One instance is only interested in ONE dimension, so a vertical arranger will not care about width change.
 */
class ViewArranger
{
public:
    /**
     * Constructor.
     * @param horizontal true if the Views are put horizontally, false for vertically.
     * @param topOffset offset from the top, where no view should appear. Only relevant for vertical.
     */
    ViewArranger(bool horizontal, int topOffset) noexcept;

    /**
     * Called when the whole is resized.
     * Can be called because:
     * - First display (Views have no sizes).
     * - New layout applied (Views have a size, but maybe not the right one according to the screen dimensions).
     * - Resize of the parent.
     * @param parentComponent the parent Component that has been resized.
     * @param views the views to manage.
     */
    void onResized(const juce::Component& parentComponent, const std::vector<ArrangerView*>& views) noexcept;

    /**
     * Called when a drag on a handle has been performed. May resize the Views if relevant and possible.
     * @param handleId the ID of the handle.
     * @param distancePixels the distance, in pixels, according to the orientation of the Handle.
     * @param views the views to manage.
     */
    void onHandleDragged(int handleId, int distancePixels, const std::vector<ArrangerView*>& views) const noexcept;

private:
    /**
     * Builds a dimension array with simply calculated size of Views.
     * @param availableSize the size of the parent we can use.
     * @param views the views to manage.
     * @return the sizes.
     */
    std::vector<int> calculateSizes(int availableSize, const std::vector<ArrangerView*>& views) const noexcept;

    /**
     * Manages the shrink of the views.
     * @param views the Views. They may have a real size set or not.
     * @param totalSizeToSave the size that needs to be retrieved, in pixels.
     * @param sizes the currently computed sizes, one for each View. All must have one size, even if default (>0). Will be updated.
     * @param viewIndexesToResize the indexes of the Views that can be resized. Shouldn't be empty.
     */
    void manageShrink(const std::vector<ArrangerView*>& views, int totalSizeToSave, std::vector<int>& sizes, const std::set<size_t>& viewIndexesToResize) const noexcept;

    /**
     * Manages the shrink of the views.
     * @param views the Views. They may have a real size set or not.
     * @param remainingSize the size that needs to be filled, in pixels.
     * @param sizes the currently computed sizes, one for each View. All must have one size, even if default (>0). Will be updated.
     * @param viewIndexesToResize the indexes of the Views that can be resized. Shouldn't be empty.
     */
    void manageGrow(const std::vector<ArrangerView*>& views, int remainingSize, std::vector<int>& sizes, const std::set<size_t>& viewIndexesToResize) const noexcept;

    /**
     * Sets the sizes from the given array to the Views.
     * @param parentComponent the parent Component that has been resized.
     * @param sizes the sizes of each Views to use.
     * @param availableSize the size of the parent we can use.
     * @param views the views to manage.
     */
    void setSizesToViews(const juce::Component& parentComponent, const std::vector<int>& sizes, int availableSize, const std::vector<ArrangerView*>& views) const noexcept;

    bool horizontal;            // True if the Views are put horizontally, false for vertically.
    int topOffset;              // Offset from the top, where no view should appear. Only relevant for vertical.
};

}   // namespace arkostracker
