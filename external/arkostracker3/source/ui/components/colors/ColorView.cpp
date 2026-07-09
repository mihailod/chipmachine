#include "ColorView.h"

namespace arkostracker 
{

ColorView::ColorView(ColorView::Listener& pListener, const juce::Colour pColor, const int pViewId, const bool pSelected) noexcept :
        listener(pListener),
        color(pColor),
        selected(pSelected),
        viewId(pViewId)
{
    setMouseClickGrabsKeyboardFocus(false);

    setBorderColorFromCurrentColor();
}

void ColorView::paint(juce::Graphics& g)
{
    const auto height = getHeight();
    const auto width = getWidth();

    // Fills the color.
    const auto fillWidth = width - 2 * borderSelectSize;
    const auto fillHeight = height - 2 * borderSelectSize;

    g.setColour(color);
    g.fillRect(borderSelectSize, borderSelectSize, fillWidth, fillHeight);

    // Draws the border.
    g.setColour(selected ? borderSelectedColor : borderNotSelectedColor);
    if (selected) {
        // Fills the whole border.
        g.drawRect(0, 0, width, height, borderSelectSize);
    } else {
        // Draws only the border around the fill.
        g.drawRect(borderSelectSize - 1, borderSelectSize - 1, fillWidth + 2 * borderNotSelectedSize,  fillHeight + 2 * borderNotSelectedSize,
                   borderNotSelectedSize);
    }
}

void ColorView::setColorAndRepaint(const juce::Colour newColor) noexcept
{
    // Need to change?
    if (color == newColor) {
        return;
    }

    color = newColor;
    setBorderColorFromCurrentColor();

    repaint();
}

const juce::Colour& ColorView::getColor() const noexcept
{
    return color;
}

void ColorView::mouseDown(const juce::MouseEvent& /*event*/)
{
    listener.onColorClicked(color, viewId);
}

void ColorView::mouseDoubleClick(const juce::MouseEvent& /*event*/)
{
    listener.onColorDoubleClicked();
}

void ColorView::setSelectedAndRepaint(const bool newSelected) noexcept
{
    // Same state? Then don't do anything.
    if (newSelected == selected) {
        return;
    }

    selected = newSelected;
    repaint();
}

void ColorView::setBorderColorFromCurrentColor() noexcept
{
    borderNotSelectedColor = juce::Colour(0xff808080);  //color.contrasting(0.4); Contrasting does not work well on all colors...
    borderSelectedColor = juce::Colour(0xffffffff);     //color.contrasting();
}

}   // namespace arkostracker
