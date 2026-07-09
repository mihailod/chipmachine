#include "BlankComponent.h"

namespace arkostracker 
{

BlankComponent::BlankComponent(const juce::Colour& newBackgroundColor) noexcept:
        backgroundColour(newBackgroundColor),
        colorId()
{
}

BlankComponent::BlankComponent(const int pColorId) noexcept :
        backgroundColour(0),
        colorId(pColorId)
{
}

void BlankComponent::setTransformationOnColour(std::function<juce::Colour(juce::Colour)> pTransformation) noexcept
{
    transformation = std::move(pTransformation);
}

void BlankComponent::paint(juce::Graphics& g)
{
    // What color to use?
    const auto colorArgb = colorId.isPresent() ? juce::LookAndFeel::getDefaultLookAndFeel().findColour(colorId.getValue()) : backgroundColour;
    auto color = juce::Colour(colorArgb);

    // Transforms the color?
    if (transformation) {
        color = transformation(color);
    }

    setOpaque(color.isOpaque());

    g.fillAll(color);
}

}   // namespace arkostracker
