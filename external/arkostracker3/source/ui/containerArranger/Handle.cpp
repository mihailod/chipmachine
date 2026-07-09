#include "Handle.h"

#include "../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

const int Handle::thickness = 3;
const int Handle::thicknessOnExtremities = 1;

Handle::Handle(HandleListener& pListener, bool pHorizontal, bool pIsExtremity) noexcept :
        lockedDimension(getDimension(pIsExtremity), getDimension(pIsExtremity), getDimension(pIsExtremity)),
        listener(pListener),
        horizontal(pHorizontal),
        isExtremity(pIsExtremity),
        mouseOverOrDragged(false)
{
}

int Handle::getDimension(bool isExtremity) noexcept
{
    return isExtremity ? thicknessOnExtremities : thickness;
}

// Component methods implementation.
// =======================================

void Handle::paint(juce::Graphics& g)
{
    g.fillAll(mouseOverOrDragged && !isExtremity
              ? findColour(static_cast<int>(LookAndFeelConstants::Colors::panelHandleMouseOver))
              : findColour(static_cast<int>(LookAndFeelConstants::Colors::interspace)));
}

void Handle::mouseEnter(const juce::MouseEvent& /*event*/)
{
    // Horizontal cannot be dragged.
    if (isExtremity || horizontal) {
        return;
    }

    mouseOverOrDragged = true;
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    repaint();
}

void Handle::mouseExit(const juce::MouseEvent& /*event*/)
{
    // Horizontal cannot be dragged.
    if (isExtremity || horizontal) {
        return;
    }

    mouseOverOrDragged = false;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
}

void Handle::mouseDrag(const juce::MouseEvent& event)
{
    // Only left button is allowed. No drag for horizontal.
    if (isExtremity || horizontal || !event.mods.isLeftButtonDown()) {
        return;
    }

    // Difference between the last known position and now.
    const auto distance = event.getPosition().getX();

    // Transmits to the listener of how much the user tried to move.
    listener.onHandleDragged(getArrangerId(), distance);
}


// ArrangerView method implementations.
// =======================================

Dimension Handle::getArrangerViewWidth() const noexcept
{
    if (horizontal) {
        return Dimension::buildFree();
    }
    return lockedDimension;
}

Dimension Handle::getArrangerViewHeight() const noexcept
{
    if (!horizontal) {
        return Dimension::buildFree();
    }
    return lockedDimension;
}

}   // namespace arkostracker
