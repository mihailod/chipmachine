#pragma once

#include "../../../utils/Id.h"
#include "../../utils/ParentViewLifeCycleAware.h"

namespace arkostracker 
{

/** The virtual Controller for the Item Editor (psg, sample, arp, pitch). */
class ItemEditorController : public ParentViewLifeCycleAware
{
public:
    /**
     * Called when a new item is selected. It may be the same as what is seen by the controller.
     * @param itemId the ID of the item.
     * @param force true if the refresh must be forced. For example, the 2nd item has been deleted, so the 3rd is displayed, yet it is now 2nd.
     */
    virtual void onNewItemSelected(const Id& itemId, bool force) noexcept = 0;

    /** Gets the keyboard focus to the view. */
    virtual void getKeyboardFocus() noexcept = 0;
};

}   // namespace arkostracker
