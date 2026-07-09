#pragma once

#include "../../../../song/cells/Effect.h"

namespace arkostracker 
{

/** Simple holder of the data of an effect index and number. */
class EffectNumber
{
public:
    /** Default constructor, useful for Optionals. */
    EffectNumber() noexcept;

    /**
     * Constructor.
     * @param effectIndex the index of the effect (0-3).
     * @param effect the effect.
     */
    EffectNumber(int effectIndex, Effect effect) noexcept;

    const int effectIndex;
    const Effect effect;
};



}   // namespace arkostracker

