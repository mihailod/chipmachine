#include "ViewPortWithHorizontalMouseWheel.h"

namespace arkostracker 
{

ViewPortWithHorizontalMouseWheel::ViewPortWithHorizontalMouseWheel(const bool pAllowKeys) noexcept :
        ViewPortInterceptKeys(pAllowKeys)
{
}

// juce::Viewport method implementations.
// =========================================

void ViewPortWithHorizontalMouseWheel::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& details)
{
    // If shift + wheel, don't do anything. Because it should do horizontal scrolling. Alt + wheel is for horizontal scrolling.
    if (event.mods.isAltDown()) {
        // Converts the vertical mouse wheel to a horizontal mouse wheel.
        auto newDetails = juce::MouseWheelDetails(details);
        newDetails.deltaX = details.deltaY;
        newDetails.deltaY = 0;
        // The "alt" mod must be removed, else juce::Viewport::useMouseWheelMoveIfNeeded dismisses the event. To do so, must create a new object.
        const juce::MouseEvent newEvent(
            event.source, event.position,
            juce::ModifierKeys(),  // Simplification, removes all modifiers (only Alt is really needed).
            event.pressure, event.orientation, event.rotation, event.tiltX, event.tiltY,
            event.eventComponent, event.originalComponent, event.eventTime, event.mouseDownPosition, event.mouseDownTime, event.getNumberOfClicks(),
            event.mouseWasDraggedSinceMouseDown()
        );
        juce::Viewport::mouseWheelMove(newEvent, newDetails);
    } else if (!event.mods.isShiftDown()) {
        juce::Viewport::mouseWheelMove(event, details);
    }
}

}   // namespace arkostracker
