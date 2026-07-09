#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker
{

/**
 * Float number as used on CPC: a 16-bit integer, plus an 8-bit for the decimal part.
 * The integer part can be limited so that it cannot go further 0xffff, and not below 0.
 */
class FpFloat
{
public:
    /**
     * Builds a float.
     * @param integerPart the integer part.
     * @param decimalPart the decimal part. Only from 0 to 255. If out of boundary, only the 8 less significant bits are kept.
     * @return the number.
     */
    explicit FpFloat(uint16_t integerPart = 0U, int decimalPart = 0);

    /** @return the integer part. */
    uint16_t getIntegerPart() const noexcept;
    /** @return the decimal part, from 0 to 0xff. */
    int getDecimalPart() const noexcept;

    /**
     * Sets the decimal part.
     * @param decimalPart only from 0 to 255. If out of boundary, keeps only 8 bits.
     */
    void setDecimalPart(int decimalPart) noexcept;

    /**
     * Sets the values. If outside of boundaries, a minimum value is used.
     * @param integerPart the integer part.
     * @param decimalPart the decimal part. Only from 0 to 255. If out of boundary, keeps only 8 bits.
     */
    void setValues(uint16_t integerPart = 0U, int decimalPart = 0) noexcept;

    /**
     * Sets the value from a number containing 4 digits. The less-significant byte is used for the decimal part, the other
     * for the integer part.
     * @param digits the 4 digits.
     */
    void setFromDigits(int digits) noexcept;

    /** Negates the value. */
    void negate() noexcept;

    /** Resets the value (integer and decimal part). */
    void reset() noexcept;

    /** @return true if the number is considered negative (from a CPC point of view). */
    bool isNegative() const noexcept;

    /**
     * Corrects the number so that it stays between 0 and the given integer part.
     * @param maximumIntegerPart the maximum integer part (15 for example).
     */
    void correctFrom0To(unsigned int maximumIntegerPart) noexcept;

    // Operators overloading.
    /** Assigns the integer part from the given one. The decimal part is set to 0. */
    FpFloat& operator=(const uint16_t& other) noexcept;
    bool operator==(const FpFloat& other) const noexcept;
    bool operator!=(const FpFloat& other) const noexcept;
    FpFloat operator+(const FpFloat& other) const noexcept;
    FpFloat operator-(const FpFloat& other) const noexcept;
    FpFloat& operator+=(const FpFloat& other) noexcept;
    FpFloat& operator-=(const FpFloat& other) noexcept;
    FpFloat operator*(int multiplier) const noexcept;

private:
    /**
     * Adds the given numbers and modifies the first number to return the result.
     * @param firstInteger the integer part of the first number. WILL be modified, as the result is put inside.
     * @param firstDecimal the decimal of the first number. WILL be modified, as the result is put inside.
     * @param secondInteger the integer part of the second number.
     * @param secondDecimal the decimal of the second number.
     */
    static void addition(uint16_t& firstInteger, int& firstDecimal, uint16_t secondInteger, int secondDecimal) noexcept;

    /**
     * Subtracts the given numbers and modifies the first number to return the result.
     * @param firstInteger the integer part of the first number. WILL be modified, as the result is put inside.
     * @param firstDecimal the decimal of the first number. WILL be modified, as the result is put inside.
     * @param secondInteger the integer part of the second number.
     * @param secondDecimal the decimal of the second number.
     */
    static void subtraction(uint16_t& firstInteger, int& firstDecimal, uint16_t secondInteger, int secondDecimal) noexcept;

    /**
     * Checks and corrects the given decimal part. If out of boundaries, makes it loop.
     * @param decimalPart the decimal part. Modifies it if necessary.
     * @return an offset to spread on the integer part (negative or positive, according to the decimal value).
     */
    static uint16_t correctDecimalPartReturnOffset(int& decimalPart) noexcept;

    uint16_t integerPart;     // The integer part. Unsigned. We WANT the overflow to be automatically managed.
    int decimalPart;          // The decimal part. Only from 0 to 255, but an int is used to detect overflow easily.
};

FpFloat operator+(uint16_t lhs, const FpFloat& rhs) noexcept;
FpFloat operator+(const FpFloat& lhs, uint16_t rhs) noexcept;
FpFloat operator-(uint16_t lhs, const FpFloat& rhs) noexcept;
FpFloat operator-(const FpFloat& lhs, uint16_t rhs) noexcept;

}   // namespace arkostracker

