#pragma once

#include "juce_core/juce_core.h"

enum class SerialProfile : uint8_t
{
    custom = 1,             // >0 to ease the mapping with the UI.
    albireo,
    usifac,
    booster,
    last = booster,

    first = custom,
};