#pragma once

#include "model/LineContext.h"

namespace arkostracker
{

class CellLocationInPosition;

/** Abstract class that determines the effects at a specific location. */
class EffectContext
{
public:
    virtual ~EffectContext() = default;

    /**
     * @return the effect context. Note that it may be inaccurate as the building is asynchronous. It may be empty.
     * @param location where to get the context from. Note that the SubsongId is ignored.
     * @param now the current time.
     */
    virtual LineContext determineContext(const CellLocationInPosition& location, juce::int64 now = juce::Time::currentTimeMillis()) const noexcept = 0;
};

}   // namespace arkostracker
