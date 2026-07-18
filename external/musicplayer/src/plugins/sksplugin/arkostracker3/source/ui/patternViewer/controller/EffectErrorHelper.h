#pragma once

#include <juce_core/juce_core.h>

#include "../../../song/cells/EffectError.h"

namespace arkostracker
{

class EffectErrorHelper
{
public:
    /** Prevents instantiation. */
    EffectErrorHelper() = delete;

    /**
     * @return a displayable error from the given error.
     * @param error the error.
     */
    static juce::String effectErrorToDisplayableString(EffectError error) noexcept;
};

}   // namespace arkostracker
