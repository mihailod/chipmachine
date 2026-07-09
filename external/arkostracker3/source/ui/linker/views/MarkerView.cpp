#include "MarkerView.h"

namespace arkostracker 
{

const float MarkerView::fontHeight = 18.0F;
const float MarkerView::dashLengths[] = { 4.0F, 4.0F };             // NOLINT(*-avoid-c-arrays)

MarkerView::MarkerView(juce::String pName) noexcept :
    font(),
    name(std::move(pName))
{
    jassert(name.isNotEmpty());     // Don't show any empty markers!

    setInterceptsMouseClicks(false, false);

    font.setHeight(fontHeight);
    const auto width = font.getStringWidth(name) + 10;      // Magic number... :(.
    setSize(width, desiredHeight);
}

void MarkerView::paint(juce::Graphics& g)
{
    const auto lineColor = juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::Label::ColourIds::textColourId);
    g.setColour(lineColor);
    g.drawDashedLine(juce::Line(0.0F, 0.0F, 0.0F, static_cast<float>(getHeight())), &dashLengths[0], 2);

    const auto textColor = lineColor.withMultipliedAlpha(0.8F);
    g.setColour(textColor);
    g.setFont(font);
    g.drawSingleLineText(name, 4, static_cast<int>(fontHeight * 0.7F));     // Magic number again.
}

}   // namespace arkostracker
