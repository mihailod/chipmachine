#pragma once

#include <juce_core/juce_core.h>

#include <array>

namespace arkostracker 
{

/** Utility class about numbers. */
class NumberUtil
{
public:
    /** Prevents instantiation. */
    NumberUtil() = delete;

    /**
     * Converts a number to a hexadecimal digit. If negative, the positive value is used.
     * @param number the number to convert. Only the less significant digit is used.
     * @return one hexadecimal digit, in upper-case.
     */
    static juce::String toHexDigit(int number) noexcept;

    /**
     * Converts a positive number to an 8-bit hexadecimal byte (without any # or & before). If negative, the sign is inverted.
     * @param number the number to convert. Only the less significant byte is used.
     * @return one hexadecimal byte.
     */
    static juce::String toHexByte(int number) noexcept;

    /**
     * Converts a number to three hexadecimal digits. If negative, the positive value is used.
     * @param number the number to convert. Only the less significant digits is used.
     * @return three hexadecimal digits, in upper-case.
     */
    static juce::String toHexThreeDigits(int number) noexcept;

    /**
     * Converts a number to four hexadecimal digits. If negative, the positive value is used.
     * @param number the number to convert. Only the less significant digits is used.
     * @param lowerCase true to show as lower-case.
     * @return four hexadecimal digits, in upper-case by default.
     */
    static juce::String toHexFourDigits(int number, bool lowerCase = false) noexcept;

    /**
     * @return a String of the number, in hexadecimal, upper case. No negative sign is managed.
     * @param number the number.
     */
    static juce::String toUnsignedHex(int number) noexcept;

    /**
     * Converts a signed number to a hexadecimal String, with a prefix before.
     * @param number the number.
     * @param prefix the prefix. May be empty.
     * @param showPlusSign if true, positive number are added a "+".
     * @param upperCase true to show upper case, false for lower case.
     */
    static juce::String signedHexToStringWithPrefix(int number, const juce::String& prefix = "&", bool showPlusSign = false, bool upperCase = true) noexcept;

    /**
     * Parses a signed hexadecimal String into an int. If it couldn't be deserialized, returns 0. The string is trimmed first.
     * This uses JUCE parsers, which ignores all invalid chars. Strange result can occur for malformed strings. If unsure, use the second version
     * with checks.
     * @param inputText a text, such as "1a2" or "-1", "-f". Case-insensitive.
     * @return the int.
     */
    static int signedHexStringToInt(const juce::String& inputText) noexcept;

    /**
     * Parses a signed hexadecimal String into an int. This method trims and checks for errors.
     * @param inputText a text, such as "1a2" or "-1", "-f". Case-insensitive.
     * @param errorOut on return, false if no error.
     * @return the int, or 0 if error.
     */
    static int signedHexStringToInt(const juce::String& inputText, bool& errorOut) noexcept;

    /**
     * Converts a number to a decimal number, padded with as many zeros as needed. If negative, the sign is added at the beginning.
     * @param number the number to convert.
     * @param digitCount how many digits to pad with 0.
     * @return a String, with at least the digit count.
     */
    static juce::String toDecimalString(int number, int digitCount) noexcept;

    /**
     * @return a binary String ("11100010") from the given number. Only positive numbers should be used.
     * @param number the number.
     * @param maximumDigitCount how many digits to extract.
     */
    template<typename TYPE>
    static juce::String toBinaryString(TYPE number, const int maximumDigitCount = 8) noexcept
    {
        juce::String string;
        for (auto index = 0; index < maximumDigitCount; ++index) {
            string = (((number & 1) == 0) ? "0" : "1") + string;

            number >>= 1;
        }

        return string;
    }

    /**
     * Corrects a number thanks to the given limits. If too low, the lowest limit is used. If too high, the highest limit is used.
     * @param number the number to correct.
     * @param lowLimit the low limit, inclusive.
     * @param highLimit the high limit, inclusive.
     * @param assertionIfOutOfBounds true to assert if out of bounds.
     * @return the number, corrected or not.
     */
    template<typename TYPE>
    [[nodiscard]] static TYPE correctNumber(TYPE number, TYPE lowLimit, TYPE highLimit, const bool assertionIfOutOfBounds = false) noexcept
    {
        if (number < lowLimit) {
            number = lowLimit;
            if (assertionIfOutOfBounds) {
                jassertfalse;
            }
        } else if (number > highLimit) {
            number = highLimit;
            if (assertionIfOutOfBounds) {
                jassertfalse;
            }
        }

        return number;
    }

    /**
     * @return the input number, corrected to be restricted from 0 to the high limit, included,
     * but acting like a modulo if out of bounds. Works with negative signs.
     * @tparam TYPE the type of the input number.
     * @param number the number. If negative, it will be made positive!
     * @param highLimit the high limit. Must be positive.
     */
    template<typename TYPE>
    static TYPE restrictWithLoop(TYPE number, const int highLimit) noexcept
    {
        jassert(highLimit > 0);

        auto newNumber = static_cast<int>(number);
        if (newNumber > highLimit) {
            newNumber = newNumber % (highLimit + 1);
        } else if (newNumber < 0) {
            // Such a mess...
            auto positive = -newNumber % (highLimit + 1);
            if (positive == 0) {
                newNumber = 0;
            } else {
                newNumber = highLimit + 1 - positive;
            }
        }

        return static_cast<TYPE>(newNumber);
    }

