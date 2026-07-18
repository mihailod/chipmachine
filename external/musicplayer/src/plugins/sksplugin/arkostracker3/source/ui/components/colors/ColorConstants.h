#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker
{

/** Constants for Colors. */
class ColorConstants
{
public:
    /** Prevents instantiation. */
    ColorConstants() = delete;

    /** The default color to be used in the swatches. */
    static const juce::uint32 defaultColorAsArgb;

    /** Colors to be displayed in swatches, for example. */
    static const std::vector<juce::uint32> colors;
    /** How many colors per "pack". */
    static const int colorPackCount;
};

}   // namespace arkostracker
