#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../utils/OptionalValue.h"

namespace arkostracker 
{

/** Utility to about (keyboard) keys. */
class KeysUtil
{
public:
    /**
     * @return the digit related to the key (1-9 and A-F, case-insensitive), or absent if there is none.
     * @param key the key.
     */
    static OptionalInt getHexNumberFromKey(const juce::KeyPress& key) noexcept;
};



}   // namespace arkostracker

