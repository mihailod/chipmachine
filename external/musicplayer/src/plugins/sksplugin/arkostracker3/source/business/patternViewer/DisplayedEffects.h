#pragma once

#include <unordered_map>

#include "../../song/cells/Effect.h"

namespace arkostracker 
{

/** Checks the validity of the displayed effects. */
class DisplayedEffects
{
public:

    /** A default map linking an effect to a displayable (and "keyable" char!). */
    static const std::unordered_map<Effect, juce::juce_wchar>& getDefaultEffectToDisplayedChar();
};

}   // namespace arkostracker

