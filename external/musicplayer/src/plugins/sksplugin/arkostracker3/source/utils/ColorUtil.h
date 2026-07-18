#pragma once

#include "OptionalValue.h"

namespace arkostracker
{

class ColorUtil
{
public:
    /** Prevents instantiation. */
    ColorUtil() = delete;

    /**
     * @return a color with a full alpha.
     * @param color the ARGB input color.
     */
    static juce::uint32 setAlphaToFull(juce::uint32 color) noexcept;
};

}       // namespace arkostracker
