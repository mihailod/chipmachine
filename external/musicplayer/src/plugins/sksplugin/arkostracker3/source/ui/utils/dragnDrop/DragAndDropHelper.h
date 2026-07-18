#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

class DragAndDropHelper
{
public:
    /** Prevents instantiation. */
    DragAndDropHelper() = delete;

    /**
     * Indicates whether a mouse drag can trigger a "startDragging". The drag'n'drop must not have been started, and the mouse must be
     * a bit apart from the source.
     * This is typically called in a mouseDrag method.
     * @param target the object to test.
     * @param event the mouse event.
     * @return if true, the mouse drag is invalid (mouseDrag should return).
     */
    static bool isMouseDragInvalid(const juce::DragAndDropContainer& target, const juce::MouseEvent& event) noexcept;
};


}   // namespace arkostracker

