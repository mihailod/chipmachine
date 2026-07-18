#include "CellEffects.h"

#include "../../business/song/cells/CellEffectsChecker.h"
#include "../../utils/NumberUtil.h"
#include "CellConstants.h"

namespace arkostracker 
{

//const CellEffect CellEffects::emptyCellEffect = CellEffect(Effect::noEffect, 0);

// Important: all modification must set the dirty flag.
// ====================================================


CellEffects::CellEffects() noexcept :
        indexToEffect(),
        dirty(false),
        cachedIndexToError()
{
}

CellEffects::CellEffects(const std::vector<std::pair<Effect, int>>& effectAndLogicalValues) noexcept :
        indexToEffect(),
        dirty(true),            // There can be errors in the given effects.
        cachedIndexToError()
{
    for (const auto& entry : effectAndLogicalValues) {
        addEffect(entry.first, entry.second);
    }
}

std::map<int, CellEffect> CellEffects::getExistingEffects() const noexcept
{
    std::map<int, CellEffect> indexToCellEffect;

    for (const auto& entry : indexToEffect) {
        // Only inserts present effect.
        const auto& effect = entry.second;
        if (effect.isPresent()) {
            const auto index = entry.first;
            indexToCellEffect.insert(std::make_pair(index, effect));
        }
    }

    return indexToCellEffect;
}

std::vector<CellEffect> CellEffects::getPresentEffects() const noexcept
{
    std::vector<CellEffect> cellEffects;

    for (const auto& entry : indexToEffect) {
        // Only inserts present effect.
        const auto& effect = entry.second;
        if (effect.isPresent()) {
            cellEffects.push_back(entry.second);
        }
    }

    return cellEffects;
}


bool CellEffects::setEffect(const int index, const CellEffect& cellEffect) noexcept
{
    if ((index < 0) || (index >= CellConstants::effectCount)) {
        jassertfalse;
        return false;
    }
    indexToEffect[index] = cellEffect;
    dirty = true;

    return true;
}

CellEffect CellEffects::getEffect(const int index) const noexcept
{
    if ((index < 0) || (index >= CellConstants::effectCount)) {
        jassertfalse;
        return CellEffect();
    }

    if (auto iterator = indexToEffect.find(index); iterator != indexToEffect.cend()) {
        return iterator->second;
    }

    // Not found.
    return CellEffect();
}

bool CellEffects::isEmpty() const noexcept
{
    // If all effects are absent, returns true.
    return std::all_of(indexToEffect.begin(), indexToEffect.end(), [](const auto& entry) {
        const auto& cellEffect = entry.second;
        return !cellEffect.isPresent();
    });
}

bool CellEffects::isClean() const noexcept
{
    // If all effects are clean, returns true.
    return std::all_of(indexToEffect.begin(), indexToEffect.end(), [](const auto& entry) {
        const auto& cellEffect = entry.second;
        return cellEffect.isClean();
    });
}

void CellEffects::clearEffect(const int index) noexcept
{
    jassert(index >= 0);

    indexToEffect.erase(index);
    dirty = true;
}

OptionalValue<CellEffect> CellEffects::find(const Effect effect) const noexcept
{
    return findWithIndex(effect).second;
}

std::pair<int, OptionalValue<CellEffect>> CellEffects::findWithIndex(const Effect effect) const noexcept
{
    for (const auto& entry : indexToEffect) {
        const auto& cellEffect = entry.second;
        if (cellEffect.getEffect() == effect) {
            return { entry.first, cellEffect };
        }
    }

    // Not found.
    return { -1, { } };
}

void CellEffects::performOnEffects(const std::function<void(std::pair<int, CellEffect&>)>& operationToApply) noexcept
{
    for (auto& entry : indexToEffect) {
        const auto index = entry.first;
        auto& cellEffect = entry.second;
        operationToApply({ index, cellEffect });
    }
}

bool CellEffects::removeEffect(const Effect effect) noexcept
{
    auto index = -1;
    for (const auto& entry : indexToEffect) {
        const auto& cellEffect = entry.second;
        if (cellEffect.getEffect() == effect) {
            index = entry.first;
            break;
        }
    }

    // Removes the effect, if found.
    if (index >= 0) {
        clearEffect(index);
    }

    return (index >= 0);
}

bool CellEffects::hasEffect(const int index) const noexcept
{
    if (auto it = indexToEffect.find(index); it != indexToEffect.cend()) {
        return (it->second.getEffect() != Effect::noEffect);        // Shouldn't be necessary, only a security.
    }

    return false;
}

bool CellEffects::areEffectsPresentAmong(const std::set<Effect>& effectsToCompareTo) const noexcept
{
    // No "no effect" must be present!
    jassert(effectsToCompareTo.find(Effect::noEffect) == effectsToCompareTo.cend());

    // Browses all the effects.
    for (const auto& cellEffect : getPresentEffects()) {
        // Does this effect belong the Set? If not, we can exit directly.
        if (effectsToCompareTo.find(cellEffect.getEffect()) != effectsToCompareTo.cend()) {
            return true;
        }
    }

    // None is found.
    return false;
}

void CellEffects::clear() noexcept
{
    indexToEffect.clear();
    dirty = true;
}

OptionalValue<EffectError> CellEffects::getError(const int effectIndex, const bool isNotePresent, const bool isInstrumentPresent) const noexcept
{
    // Rebuilds the cached map, unless no change has been performed.
    if (dirty) {
        cachedIndexToError = CellEffectsChecker::checkEffects(*this, isNotePresent, isInstrumentPresent);
        dirty = false;
    }

    if (auto iterator = cachedIndexToError.find(effectIndex); iterator != cachedIndexToError.cend()) {
        return iterator->second;
    }

    return { };         // No error.
}

bool CellEffects::addEffect(const Effect effect, const int logicalValue) noexcept
{
    // Where to insert?
    for (auto effectIndex = 0; effectIndex < CellConstants::effectCount; ++effectIndex) {
        auto iterator = indexToEffect.find(effectIndex);
        if ((iterator == indexToEffect.cend()) || (iterator->second.getEffect() == Effect::noEffect)) {
            setEffect(effectIndex, CellEffect::buildFromLogicalValue(effect, logicalValue));
            return true;
        }
    }

    // No room!
    return false;
}

void CellEffects::setEffectDigit(const int effectIndex, const int digitIndex, const int value) noexcept
{
    jassert((effectIndex >= 0) && (effectIndex < CellConstants::effectCount));

    // Does the effect exist?
    auto iterator = indexToEffect.find(effectIndex);
    CellEffect cellEffect;      // Default one (empty) created, used if the effect is not there.
    if (iterator != indexToEffect.cend()) {
        // The effect exists.
        cellEffect = iterator->second;
    }

    auto newCellEffect = fillCellEffectWithDigit(cellEffect, digitIndex, value);
    setEffect(effectIndex, newCellEffect);
}

void CellEffects::setEffectNumber(const int effectIndex, const Effect effect) noexcept
{
    jassert((effectIndex >= 0) && (effectIndex < CellConstants::effectCount));

    // Does the effect exist?
    auto iterator = indexToEffect.find(effectIndex);
    CellEffect cellEffect;      // Default one (empty) created, used if the effect is not there.
    if (iterator != indexToEffect.cend()) {
        // The effect exists.
        cellEffect = iterator->second;
    }

    auto newCellEffect = fillCellEffectWithEffect(cellEffect, effect);
    setEffect(effectIndex, newCellEffect);
}

void CellEffects::setEffectValue(const int effectIndex, const int rawValue) noexcept
{
    jassert((effectIndex >= 0) && (effectIndex < CellConstants::effectCount));

    // Does the effect exist?
    auto iterator = indexToEffect.find(effectIndex);

    CellEffect cellEffect(Effect::noEffect, rawValue);
    if (iterator != indexToEffect.cend()) {
        // The effect exists. Replaces its value.
        const auto readCellEffect = iterator->second;
        cellEffect = CellEffect(readCellEffect.getEffect(), rawValue);
    }

    setEffect(effectIndex, cellEffect);
}

void CellEffects::shiftEffects(const int shift) noexcept
{
    if (shift == 0) {
        return;
    }
    // Creates a new map from the current one.
    decltype(indexToEffect) newIndexToEffect;
    for (const auto&[index, effect] : indexToEffect) {
        const auto newIndex = index + shift;
        // Only encodes the effect if within bounds.
        if ((newIndex >=0) && (newIndex < CellConstants::effectCount)) {
            newIndexToEffect[newIndex] = effect;
        }
    }

    indexToEffect = newIndexToEffect;
    dirty = true;
}

CellEffect CellEffects::fillCellEffectWithDigit(const CellEffect& cellEffect, const int digitIndex, const int digit) noexcept
{
    jassert((digitIndex >= 0) && (digitIndex < 3));
    jassert((digit >= 0) && (digit <= 15));

    // Gets a new value with the new digit.
    const auto rawValue = cellEffect.getEffectRawValue();
    const auto newRawValue = NumberUtil::replaceDigit(rawValue, digitIndex, digit);

    return CellEffect(cellEffect.getEffect(), newRawValue);
}

CellEffect CellEffects::fillCellEffectWithEffect(const CellEffect& cellEffect, const Effect effect) noexcept
{
    return CellEffect(effect, cellEffect.getEffectRawValue());
}

bool CellEffects::operator==(const CellEffects& rhs) const noexcept
{
    return (indexToEffect == rhs.indexToEffect);
}

bool CellEffects::operator!=(const CellEffects& rhs) const noexcept
{
    return !(rhs == *this);
}


}   // namespace arkostracker

