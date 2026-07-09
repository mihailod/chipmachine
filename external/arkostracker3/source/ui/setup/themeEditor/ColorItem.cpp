#include "ColorItem.h"

namespace arkostracker 
{

ColorItem::ColorItem(const int pColorId, juce::String pColorName, Listener& pListener) :
        colorId(pColorId),
        colorName(std::move(pColorName)),
        listener(pListener)
{
}

std::unique_ptr<juce::Component> ColorItem::createItemComponent()
{
    return std::make_unique<ColorComponent>(colorName, *this);
}

bool ColorItem::mightContainSubItems()
{
    return false;
}

void ColorItem::itemClicked(const juce::MouseEvent& event)
{
    if (event.mods.isLeftButtonDown()) {
        listener.onColorSwatchClicked(colorId);
    }
}

int ColorItem::getColorId() const noexcept
{
    return colorId;
}

juce::Colour ColorItem::findColourFromTheme() const noexcept
{
    return juce::LookAndFeel::getDefaultLookAndFeel().findColour(colorId);
}


// ===========================================

ColorComponent::ColorComponent(const juce::String& pColorName, ColorItem& pColorItem) noexcept :
        label(juce::String(), pColorName),
        colorItem(pColorItem),
        colorSwatch(colorItem.findColourFromTheme())
{
    setInterceptsMouseClicks(false, false);     // Else the parent doesn't get the clicks.

    addAndMakeVisible(label);
    addAndMakeVisible(colorSwatch);

    label.setJustificationType(juce::Justification::Flags::verticallyCentred);
}

// Component method implementations.
// ==========================================
void ColorComponent::resized()
{
    constexpr auto swatchWidth = 30;
    constexpr auto swatchVerticalMargins = 2;
    constexpr auto margins = 10;

    const auto height = getHeight();

    const auto swatchX = getWidth() - swatchWidth - margins;
    const auto swatchHeight = getHeight() - 2 * swatchVerticalMargins;

    colorSwatch.setBounds(swatchX, swatchVerticalMargins, swatchWidth, swatchHeight);
    label.setBounds(0, 0, swatchX - margins, height);
}

void ColorComponent::lookAndFeelChanged()
{
    // Forces the refresh with the color from the Theme.
    const auto color = colorItem.findColourFromTheme();
    colorSwatch.setSwatchColor(color);
}

}   // namespace arkostracker
