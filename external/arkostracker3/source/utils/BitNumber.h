#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/**
 * Class to generate an unsigned int (or smaller) from the given bool or numbers.
 * This is handy when generating a byte (for example) composed of groups of bits of specific meaning,
 * such as an assembler player would have to decode.
 *
 * Note that bits/numbers are injected from less-significant bit to more, making the injection more natural.
 */
class BitNumber
{
public:
    /**
     * Constructor.
     * @param mask the mask, to keep only the interesting bits, at the end.
     */
    explicit BitNumber(unsigned int mask = 0xffU) noexcept;

    /**
     * Injects a boolean as a bit, to the next bit.
     * @param b the boolean. If true, 1 is injected, else 0.
     */
    void injectBool(bool b) noexcept;

    /**
     * Injects bits given as String. For example "100". They are encoded naturally:
     * in an empty number, the number would be "100", not "001".
     * @param bits the bits ("101", etc.).
     */
    void injectBits(const juce::String& bits) noexcept;

    /**
     * Injects an number to the next bits, but only keeps a certain amount of bits in the original number.
     * @param i the number to inject.
     * @param bitNumbers how many bit are used in the number (>0).
     */
    template<typename NUMBER_TYPE>
    void injectNumber(NUMBER_TYPE i, const unsigned int bitNumbers) noexcept
    {
        jassert(currentBitIndex < 32);

        // Converts the bit numbers to a mask. Not efficient, maybe use a map?...
        auto mask = 0;
        {
            auto remainingBitNumbers = bitNumbers;
            while (remainingBitNumbers > 0) {
                mask <<= 1;
                mask |= 1;
                --remainingBitNumbers;
            }
        }

        // Injects the ANDed number.
        number |= static_cast<unsigned int>(((i & mask) << currentBitIndex));

        // Goes to next bits. They should still be within limit (or reaching it).
        currentBitIndex += bitNumbers;
        jassert(currentBitIndex <= 32);
    }

    /** @return the number, keeping only the bits from the mask given on construction. */
    int get() const noexcept;

private:
    unsigned int finalMask;                     // The mask to keep only the interesting bits, at the end.
    unsigned int currentBitIndex;               // The current bit index. Starts at 0.
    unsigned int number;                        // The number.
};

}   // namespace arkostracker

