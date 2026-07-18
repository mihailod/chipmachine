#include "EffectNumber.h"

namespace arkostracker 
{

EffectNumber::EffectNumber() noexcept :
        effectIndex(0),
        effect(Effect::noEffect)
{
}

EffectNumber::EffectNumber(int pEffectIndex, Effect pEffect) noexcept :
        effectIndex(pEffectIndex),
        effect(pEffect)
{
}


}   // namespace arkostracker

