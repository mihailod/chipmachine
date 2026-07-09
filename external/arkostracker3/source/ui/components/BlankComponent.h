#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../utils/OptionalValue.h"

namespace arkostracker 
{

/** A very simple Component with only a background color. */
class BlankComponent final : public juce::Component
{
public:
    /**
     * Constructor for a static color. This will NOT adapt automatically in case of change of LookAndFeel.
     * @param newBackgroundColor the color to display. If opaque, the component will be too.
     */
    explicit BlankComponent(const juce::Colour& newBackgroundColor) noexcept;

    /**
     * Constructor. This will adapt in case of change of LookAndFeel.
     * @param colorId the color id of the theme.
     */
    explicit BlankComponent(int colorId) noexcept;

    /**
     * Sets a transformation on the color, if wanted. This is useful to use a theme color id, but applies a contract/alpha for example.
     * @param transformation the transformation.
     */
    void setTransformationOnColour(std::function<juce::Colour(juce::Colour)> transformation) noexcept;

    // Component method implementations.
    // ==============================================
    void paint(juce::Graphics& g) override;

private:
    juce::Colour backgroundColour;                          // The background color.
    OptionalInt colorId;

    std::function<juce::Colour(juce::Colour)> transformation;
};

}   // namespace arkostracker
