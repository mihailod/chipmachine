#include "EffectDigit.h"

#include <juce_core/juce_core.h>

namespace arkostracker 
{

EffectDigit::EffectDigit() noexcept:
        effectIndex(),
        digitIndex(),
        value()
{
}

EffectDigit::EffectDigit(int pEffectIndex, int pDigitIndex, int pValue) noexcept:
        effectIndex(pEffectIndex),
        digitIndex(pDigitIndex),
        value(pValue)
{
    jassert((effectIndex >= 0) && (effectIndex < 4));
    jassert((digitIndex >= 0) && (digitIndex < 3));
    jassert((value >= 0) && (value <= 15));
}


}   // namespace arkostracker

