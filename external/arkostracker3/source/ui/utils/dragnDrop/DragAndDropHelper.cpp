#include "DragAndDropHelper.h"

namespace arkostracker 
{

bool DragAndDropHelper::isMouseDragInvalid(const juce::DragAndDropContainer& target, const juce::MouseEvent& event) noexcept
{
    return (!event.mods.isLeftButtonDown() || target.isDragAndDropActive() || (event.getDistanceFromDragStart() < 5));
}


}   // namespace arkostracker

