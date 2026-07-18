#pragma once

#include <unordered_map>

#include "../../../song/cells/EffectError.h"

namespace arkostracker 
{

class Cell;
class CellEffects;

/** Checks, corrects, normalizes the effects in a Cell. */
class CellEffectsChecker
{
public:
    /**
     * Checks if the given effects are error free.
     * @param effectsToCheck the effects to check.
     * @param isNotePresent true if a note is present. This is to perform further tests. If not known, set to false.
     * @param isInstrumentPresent true if an instrument is present. This is to perform further tests. If not known, set to false.
     * @return the error, for each index. Empty if no error is present.
     */
    static std::unordered_map<int, EffectError> checkEffects(const CellEffects& effectsToCheck, bool isNotePresent, bool isInstrumentPresent) noexcept;

    /**
     * Corrects the given CellEffects from the given Cell.
     * @param cell the cell.
     * @return the effects, corrected or similar.
     */
    static CellEffects correctEffects(const Cell& cell) noexcept;

    /**
     * Corrects the given CellEffects.
     * @param effectsToCorrect the effects to correct.
     * @param isNotePresent true if a note is present. This is to perform further tests. If not known, set to false.
     * @param isInstrumentPresent true if an instrument is present. This is to perform further tests. If not known, set to false.
     * @return the effects, corrected or similar.
     */
    static CellEffects correctEffects(const CellEffects& effectsToCorrect, bool isNotePresent, bool isInstrumentPresent) noexcept;

    /**
     * Normalizes the given CellEffects from its Cell, as a convenience.
     * Removes the effects in error, and order them so that they are always encoded the same way.
     * @param cell the Cell.
     * @return the Cell Effects, corrected, normalized.
     */
    static CellEffects normalize(const Cell& cell) noexcept;

    /**
     * Normalizes the given CellEffects. Removes the effects in error, and order them so that they are always encoded the same way.
     * @param cellEffects the CellEffects.
     * @param isNotePresent true if a note is present. This is to perform further tests. If not known, set to false.
     * @param isInstrumentPresent true if an instrument is present. This is to perform further tests. If not known, set to false.
     * @return the Cell Effects, corrected, normalized.
     */
    static CellEffects normalize(const CellEffects& cellEffects, bool isNotePresent, bool isInstrumentPresent) noexcept;
};

}   // namespace arkostracker
