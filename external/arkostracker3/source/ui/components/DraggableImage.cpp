#include "DraggableImage.h"

namespace arkostracker
{

DraggableImage::DraggableImage(Listener& pListener, const void* pImageBytes, size_t pImageSize, juce::Colour pColor, bool pUseColor) noexcept:
        ColoredImage(pImageBytes, pImageSize, pColor, pUseColor),
        listener(pListener)
{
}

// Component method implementations.
// ===================================================

void DraggableImage::mouseDrag(const juce::MouseEvent& event)
{
    listener.onDraggableImageDragged(*this, event);
}

}   // namespace arkostracker
