#include "CellEffectsChecker.h"

#include "../../../song/cells/Cell.h"
#include "../../../song/cells/CellEffects.h"

namespace arkostracker 
{

std::unordered_map<int, EffectError> CellEffectsChecker::checkEffects(const CellEffects& effectsToCheck, const bool isNotePresent, const bool isInstrumentPresent) noexcept
{
    std::unordered_map<int, EffectError> indexToError;

    // A first pass to simplify: finds a Reset. Anything before it is an error.
    const auto resetIndex = effectsToCheck.findWithIndex(Effect::reset).first;
    for (auto index = 0; index < resetIndex; ++index) {     // -1 if not found, so nothing more to test.
        if (effectsToCheck.hasEffect(index)) {
            indexToError[index] = EffectError::removeEffectBeforeReset;
        }
    }

    auto setVolumeEffectCount = 0;          // Matches only the "set volume", not the volume glide.
    auto volumeInOutEffectCount = 0;
    auto pitchEffectCount = 0;
    auto pitchTableEffectCount = 0;
    auto arpeggioEffectCount = 0;
    auto resetEffectCount = 0;
    auto legatoEffectCount = 0;

    auto forceArpeggioSpeedCount = 0;
    auto forceInstrumentSpeedCount = 0;
    auto forcePitchTableSpeedCount = 0;

    // Browses each effect.
    for (const auto& [index, cellEffect] : effectsToCheck.getExistingEffects()) {
        const auto effect = cellEffect.getEffect();
        const auto logicalValue = cellEffect.getEffectLogicalValue();
        const auto logicalValueIs0 = (logicalValue == 0);
        const auto resetFound = (resetEffectCount > 0);
        const auto logicalValueIsZeroAndResetFound = resetFound && logicalValueIs0;
        const auto isEffectGlide = (effect == Effect::pitchGlide);

        OptionalValue<EffectError> foundEffectError;

        switch (effect) {
            case Effect::noEffect:
                // Nothing to do if no effect.
                continue;
            case Effect::legato:
                // It is not supposed to be handled, but this can be reached by generators. Don't do anything about it, but check it has not been generated twice.
                if (legatoEffectCount++ > 0) {
                    foundEffectError = EffectError::tooManyLegatoEffects;
                }
                break;
            case Effect::reset:
                if (resetEffectCount++ > 0) {
                    foundEffectError = EffectError::tooManyResetEffects;
                }
                break;
            case Effect::volume:
                if (setVolumeEffectCount++ > 0) {
                    foundEffectError = EffectError::tooManyVolumeEffects;
                } else if (volumeInOutEffectCount > 0) {
                    // No "volume" should be put after a volume slide, it makes the latter useless.
                    foundEffectError = EffectError::volumeHidingVolumeSlide;
                } else if (resetEffectCount > 0) {
                    foundEffectError = EffectError::volumeMustBeCombinedWithReset;
                }
                break;
            case Effect::volumeIn: [[fallthrough]];
            case Effect::volumeOut:
                if (volumeInOutEffectCount++ > 0) {
                    foundEffectError = EffectError::tooManyVolumeSlideEffects;
                } else if (logicalValueIsZeroAndResetFound) {
                    foundEffectError = EffectError::uselessEffectStopAfterReset;
                }
                break;
            case Effect::arpeggio3Notes: [[fallthrough]];
            case Effect::arpeggio4Notes: [[fallthrough]];
            case Effect::arpeggioTable:
                if (arpeggioEffectCount++ > 0) {
                    foundEffectError = EffectError::tooManyArpeggioEffects;
                } else if (forceArpeggioSpeedCount > 0) {
                    foundEffectError = EffectError::arpeggioTableOrDirectHidingForceSpeed;
                } else if (logicalValueIsZeroAndResetFound) {
                    foundEffectError = EffectError::uselessEffectStopAfterReset;
                }
                break;
            case Effect::pitchDown: [[fallthrough]];
            case Effect::pitchUp: [[fallthrough]];
            case Effect::fastPitchDown: [[fallthrough]];
            case Effect::fastPitchUp: [[fallthrough]];
            case Effect::pitchGlide:
                if (pitchEffectCount++ > 0) {
                    foundEffectError = EffectError::tooManyPitchEffects;
                } else if (logicalValueIsZeroAndResetFound) {
                    foundEffectError = EffectError::uselessEffectStopAfterReset;
                } else if (resetFound && isEffectGlide) {
                    foundEffectError = EffectError::pitchGlideAfterResetIsMeaningless;
                } else if (isNotePresent && isInstrumentPresent && isEffectGlide) {
                    foundEffectError = EffectError::pitchGlideWithInstrumentIsForbidden;
                // Glide/Pitch with value 0 and a note+instrument are considered useless.
                } else if (isNotePresent && isInstrumentPresent && logicalValueIs0) {
                    foundEffectError = EffectError::pitchAndGlide0WithNewInstrumentIsUseless;
                // Glide 0 with a note but without instrument is useless.
                } else if (isNotePresent && !isInstrumentPresent && logicalValueIs0 && isEffectGlide) {
                    foundEffectError = EffectError::pitchGlide0WithNoteIsUseless;
                }
                break;
            case Effect::pitchTable:
                if (pitchTableEffectCount++ > 0) {
                    foundEffectError = EffectError::tooManyPitchTableEffects;
                } else if (forcePitchTableSpeedCount > 0) {
                    foundEffectError = EffectError::pitchTableHidingForceSpeed;
                } else if (logicalValueIsZeroAndResetFound) {
                    foundEffectError = EffectError::uselessEffectStopAfterReset;
                }
                break;
            case Effect::forceArpeggioSpeed:
                if (forceArpeggioSpeedCount++ > 0) {
                    foundEffectError = EffectError::tooManyForceArpeggioSpeedEffects;
                } else if (logicalValueIsZeroAndResetFound) {
                    foundEffectError = EffectError::uselessEffectStopAfterReset;
                }
                break;
            case Effect::forceInstrumentSpeed:
                if (forceInstrumentSpeedCount++ > 0) {
                    foundEffectError = EffectError::tooManyForceInstrumentSpeedEffects;
                } else if (logicalValueIsZeroAndResetFound) {
                    foundEffectError = EffectError::uselessEffectStopAfterReset;
                }
                break;
            case Effect::forcePitchTableSpeed:
                if (forcePitchTableSpeedCount++ > 0) {
                    foundEffectError = EffectError::tooManyForcePitchTableSpeedEffects;
                } else if (logicalValueIsZeroAndResetFound) {
                    foundEffectError = EffectError::uselessEffectStopAfterReset;
                }
                break;
        }

        // Any error found? Don't overwrite the already found error.
        if (foundEffectError.isPresent() && (indexToError.find(index) == indexToError.cend()) ) {
            indexToError[index] = foundEffectError.getValue();
        }
    }

    return indexToError;
}

CellEffects CellEffectsChecker::correctEffects(const Cell& cell) noexcept
{
    return correctEffects(cell.getEffects(), cell.isNote(), cell.isInstrument());
}

CellEffects CellEffectsChecker::correctEffects(const CellEffects& effectsToCorrect, const bool isNotePresent, const bool isInstrumentPresent) noexcept
{
    // Checks the effects.
    const auto indexToError = checkEffects(effectsToCorrect, isNotePresent, isInstrumentPresent);
    if (indexToError.empty()) {
        return effectsToCorrect;        // All is correct.
    }

    // At least one error.
    auto cellEffects = effectsToCorrect;

    // Clears the effect related to errors.
    for (const auto& entry : indexToError) {
        const auto index = entry.first;
        cellEffects.clearEffect(index);
    }

    return cellEffects;
}

CellEffects CellEffectsChecker::normalize(const Cell& cell) noexcept
{
    return normalize(cell.getEffects(), cell.isNote(), cell.isInstrument());
}

CellEffects CellEffectsChecker::normalize(const CellEffects& originalCellEffects, const bool isNotePresent, const bool isInstrumentPresent) noexcept
{
    const auto cellEffects = correctEffects(originalCellEffects, isNotePresent, isInstrumentPresent);

    // Reorders the effects. Puts them first in a vector.
    auto indexToCellEffect = cellEffects.getExistingEffects();
    std::vector<CellEffect> cellEffectsCollection;
    cellEffectsCollection.reserve(indexToCellEffect.size());
    for (auto&[index, cellEffect] : indexToCellEffect) {
        cellEffectsCollection.push_back(cellEffect);
    }

    // Sorts the list.
    static const std::unordered_map<Effect, int> effectToOrder =
    {
            { Effect::reset                       , 0 },
            { Effect::legato                      , 1 },
            { Effect::volume                      , 2 },
            { Effect::volumeIn                    , 3 },
            { Effect::volumeOut                   , 4 },
            { Effect::pitchTable                  , 5 },
            { Effect::pitchUp                     , 6 },
            { Effect::pitchDown                   , 7 },
            { Effect::pitchGlide                  , 8 },
            { Effect::fastPitchUp                 , 9 },
            { Effect::fastPitchDown               , 10 },
            { Effect::arpeggioTable               , 11 },
            { Effect::arpeggio3Notes              , 12 },
            { Effect::arpeggio4Notes              , 13 },
            { Effect::forceInstrumentSpeed        , 14 },
            { Effect::forceArpeggioSpeed          , 15 },
            { Effect::forcePitchTableSpeed        , 16 },
    };
    jassert(effectToOrder.size() == static_cast<size_t>(Effect::lastEffect2_0));

    std::sort(cellEffectsCollection.begin(), cellEffectsCollection.end(), [&](const CellEffect& left, const CellEffect& right) -> bool {
        const auto itLeft = effectToOrder.find(left.getEffect());
        const auto itRight = effectToOrder.find(right.getEffect());
        if ((itLeft == effectToOrder.cend()) || (itRight == effectToOrder.cend())) {
            jassertfalse;       // Should never happen! The effect is not in the map above!
            return false;
        }

        return (itLeft->second < itRight->second);
    });

    // Creates the final CellEffects.
    CellEffects sortedCellEffects;
    for (auto& cellEffect : cellEffectsCollection) {
        const auto success = sortedCellEffects.addEffect(cellEffect.getEffect(), cellEffect.getEffectLogicalValue());
        jassert(success); (void)success;
    }

    return sortedCellEffects;
}

}   // namespace arkostracker
