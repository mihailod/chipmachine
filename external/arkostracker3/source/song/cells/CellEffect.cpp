#include "CellEffect.h"

namespace arkostracker 
{

CellEffect::CellEffect(const Effect pEffect, const int pEffectRawValue) noexcept :
        effect(pEffect),
        effectRawValue(pEffectRawValue)
{
}

bool CellEffect::operator==(const CellEffect& other) const noexcept
{
    return ((effect == other.effect) && (effectRawValue == other.effectRawValue));
}

bool CellEffect::operator!=(const CellEffect& other) const noexcept
{
    return !operator==(other);
}

bool CellEffect::operator<(const CellEffect& other) const noexcept
{
    if (static_cast<unsigned char>(effect) != static_cast<unsigned char>(other.effect)) {           // NOLINT(clion-misra-cpp2008-5-0-4)
        return (static_cast<unsigned char>(effect) < static_cast<unsigned char>(other.effect));     // NOLINT(clion-misra-cpp2008-5-0-4)
    }

    return effectRawValue < other.effectRawValue;
}

Effect CellEffect::getEffect() const noexcept
{
    return effect;
}

int CellEffect::getEffectRawValue() const noexcept
{
    return effectRawValue;
}

bool CellEffect::isPresent() const noexcept
{
    return (effect != Effect::noEffect);
}

bool CellEffect::isClean() const noexcept
{
    return (effect == Effect::noEffect) && (effectRawValue == 0);
}

CellEffect CellEffect::buildFromLogicalValue(const Effect effect, const int effectLogicalValue) noexcept
{
    jassert(effectLogicalValue >= 0);
    const auto digitCount = EffectUtil::getDigitCount(effect);

    // Restricts the value, as a security.
    unsigned int mask;    // NOLINT(*-init-variables)
    unsigned int shiftCount;        // Will shift to the "left".    // NOLINT(*-init-variables)
    switch (digitCount) {
        default:
            jassertfalse;
            [[fallthrough]];
        case 0:
            mask = 0U;               // Empties the value.
            shiftCount = 0U;
            break;
        case 1:
            mask = 0b000000001111U;
            shiftCount = 8U;
            break;
        case 2:
            mask = 0b000011111111U;
            shiftCount = 4U;
            break;
        case 3:
            mask = 0b111111111111U;
            shiftCount = 0U;
            break;
    }

    const auto rawValue = static_cast<int>((static_cast<unsigned int>(effectLogicalValue) & mask) << shiftCount);

    return CellEffect(effect, rawValue);
}

int CellEffect::getEffectLogicalValue() const noexcept
{
    const auto digitCount = EffectUtil::getDigitCount(effect);

    unsigned int shiftCount;        // Will shift to the "right".       // NOLINT(*-init-variables)
    unsigned int finalMask;                                             // NOLINT(*-init-variables)
    switch (digitCount) {
        default:
            jassertfalse;
        case 0:
            [[fallthrough]];
        case 3:
            shiftCount = 0U;
            finalMask = 0b111111111111U;
            break;
        case 2:
            shiftCount = 4U;
            finalMask = 0b000011111111U;
            break;
        case 1:
            shiftCount = 8U;
            finalMask = 0b000000001111U;
            break;
    }

    return static_cast<int>((static_cast<unsigned int>(effectRawValue) >> shiftCount) & finalMask);
}

}   // namespace arkostracker
