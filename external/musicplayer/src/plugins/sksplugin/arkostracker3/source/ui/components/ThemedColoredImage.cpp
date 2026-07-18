#include "ThemedColoredImage.h"

arkostracker::ThemedColoredImage::ThemedColoredImage(const void* pImageBytes, const size_t pImageSize, const int pLookAndFeelColorId,
    const float pOpacityWhenNotHovered, const float pOpacityWhenHovered) noexcept :
        onClick(),
        innerImage(juce::ImageFileFormat::loadFrom(pImageBytes, pImageSize)),
        lookAndFeelColorId(pLookAndFeelColorId),
        cachedColor(determineImageColor()),
        opacityWhenNotHovered(pOpacityWhenNotHovered),
        opacityWhenHovered(pOpacityWhenHovered),
        isHovered()
{
}

juce::Colour arkostracker::ThemedColoredImage::determineImageColor() const noexcept
{
    return juce::LookAndFeel::getDefaultLookAndFeel().findColour(lookAndFeelColorId);
}

int arkostracker::ThemedColoredImage::getImageWidth() const noexcept
{
    return innerImage.getWidth();
}

int arkostracker::ThemedColoredImage::getImageHeight() const noexcept
{
    return innerImage.getHeight();
}


// Component method implementations.
// ======================================

void arkostracker::ThemedColoredImage::paint(juce::Graphics& g)
{
    // If disabled, uses a low alpha.
    auto alpha = isHovered ? opacityWhenHovered : opacityWhenNotHovered;
    if (!isEnabled()) {
        alpha *= opacityWhenDisabled;
    }
    const auto colorToUse = cachedColor.withMultipliedAlpha(alpha);

    g.setColour(colorToUse);

    g.drawImage(innerImage, getLocalBounds().toFloat(), juce::RectanglePlacement::centred | juce::RectanglePlacement::doNotResize, true);
}

void arkostracker::ThemedColoredImage::lookAndFeelChanged()
{
    cachedColor = determineImageColor();
    repaint();
}

void arkostracker::ThemedColoredImage::mouseDown(const juce::MouseEvent& event)
{
    if (onClick != nullptr) {
        onClick(event.mods.isLeftButtonDown());
    }
}

void arkostracker::ThemedColoredImage::mouseEnter(const juce::MouseEvent& /*event*/)
{
    // No need to repaint if there is no change between hovered/not hovered.
    if (juce::approximatelyEqual(opacityWhenHovered, opacityWhenNotHovered)) {
        return;
    }

    isHovered = true;
    repaint();
}

void arkostracker::ThemedColoredImage::mouseExit(const juce::MouseEvent& /*event*/)
{
    // No need to repaint if there is no change between hovered/not hovered.
    if (juce::approximatelyEqual(opacityWhenHovered, opacityWhenNotHovered)) {
        return;
    }

    isHovered = false;
    repaint();
}
