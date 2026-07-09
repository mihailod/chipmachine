#pragma once

#include "Effect.h"

namespace arkostracker 
{

/**
 * Class for an effect (effect number and value).
 * It has no knowledge about the effect validity or goal.
 */
class CellEffect
{
public:
    /**
     * Constructor from the raw value. Use buildFromLogicalValue to build from the logical value.
     * @param effect the effect.
     * @param effectRawValue the raw value. This has all the digits, but less may be actually useful.
     */
    explicit CellEffect(Effect effect = Effect::noEffect, int effectRawValue = 0) noexcept;

    /**
     * Builder to construct an effect from its logical value instead of its raw. For example, for a volume, one digit only is enough.
     * @param effect the effect.
     * @param effectLogicalValue the logical value.
     * @return the built effect.
     */
    static CellEffect buildFromLogicalValue(Effect effect, int effectLogicalValue) noexcept;

    bool operator==(const CellEffect& other) const noexcept;
    bool operator!=(const CellEffect& other) const noexcept;
    /**
     * Inferior operator, useful for uniqueness of the CellEffect inside a set.
     * @param other the other item.
     * @return true if the current item must be put before the other one.
     */
    bool operator<(const CellEffect& other) const noexcept;

    /** @return the effect number. */
    Effect getEffect() const noexcept;
    /** @return the effect raw value. 0 if there is none. This has all the digits, but less may be actually useful. This is what is encoded. */
    int getEffectRawValue() const noexcept;
    /**
     * @return the effect logical value. This is the "useful" value. For example, 0xF for a volume, and not 0xF00, contrary to what is encoded.
     * Only the useful digits are also returned.
     */
    int getEffectLogicalValue() const noexcept;

    /** @return true if the effect is present (i.e. it is not a "no effect"). */
    bool isPresent() const noexcept;
        /** @return true if no effect is present, but also if the raw value is 0. */
    bool isClean() const noexcept;

private:
    Effect effect;                      // The effect itself.
    int effectRawValue;                 // The raw value. May have more digits that are actually used.
};

}   // namespace arkostracker