    /**
     * Corrects the given number if needed. If above the high limit, set to the low limit (regardless of how much it is bigger).
     * If lower than the low limit, set to the high limit.
     * @tparam TYPE the type of the input number.
     * @param number the number to check.
     * @param lowLimit the low limit. Can be reached.
     * @param highLimit the high limit. Can be reached.
     * @return the number, corrected or not.
     */
    template<typename TYPE>
    static TYPE correctWithLimitedLoop(TYPE number, TYPE lowLimit, TYPE highLimit) noexcept
    {
        if (number < lowLimit) {
            return highLimit;
        }
        if (number > highLimit) {
            return lowLimit;
        }

        return number;
    }

    /**
     * @return true if any of the flags are set in the given number.
     * @tparam TYPE the type of the number. Should be unsigned.
     * @tparam FLAG_TYPE the type of the flag number. Should be unsigned.
     * @param number the number.
     * @param flags the flags (1 << x, or more bits).
     */
    template<typename TYPE, typename FLAG_TYPE>
    static bool isBitPresent(TYPE number, FLAG_TYPE flags) noexcept
    {
        return ((number & flags) != 0);
    }

    /**
     * Converts a value from linear to logarithmic.
     * @param value the linear value.
     * @param maximumInputValue the maximum value for the input number.
     * @param maximumOutputValue the maximum value for the output number (0xf for example to constraint it to 4 bits).
     */
    template<typename TYPE>
    static TYPE toLog(TYPE value, TYPE maximumInputValue, TYPE maximumOutputValue) noexcept
    {
        // See http://www.avrfreaks.net/forum/log-lookup-table.
        if (value < 1) {
            return 0;       // Log(1) is 0.
        }
        return static_cast<TYPE>(std::round(std::log(static_cast<double>(value)) / std::log(maximumInputValue) * static_cast<double>(maximumOutputValue)));
    }


    /** @return the first nibble of a number. */
    static int getFirstNibble(int number) noexcept;
    /** @return the second nibble of a number. */
    static int getSecondNibble(int number) noexcept;
    /** @return the third nibble of a number. */
    static int getThirdNibble(int number) noexcept;

    /**
     * @return a byte, extracted from the number.
     * @param number the number.
     * @param byteIndex the index of the byte (0 is the LSB, 1 is the MSB for a 16-bit number, etc.).
     */
    static int getByte(int number, int byteIndex) noexcept;

    /**
     * Converts seconds into minutes and seconds.
     * @param seconds the seconds.
     * @return the minutes and seconds.
     */
    static std::pair<int, int> toMinutesAndSeconds(int seconds) noexcept;

    /**
     * Displays minutes and seconds (mm:ss). The seconds are padded, the minutes are not.
     * @param minutes the minutes. Must be positive.
     * @param seconds the seconds. Must be positive and within range.
     * @param remaining if true, shows "-" before.
     * @return the String to display.
     */
    static juce::String getDisplayMinutesAndSeconds(int minutes, int seconds, bool remaining = false) noexcept;

    /**
     * Replaces a digit inside a number.
     * @param originalNumber the number to modify. May be negative.
     * @param digitIndex the rank of the digit (0 is the less significant).
     * @param digit the digit (between 0 and 15, both included). Other bits are discarded.
     * @return the new number.
     */
    static int replaceDigit(int originalNumber, int digitIndex, int digit) noexcept;

    /** @return 1 if the given boolean is true. */
    static int boolToInt(bool b) noexcept;

    /**
     * Generates an ID from an array of numbers. The ID is as long as there are values! They are concatenated into one String.
     * This is of course only relevant for short arrays. This method is useful to generate labels for assembler code, for example.
     * If negative, a number will be added a "M" before, else "P". This is only to separate the numbers easily.
     */
    template<typename TYPE>
    static juce::String generateIdFromArray(const std::vector<TYPE>& values)
    {
        juce::String result;
        for (TYPE value : values) {
            if (value < 0) {
                result += "M";
                value = -value;
            } else {
                result += "P";
            }
            result += juce::String(value);
        }

        return result;
    }

    /** @return true if the given number is an integer. */
    static bool isInteger(double number) noexcept;

    /**
     * @return the new numbers after balance.
     * @param nb1 the first number. Positive.
     * @param nb2 the second number. Positive.
     * @param balancePercent the balance, from 0 to 100.
     * @param minimum the minimum the numbers can be.
     * @param maximum the maximum the numbers can be.
     */
    static std::pair<int, int> calculateBalance(int nb1, int nb2, int balancePercent, int minimum, int maximum) noexcept;

private:
    static const juce::String signedHexChars;

    static const std::array<juce::String, 16> digitToHexadecimalString;   // Links a digit to a one-char hexadecimal String.
};

}   // namespace arkostracker
