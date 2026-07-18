#pragma once

#include "ItemEditorController.h"

namespace arkostracker 
{

/** An InstrumentEditorController that does nothing. */
class InstrumentEditorControllerNoOp final : public ItemEditorController
{
public:

    // ItemEditorController method implementations.
    // ==========================================================
    void onNewItemSelected(const Id& itemId, bool force) noexcept override;
    void getKeyboardFocus() noexcept override;

    // ParentViewLifeCycleAware method implementations.
    // ===================================================
    void onParentViewCreated(BoundedComponent& parentView) override;
    void onParentViewResized(int startX, int startY, int newWidth, int newHeight) override;
    void onParentViewFirstResize(juce::Component& parentView) override;
    void onParentViewDeleted() noexcept override;
};


}   // namespace arkostracker

