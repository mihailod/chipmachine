#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/**
 * A simple wrapper around an image, which colors an image. As a convenience, a flag prevents the image to be colored if wanted.
 * It is probably better to use ThemedColoredImage though!
 */
class ColoredImage : public juce::Component
{
public:
    /**
     * Constructor.
     * @param imageBytes the bytes of the image (BinaryData::xxx).
     * @param imageSize the size of the image (BinaryData::xxxSize).
     * @param color the color. May have an alpha layer. Ignored if useColor is false.
     * @param useColor true to use the color. Useful when wanting to use this class in a button, where sometimes a color can be used, sometimes not.
     */
    ColoredImage(const void* imageBytes, size_t imageSize, juce::Colour color, bool useColor = true) noexcept;

    /**
     * Constructor without an image yet.
     * @param color the color. May have an alpha layer. Ignored if useColor is false.
     * @param useColor true to use the color. Useful when wanting to use this class in a button, where sometimes a color can be used, sometimes not.
     */
    explicit ColoredImage(juce::Colour color = juce::Colours::white, bool useColor = true) noexcept;

    /**
     * Sets a new image.
     * @param imageBytes the image data.
     * @param imageSize the image data size.
     */
    void setImage(const void* imageBytes, size_t imageSize) noexcept;

    /**
     * Sets a new image.
     * @param image the image.
     */
    void setImage(const juce::Image& image) noexcept;

    /** Sets a new color for the image, refreshes the display. */
    void setImageColor(juce::Colour color) noexcept;

    /** @return the image width. */
    int getImageWidth() const noexcept;
    /** @return the image height. */
    int getImageHeight() const noexcept;

    // Component method implementations.
    // ======================================
    void paint(juce::Graphics& g) override;

private:
    juce::Image innerImage;
    juce::Colour color;
    const bool useColor;
};

}   // namespace arkostracker
