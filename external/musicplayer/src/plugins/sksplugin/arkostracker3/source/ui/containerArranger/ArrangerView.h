#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Dimension.h"

namespace arkostracker 
{

/**
 * An abstract class for views to return how high the should be.
 * Each has its own auto-increasing ID, so no view can have the same ID.
 */
class ArrangerView : public juce::Component
{
public:
    /** Constructor. */
    ArrangerView() noexcept;

    /** Destructor. */
    ~ArrangerView() override = default;

    /** @return an object about how the width should be treated. */
    virtual Dimension getArrangerViewWidth() const noexcept = 0;
    /** @return an object about how the height should be treated. */
    virtual Dimension getArrangerViewHeight() const noexcept = 0;

    /**
     * @return the width or height of the current View. It may be 0 if not resized yet.
     * @param width true to get the width, false to get the height.
     */
    int getCurrentSize(bool width) const noexcept;

    /**
     * @return the maximum width or height of the current View.
     * @param width true to get the width, false to get the height.
     */
    int getMaximumSize(bool width) const noexcept;

    /**
     * @return the minimum width or height of the current View.
     * @param width true to get the width, false to get the height.
     */
    int getMinimumSize(bool width) const noexcept;

    /** @return the ID for this ArrangerView. */
    int getArrangerId() const noexcept;

    /**
     * Adds a value to one size.
     * @param toWidth true to add to the width, false to the height.
     * @param valueToAdd the value. May be negative of course.
     */
    void addToSize(bool toWidth, int valueToAdd) noexcept;

    /**
     * Adds a value to one dimension of the position.
     * @param toWidth true to add to the width, false to the height.
     * @param valueToAdd the value. May be negative of course.
     */
    void addToPosition(bool toWidth, int valueToAdd) noexcept;

    /** @return true if the header is hidden. False if not, or if not relevant. */
    virtual bool isHiddenHeader() const;

private:
    int arrangerId;                     // The ID for this view.
};

}   // namespace arkostracker

