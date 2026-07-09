#include "NoInstrumentEditorControllerImpl.h"

#include "../../view/none/NoInstrumentEditorViewImpl.h"
#include "../../view/none/NoInstrumentEditorViewNoOp.h"

namespace arkostracker 
{

NoInstrumentEditorControllerImpl::NoInstrumentEditorControllerImpl() noexcept :
        instrumentEditorView(std::make_unique<NoInstrumentEditorViewNoOp>())
{
}

// InstrumentEditorController method implementations.
// =====================================================

void NoInstrumentEditorControllerImpl::onNewItemSelected(const Id& /*instrumentId*/, bool /*force*/) noexcept
{
}

void NoInstrumentEditorControllerImpl::onParentViewCreated(BoundedComponent& parentView)
{
    instrumentEditorView = std::make_unique<NoInstrumentEditorViewImpl>();

    parentView.addAndMakeVisible(*instrumentEditorView);
}

void NoInstrumentEditorControllerImpl::onParentViewResized(const int startX, const int startY, const int newWidth, const int newHeight)
{
    instrumentEditorView->setBounds(startX, startY, newWidth, newHeight);
}

void NoInstrumentEditorControllerImpl::onParentViewFirstResize(juce::Component& /*parentView*/)
{
}

void NoInstrumentEditorControllerImpl::onParentViewDeleted() noexcept
{
    // Goes back to the no-op view.
    instrumentEditorView = std::make_unique<NoInstrumentEditorViewNoOp>();
}

void NoInstrumentEditorControllerImpl::getKeyboardFocus() noexcept
{
    instrumentEditorView->getKeyboardFocus();
}

}   // namespace arkostracker
