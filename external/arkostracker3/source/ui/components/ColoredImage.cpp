#include "ColoredImage.h"

namespace arkostracker 
{

ColoredImage::ColoredImage(const void* pImageBytes, const size_t pImageSize, const juce::Colour pColor, const bool pUseColor) noexcept :
        innerImage(juce::ImageFileFormat::loadFrom(pImageBytes, pImageSize)),
        color(pColor),
        useColor(pUseColor)
{
}

ColoredImage::ColoredImage(const juce::Colour pColor, const bool pUseColor) noexcept :
        innerImage(),
        color(pColor),
        useColor(pUseColor)
{
}

void ColoredImage::paint(juce::Graphics& g)
{
    // If disabled, uses a low alpha.
    const auto colorToUse = isEnabled() ? color : color.withAlpha(0.4F);
    g.setColour(colorToUse);

    g.drawImage(innerImage, getLocalBounds().toFloat(), juce::RectanglePlacement::centred | juce::RectanglePlacement::doNotResize,
                useColor);
}

void ColoredImage::setImageColor(const juce::Colour newColor) noexcept
{
    color = newColor;
    if (useColor) {
        repaint();
    }
}

void ColoredImage::setImage(const void* imageBytes, const size_t imageSize) noexcept
{
    innerImage = juce::ImageFileFormat::loadFrom(imageBytes, imageSize);
}

void ColoredImage::setImage(const juce::Image& image) noexcept
{
    innerImage = image;
}

int ColoredImage::getImageWidth() const noexcept
{
    return innerImage.getWidth();
}

int ColoredImage::getImageHeight() const noexcept
{
    return innerImage.getHeight();
}

}   // namespace arkostracker
