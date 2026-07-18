#include "InstrumentEditorControllerNoOp.h"

namespace arkostracker 
{

// ItemEditorController method implementations.
// ==========================================================

void InstrumentEditorControllerNoOp::onNewItemSelected(const Id& /*itemId*/, bool /*force*/) noexcept
{
}

void InstrumentEditorControllerNoOp::getKeyboardFocus() noexcept
{
}


// ParentViewLifeCycleAware method implementations.
// ===================================================

void InstrumentEditorControllerNoOp::onParentViewCreated(BoundedComponent& /*parentView*/)
{
}

void InstrumentEditorControllerNoOp::onParentViewResized(int /*startX*/, int /*startY*/, int /*newWidth*/, int /*newHeight*/)
{
}

void InstrumentEditorControllerNoOp::onParentViewFirstResize(juce::Component& /*parentView*/)
{
}

void InstrumentEditorControllerNoOp::onParentViewDeleted() noexcept
{
}


}   // namespace arkostracker

