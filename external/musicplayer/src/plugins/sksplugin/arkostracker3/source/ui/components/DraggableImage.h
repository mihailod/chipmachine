#pragma once

#include "ColoredImage.h"

namespace arkostracker
{

/** Simple image which drag event can be notified. As a convenience, this is a wrapper around the ColoredImage, so a coloration can be used if wanted. */
class DraggableImage final : public ColoredImage
{
public:
    /** Listener to a drag. The client can, for example, relocate the Component. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the image is dragged.
         * @param draggedImage the dragged image.
         * @param event the mouse event.
         */
        virtual void onDraggableImageDragged(DraggableImage& draggedImage, const juce::MouseEvent& mouseEvent) = 0;
    };

    /**
     * Constructor.
     * @param listener the listener to the drag.
     * @param imageBytes the bytes of the image (BinaryData::xxx).
     * @param imageSize the size of the image (BinaryData::xxxSize).
     * @param color the color. May be an alpha layer. Ignored if useColor is false.
     * @param useColor true to use the color. Useful when wanting to use this class in a button, where sometimes a color can be used, sometimes not.
     */
    DraggableImage(Listener& listener, const void* imageBytes, size_t imageSize, juce::Colour color, bool useColor = true) noexcept;


    // Component method implementations.
    // ===================================================
    void mouseDrag(const juce::MouseEvent& event) override;

private:
    Listener& listener;               // The listener to the drag.
};

}   // namespace arkostracker
