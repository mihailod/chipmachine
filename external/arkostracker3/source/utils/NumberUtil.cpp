#include "NumberUtil.h"

namespace arkostracker 
{
const juce::String NumberUtil::signedHexChars = "0123456789abcdefABCDEF-";      // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)

const std::array<juce::String, 16> NumberUtil::digitToHexadecimalString = { // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
        "0",
        "1",
        "2",
        "3",
        "4",
        "5",
        "6",
        "7",
        "8",
        "9",
        "A",
        "B",
        "C",
        "D",
        "E",
        "F"
};

juce::String NumberUtil::toHexDigit(const int number) noexcept
{
    const auto positiveNumber = (number < 0) ? static_cast<unsigned int>(-number) : static_cast<const unsigned int>(number);
    return digitToHexadecimalString[positiveNumber & 15U];
}

juce::String NumberUtil::toHexByte(const int originalNumber) noexcept
{
    // Inverts the sign if negative.
    const auto usedNumber = (originalNumber < 0) ? static_cast<unsigned int>(-originalNumber) : static_cast<const unsigned int>(originalNumber);

    const auto firstDigit = usedNumber & 15U;
    const auto secondDigit = (usedNumber >> 4U) & 15U;

    juce::String str;
    str.append(digitToHexadecimalString.at(secondDigit), 1);
    str.append(digitToHexadecimalString.at(firstDigit), 1);
    return str;
}

juce::String NumberUtil::toHexThreeDigits(const int originalNumber) noexcept
{
    // Inverts the sign if negative.
    const auto number = (originalNumber < 0) ? static_cast<unsigned int>(-originalNumber) : static_cast<const unsigned int>(originalNumber);

    const auto firstDigit = number & 15U;
    const auto secondDigit = (number >> 4U) & 15U;
    const auto thirdDigit = (number >> 8U) & 15U;

    auto str(digitToHexadecimalString[thirdDigit]);
    str.append(digitToHexadecimalString[secondDigit], 1);
    str.append(digitToHexadecimalString[firstDigit], 1);
    return str;
}

juce::String NumberUtil::toHexFourDigits(const int originalNumber, const bool lowerCase) noexcept
{
    // Inverts the sign if negative.
    const auto number = (originalNumber < 0) ? static_cast<unsigned int>(-originalNumber) : static_cast<const unsigned int>(originalNumber);

    const auto firstDigit = number & 15U;
    const auto secondDigit = (number >> 4U) & 15U;
    const auto thirdDigit = (number >> 8U) & 15U;
    const auto fourthDigit = (number >> 12U) & 15U;

    auto str(digitToHexadecimalString[fourthDigit]);
    str.append(digitToHexadecimalString[thirdDigit], 1);
    str.append(digitToHexadecimalString[secondDigit], 1);
    str.append(digitToHexadecimalString[firstDigit], 1);
    return lowerCase ? str.toLowerCase() : str;
}

juce::String NumberUtil::toUnsignedHex(const int number) noexcept
{
    jassert(number >= 0);
    return juce::String::toHexString(number).toUpperCase();
}

juce::String NumberUtil::toDecimalString(const int number, const int digitCount) noexcept
{
    // Inverts the sign if negative.
    auto usedNumber = number;
    if (usedNumber < 0) {
        usedNumber = -usedNumber;
    }

    const juce::String numberString(usedNumber);
    auto zerosToAdd = (digitCount - numberString.length());
    juce::String resultString;
    // Adds padding.
    while (zerosToAdd > 0) {
        resultString += '0';
        --zerosToAdd;
    }
    resultString += numberString;

    if (number < 0) {       // Encodes the possible minus sign.
        resultString = "-" + resultString;
    }

    return resultString;
}

juce::String NumberUtil::signedHexToStringWithPrefix(const int originalNumber, const juce::String& prefix, const bool showPlusSign, const bool upperCase) noexcept
{
    // Makes the number positive.
    const auto negative = (originalNumber < 0);
    auto number = negative ? static_cast<unsigned int>(-originalNumber) : static_cast<unsigned int>(originalNumber);

    juce::String result;
    do {
        // Converts the less significant quartet to a String, as long as there are data to encode.
        const auto quartet = (number & 0xfU);
        result = digitToHexadecimalString.at(quartet) + result;
        number = number >> 4U;
    } while (number != 0);

    if (!upperCase) {
        result = result.toLowerCase();
    }

    // Puts the prefix, and the sign if needed.
    if (negative) {
        result = "-" + prefix + result;
    } else {
        const auto signString = showPlusSign ? "+" : juce::String();
        result = signString + prefix + result;
    }

    return result;
}

int NumberUtil::signedHexStringToInt(const juce::String& rawInputText) noexcept
{
    const auto inputText = rawInputText.trim();

    const auto negative = inputText.startsWith("-");
    // Removes the sign, if any.
    const auto textToUse = negative ? inputText.substring(1) : inputText;

    const auto number = textToUse.getHexValue64();

    return static_cast<int>(negative ? -number : number);
}

int NumberUtil::signedHexStringToInt(const juce::String& rawInputText, bool& errorOut) noexcept
{
    const auto inputText = rawInputText.trim();

    const auto negativeIndex = inputText.indexOf("-");
    // Empty or only "-" is not allowed.
    if (inputText.isEmpty() || ((negativeIndex == 0) && inputText.length() == 1)) {
        errorOut = true;
        return 0;
    }

    // "-" allowed only at the beginning, or not present. Only one is allowed.
    if ((negativeIndex > 0) || (inputText.indexOf(negativeIndex + 1, "-") > negativeIndex)) {
        errorOut = true;
        return 0;
    }

    // Only specific chars are allowed.
    if (!inputText.containsOnly(signedHexChars)) {
        errorOut = true;
        return 0;
    }

    errorOut = false;
    return signedHexStringToInt(inputText);
}

int NumberUtil::getFirstNibble(const int number) noexcept
{
    return (number & 0xf);
}

int NumberUtil::getSecondNibble(const int number) noexcept
{
    return (number >> 4) & 0xf;
}

int NumberUtil::getThirdNibble(const int number) noexcept
{
    return (number >> 8) & 0xf;
}

int NumberUtil::getByte(const int number, const int byteIndex) noexcept
{
    return static_cast<int>((static_cast<unsigned int>(number) >> (static_cast<unsigned int>(byteIndex) * 8U) & 0xffU));
}

std::pair<int, int> NumberUtil::toMinutesAndSeconds(const int inputSeconds) noexcept
{
    jassert(inputSeconds >= 0);

    auto minutes = inputSeconds / 60;
    auto seconds = inputSeconds % 60;
    return { minutes, seconds };
}

juce::String NumberUtil::getDisplayMinutesAndSeconds(const int minutes, const int seconds, const bool remaining) noexcept
{
    jassert((seconds >= 0) && (seconds <= 60));
    jassert(minutes >= 0);

    const auto minutesString = juce::String(minutes);
    const auto secondsString = juce::String(seconds).paddedLeft('0', 2);

    return (remaining ? "-" : "") + minutesString + ":" + secondsString;
}

int NumberUtil::replaceDigit(const int originalNumber, const int digitIndex, int digit) noexcept
{
    // Inverts the sign if negative.
    auto number = (originalNumber < 0) ? -originalNumber : originalNumber;

    digit &= 0xf;
    const auto nibbleShift = digitIndex * 4;

    // First, create a mask that is the opposite of what we want to replace.
    auto mask = (0xf << nibbleShift);            // Shifts the nibbles of 4 bits.
    mask = ~mask;       // Now we have the right mask.
    number = ((number & mask) | (digit << nibbleShift));
    return (originalNumber >= 0) ? number : -number;        // Re-inverts the final result, if the original number was negative.
}

int NumberUtil::boolToInt(const bool b) noexcept
{
    return b ? 1 : 0;
}

bool NumberUtil::isInteger(const double number) noexcept
{
    auto intPart = 0.0;
    return juce::exactlyEqual(modf(number, &intPart), 0.0);
}

std::pair<int, int> NumberUtil::calculateBalance(const int nb1, const int nb2, const int balancePercent, const int minimum, const int maximum) noexcept
{
    const auto balancePercentShelf1 = static_cast<double>(balancePercent) * 2.0 / 100.0;        // From 0 to 2.
    const auto balancePercentShelf2 = 2.0 - balancePercentShelf1;

    const auto addOnShelf1 = (balancePercent <= 50);

    const auto isShelf1Lower = balancePercentShelf1 <= balancePercentShelf2;
    const auto lowestBalance = isShelf1Lower ? balancePercentShelf1 : balancePercentShelf2;
    const auto lowestPeriod = std::min(nb1, nb2);
    auto balanceOffset = lowestPeriod - static_cast<int>(lowestPeriod * lowestBalance);

    const auto correction = (lowestPeriod - balanceOffset) < minimum ? minimum : 0;
    balanceOffset -= correction;

    const auto newNb1 = std::min(maximum, nb1 + (addOnShelf1 ? balanceOffset : -balanceOffset));
    const auto newNb2 = std::min(maximum, nb2 + (addOnShelf1 ? -balanceOffset : balanceOffset));

    return { newNb1, newNb2 };
}

}   // namespace arkostracker
