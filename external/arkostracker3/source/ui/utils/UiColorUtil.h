#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../utils/OptionalValue.h"

namespace arkostracker
{

/** Decoupled from ColorUtil, because uses Juce. */
class UiColorUtil
{
public:
    /** Prevents instantiation. */
    UiColorUtil() = delete;

    /** @return an optional 32 bits value from the optional color. */
    static OptionalValue<juce::uint32> toUInt32(const OptionalValue<juce::Colour>& color) noexcept;
};

}       // namespace arkostracker
