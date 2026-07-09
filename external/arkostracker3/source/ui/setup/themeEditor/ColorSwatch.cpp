#include "ColorSwatch.h"

namespace arkostracker 
{

ColorSwatch::ColorSwatch(juce::Colour pColor) noexcept :
        color(pColor)
{
    setOpaque(true);
}

void ColorSwatch::setSwatchColor(juce::Colour newColor) noexcept
{
    color = newColor;
    repaint();
}

void ColorSwatch::paint(juce::Graphics& g)
{
    auto newColor = color.withAlpha(1.0F);      // Forces alpha, else the color will blend with whatever is on the background, and will lack realism.
    g.fillAll(newColor);

    auto borderColor = newColor.contrasting(0.7F);
    g.setColour(borderColor);
    g.drawRect(getLocalBounds());
}

}   // namespace arkostracker

