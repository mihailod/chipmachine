#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/** A simple filled rectangle with a border. No interaction. */
class ColorSwatch : public juce::Component
{
public:
    /**
     * Constructor.
     * @param color the color to show.
     */
    explicit ColorSwatch(juce::Colour color) noexcept;

    /**
     * Sets the SwatchColor, updates the UI.
     * @param color the new color.
     */
    void setSwatchColor(juce::Colour color) noexcept;

    // Component method implementations.
    // ====================================
    void paint(juce::Graphics& g) override;

private:
    juce::Colour color;
};

}   // namespace arkostracker

