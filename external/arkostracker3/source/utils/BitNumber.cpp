#include "BitNumber.h"

namespace arkostracker 
{

BitNumber::BitNumber(unsigned int mask) noexcept :
    finalMask(mask),
    currentBitIndex(0U),
    number(0U)
{
}

void BitNumber::injectBool(bool b) noexcept
{
    jassert(currentBitIndex < 32U);

    const auto bAsInt = b ? 1U : 0U;

    // Injects the bit at the current bit index.
    number |= (bAsInt << currentBitIndex);

    ++currentBitIndex;
}

void BitNumber::injectBits(const juce::String& bits) noexcept
{
    jassert(currentBitIndex < 32U);

    const auto bitCount = bits.length();
    jassert(bitCount > 0);

    // Browses the String from the right to the left.
    for (auto charIndex = (bitCount - 1); charIndex >= 0; --charIndex) {
        jassert((bits[charIndex] == '1') || (bits[charIndex] == '0'));

        // "1" found? Puts a 1 bit.
        if (bits[charIndex] == '1') {
            number |= (1U << currentBitIndex);
        }

        ++currentBitIndex;
    }


    // The bit index should still be within limit (or reaching it).
    jassert(currentBitIndex <= 32U);
}

int BitNumber::get() const noexcept
{
    return static_cast<int>(number & finalMask);
}

}   // namespace arkostracker

