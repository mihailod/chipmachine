#pragma once

#include "../../view/none/NoInstrumentEditorView.h"
#include "../ItemEditorController.h"

namespace arkostracker 
{

/** Implementation of the controller for the "none selected" Instrument Editor. */
class NoInstrumentEditorControllerImpl final : public ItemEditorController
{
public:
    /** Constructor. */
    NoInstrumentEditorControllerImpl() noexcept;

    // ItemEditorController method implementations.
    // =====================================================
    void onNewItemSelected(const Id& instrumentId, bool force) noexcept override;
    void onParentViewCreated(BoundedComponent& parentView) override;
    void onParentViewResized(int startX, int startY, int newWidth, int newHeight) override;
    void onParentViewFirstResize(juce::Component& parentView) override;
    void onParentViewDeleted() noexcept override;
    void getKeyboardFocus() noexcept override;

private:
    std::unique_ptr<NoInstrumentEditorView> instrumentEditorView;
};

}   // namespace arkostracker
