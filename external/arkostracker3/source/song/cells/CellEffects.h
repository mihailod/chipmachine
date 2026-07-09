#pragma once

#include "../../utils/OptionalValue.h"
#include "CellEffect.h"
#include "Effect.h"
#include "EffectError.h"

namespace arkostracker 
{

/** Represents effects for a Cell. */
class CellEffects           // TODO TU this.
{
public:
    /** Constructor on an empty object. */
    CellEffects() noexcept;

    /**
     * Constructor with effects.
     * @param effectAndLogicalValues a list of effect and their logical value.
     */
    explicit CellEffects(const std::vector<std::pair<Effect, int>>& effectAndLogicalValues) noexcept;

    /** @return a map linking the effect index to an effect. Only the "present" are returned. */
    std::map<int, CellEffect> getExistingEffects() const noexcept;

    /** @return the non-empty effects, in order. */
    std::vector<CellEffect> getPresentEffects() const noexcept;

    /**
     * Sets an effect. If present, the previous effect is overwritten.
     * A "no effect" is possible because the user may want to set a no effect, yet the value is still here
     * and the user doesn't want it to be removed.
     * @param index the index (0-3).
     * @param cellEffect the effect.
     * @return false if effect index out of bounds and nothing has been added.
     */
    bool setEffect(int index, const CellEffect& cellEffect) noexcept;

    /**
     * @return the effect from the given index. May be "no effect".
     * @param index the index. May be out of bounds, in which case a "no effect" is returned.
     */
    CellEffect getEffect(int index) const noexcept;

    /** Indicates whether the Effects are empty or not. "no effects" are not accounted for. */
    bool isEmpty() const noexcept;
    /** @return true if no effect is present, but also if the raw value is 0. */
    bool isClean() const noexcept;

    /**
     * Erases an effect. May be invalid.
     * @param index the index.
     */
    void clearEffect(int index) noexcept;

    /**
     * @return the CellEffect if the effect is found. They are browsed in order.
     * @param effect the effect to find.
     */
    OptionalValue<CellEffect> find(Effect effect) const noexcept;

    /**
     * @return the CellEffect if the effect is found and its index (or -1). They are browsed in order.
     * @param effect the effect to find.
     */
    std::pair<int, OptionalValue<CellEffect>> findWithIndex(Effect effect) const noexcept;

    /**
     * Browses all the effects. It may be an empty effect. The function is called, with the index and the CellEffect.
     * It is possible to modify the CellEffect.
     * @param operationToApply the operation to apply.
     */
    void performOnEffects(const std::function<void(std::pair<int, CellEffect&>)>& operationToApply) noexcept;

    /**
     * Removes the first effect and value which matches.
     * @param effect the effect.
     * @return true if found and removed.
     */
    bool removeEffect(Effect effect) noexcept;

    /** Clears the whole content. */
    void clear() noexcept;

    /**
     * @return a possible Error for the effect which index is given.
     * @param effectIndex the effect index. May be invalid, in which case no error is returned.
     */
    OptionalValue<EffectError> getError(int effectIndex, bool isNotePresent, bool isInstrumentPresent) const noexcept;

    /**
     * Adds an effect, where there is room.
     * @param effect the Effect.
     * @param logicalValue the logical value (no need to shift).
     * @return false if too many effects and nothing has been added.
     */
    bool addEffect(Effect effect, int logicalValue) noexcept;

    /**
     * Sets a digit of an effect. If invalid, nothing happens but asserts.
     * @param effectIndex the index of the effect (0-3).
     * @param digitIndex the digit index (0-2).
     * @param value the value (0-15).
     */
    void setEffectDigit(int effectIndex, int digitIndex, int value) noexcept;

    /**
     * Sets an effect number. If invalid, nothing happens but asserts.
     * @param effectIndex the index of the effect (0-3).
     * @param effect the effect.
     */
    void setEffectNumber(int effectIndex, Effect effect) noexcept;

    /**
     * Sets the value of an effect.
     * @param effectIndex the effect index. May be invalid.
     * @param rawValue the raw value (3 digits).
     */
    void setEffectValue(int effectIndex, int rawValue) noexcept;

    /**
     * Shifts the effect, in any direction.
     * @param shift positive to shift "to the right".
     */
    void shiftEffects(int shift) noexcept;

     /**
      * @return true if there is an effect at the given index.
      * @param index the index.
      */
    bool hasEffect(int index) const noexcept;

    /**
     * Indicates whether effects are present, and if yes, they are only among the given ones.
     * @param effects the effects we are looking for. Must NOT contain the No Effect!
     * @return true if at least one of the given effect is present.
     */
    bool areEffectsPresentAmong(const std::set<Effect>& effects) const noexcept;

    /**
     * Fills the given CellEffect with a value, at a specific digit.
     * @param cellEffect the cell effect to fill.
     * @param digitIndex the digit index (0-2).
     * @param value the value (0-15).
     * @return the modified CellEffect.
     */
    static CellEffect fillCellEffectWithDigit(const CellEffect& cellEffect, int digitIndex, int value) noexcept;

    /**
     * Fills the given CellEffect with a new Effect.
     * @param cellEffect the cell effect to fill.
     * @param effect the effect.
     * @return the modified CellEffect.
     */
    static CellEffect fillCellEffectWithEffect(const CellEffect& cellEffect, Effect effect) noexcept;

    /** Equal if the effects are exactly the same, in the same order. */
    bool operator==(const CellEffects& rhs) const noexcept;
    bool operator!=(const CellEffects& rhs) const noexcept;

private:
    //static const CellEffect emptyCellEffect;            // An empty Cell Effect to be returned whenever needed.

    std::map<int, CellEffect> indexToEffect;            // ORDERED Map linking an index to its effect. It may be a "no effect" (the user may want to keep them).

    mutable bool dirty;                                 // True whenever a change is done.
    mutable std::unordered_map<int, EffectError> cachedIndexToError;        // Map linking an index to an error. It is cached.
};

}   // namespace arkostracker

