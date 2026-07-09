#pragma once

namespace arkostracker 
{

/** Simple holder of the data of an effect digit (location, digit). */
class EffectDigit
{
public:
    /** Default constructor, useful for Optionals. */
    EffectDigit() noexcept;

    /**
     * Constructor.
     * @param effectIndex the index of the effect (0-3).
     * @param digitIndex the digit index (0-2).
     * @param value the value (0-15).
     */
    EffectDigit(int effectIndex, int digitIndex, int value) noexcept;

    const int effectIndex;
    const int digitIndex;
    const int value;
};



}   // namespace arkostracker

