#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../containerArranger/BoundedComponent.h"

namespace arkostracker 
{

/** Abstract class for Views to notify objects about the life and death of their parent. */
class ParentViewLifeCycleAware
{
public:
    /** Destructor. */
    virtual ~ParentViewLifeCycleAware() = default;

    /**
     * The parent view is created.
     * @param parentView the Parent View.
     */
    virtual void onParentViewCreated(BoundedComponent& parentView) = 0;

    /**
     * Called when the parent view is resized. The views may want to reorder themselves.
     * @param startX where the component must be located in X. Warning, all children don't manage this.
     * @param startY where the component must be located in Y. Warning, all children don't manage this.
     * @param newWidth the new width.
     * @param newHeight the new height.
     */
    virtual void onParentViewResized(int startX, int startY, int newWidth, int newHeight) = 0;

    /**
     * The parent view has a size for the first time.
     * @param parentView the Parent View.
     */
    virtual void onParentViewFirstResize(juce::Component& parentView) = 0;

    /** Called on the destructor of the parent View. */
    virtual void onParentViewDeleted() noexcept = 0;
};

}   // namespace arkostracker

