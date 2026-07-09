#include <utility>

#include "Parameter.h"

namespace arkostracker
{

const juce::String Parameter::numbersOnly = "-0123456789";                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String Parameter::hexadecimalOnly = "0123456789abcdefABCDEF";         // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String Parameter::floatOnly = "-0123456789.";                         // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String Parameter::hexadecimalTagLowerCase = "0x";                     // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String Parameter::hexadecimalTagUpperCase = "0X";                     // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

Parameter::Parameter(ParameterType pParameterType, juce::String pDescription) noexcept :
    description(std::move(pDescription)),
    value(),
    parameterType(pParameterType),
    present(false)
{
}

const juce::String& Parameter::getDescription() const noexcept
{
    return description;
}

void Parameter::setValue(const juce::String& newValue) noexcept
{
    value = newValue;
    present = true;
}

bool Parameter::isPresent() const noexcept
{
    return present;
}

juce::String Parameter::getValueAsString() const noexcept
{
    return value;
}

int Parameter::getValueAsInteger() const noexcept
{
    // Is it a hexadecimal number?
    bool isHexadecimal;         // NOLINT(*-init-variables)
    const auto result = hexadecimalStringToInt(value, isHexadecimal);
    if (isHexadecimal) {
        return result;
    }

    // Maybe Integer. No check performed.
    return value.getIntValue();
}

int Parameter::getValueAsIntegerWithFailureTest(bool& successOut) const noexcept
{
    successOut = isParameterAnInteger();
    return getValueAsInteger();
}

float Parameter::getValueAsFloat() const noexcept
{
    // No check performed.
    return value.getFloatValue();
}

int Parameter::hexadecimalStringToInt(const juce::String& string, bool& successOut) noexcept
{
    successOut = false;
    auto result = 0;

    // Signed? If yes, inverts it and removes it from the String.
    bool minus;    // NOLINT(*-init-variables)
    juce::String newString;
    if (string.startsWith("-")) {
        newString = string.substring(1);
        minus = true;
    } else {
        newString = string;
        minus = false;
    }

    const auto hexadecimalTagLength = hexadecimalTagLowerCase.length();
    jassert(hexadecimalTagUpperCase.length() == hexadecimalTagLowerCase.length());
    if (newString.startsWith(hexadecimalTagLowerCase) || newString.startsWith(hexadecimalTagUpperCase)) {
        // Checks for the value after the tag, it must contain only hexadecimal digits.
        const auto valueWithoutTag = newString.substring(hexadecimalTagLength);
        const juce::StringRef stringRef(hexadecimalOnly);
        if (valueWithoutTag.containsOnly(stringRef)) {
            result = valueWithoutTag.getHexValue32();
            successOut = true;
        }
    }

    // Put the sign back, if any.
    if (minus) {
        result = -result;
    }

    return result;
}

bool Parameter::isParameterAnInteger() const noexcept
{
    // Hexadecimal? We don't care about the result itself.
    bool isHexadecimal;    // NOLINT(*-init-variables)
    hexadecimalStringToInt(value, isHexadecimal);
    if (isHexadecimal) {
        return true;
    }
    // Not hexadecimal. Integer?
    const juce::StringRef stringRef(numbersOnly);
    return value.containsOnly(stringRef);
}

bool Parameter::isParameterAFloat() const noexcept
{
    // Decimal only.
    // Only one "." at max.
    if (value.indexOfChar(floatDot) != value.lastIndexOfChar(floatDot)) {
        return false;
    }

    const juce::StringRef stringRef(floatOnly);
    return value.containsOnly(stringRef);
}

bool Parameter::doesMatchParameterType() const noexcept
{
    bool result;    // NOLINT(*-init-variables)
    switch (parameterType) {
    case ParameterType::string:
        result = true;      // Always right!
        break;
    case ParameterType::integer:
        result = isParameterAnInteger();
        break;
    case ParameterType::floatNumber:
        result = isParameterAFloat();
        break;
    default:
        jassertfalse;       // Shouldn't happen.
        result = false;
        break;
    }

    return result;
}

}   // namespace arkostracker
