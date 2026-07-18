#include "StringUtil.h"

#include "NumberUtil.h"

namespace arkostracker 
{

const juce::String StringUtil::boolTrue = "true";                                                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String StringUtil::boolFalse = "false";                                                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String StringUtil::boolYes = "Yes";                                                              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String StringUtil::boolNo = "No";                                                                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String StringUtil::integerNumbers = "0123456789";                                                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String StringUtil::integerNumbersSigned = "0123456789-";                                         // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String StringUtil::decimalNumbers = "0123456789.";                                               // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String StringUtil::minus = "-";                                                                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

bool StringUtil::stringToBool(const juce::String& string) noexcept
{
    return string.trim().equalsIgnoreCase("true");
}

juce::String StringUtil::boolToString(const bool b) noexcept
{
    return b ? boolTrue : boolFalse;
}

juce::String StringUtil::boolToYesOrNo(const bool b) noexcept
{
    return b ? boolYes : boolNo;
}

juce::String StringUtil::extractExtension(const juce::String& inputString, const bool addDot) noexcept
{
    // Is there a dot?
    const juce::StringRef dot = ".";
    const auto dotIndex = inputString.indexOf(dot);
    // Extracts what is after the dot.
    auto extensionWithoutDot = (dotIndex < 0) ? inputString : inputString.substring(dotIndex + 1);

    // Returns it (with a dot if wanted)... Which may be the exact opposite of what we did before. Oh well...
    return addDot ? ("." + extensionWithoutDot) : extensionWithoutDot;
}

std::vector<juce::String> StringUtil::split(const juce::String& input, const juce::String& separator) noexcept
{
    // The separator is empty? If yes, returns the input.
    if (separator.isEmpty()) {
        return { input };
    }
    // Consists only of the separator? Then returns nothing.
    if (input == separator) {
        return { };
    }

    std::vector<juce::String> results;

    const auto size = input.length();

    auto startIndex = 0;

    // Searches for the next separator in the input String.
    auto continueBrowsing = true;
    while (continueBrowsing) {
        const auto separatorIndex = input.indexOf(startIndex, separator);
        if (separatorIndex >= 0) {
            // A separator has been found. The previous substring is a String to extract.
            const auto oneSplit = input.substring(startIndex, separatorIndex);
            if (oneSplit.isNotEmpty()) {
                results.push_back(oneSplit);
            }

            // Moves past the separator and continues.
            startIndex = separatorIndex + separator.length();
        } else {
            continueBrowsing = false;
        }
    }

    // When all is done, is there a split not included?
    if (startIndex < size) {
        const auto oneSplit = input.substring(startIndex, size);
        results.push_back(oneSplit);
    }

    return results;
}

int StringUtil::stringToNoteNumber(const juce::String& string) noexcept
{
    // Security. Should be enough room for "C#3" for example, but "C#11" is not allowed.
    if (string.length() != 3) {
        return -1;
    }

    const auto stringToUse = string.trim().toUpperCase();

    static const std::map<juce::String, int> noteStringToNoteInOctave = {
            { "C-", 0 },
            { "C#", 1 },
            { "D-", 2 },
            { "D#", 3 },
            { "E-", 4 },
            { "F-", 5 },
            { "F#", 6 },
            { "G-", 7 },
            { "G#", 8 },
            { "A-", 9 },
            { "A#", 10 },
            { "B-", 11 }
    };

    // What is the note in the octave?
    const auto iterator = noteStringToNoteInOctave.find(stringToUse.substring(0, 2));
    if (iterator == noteStringToNoteInOctave.cend()) {
        return -1;
    }
    const auto noteInOctave = iterator->second;

    // Gets the octave.
    const auto rawOctave = stringToUse.substring(2, 3);
    auto error = false;
    const auto octave = NumberUtil::signedHexStringToInt(rawOctave, error);
    if (error) {
        return -1;
    }

    return (octave * 12) + noteInOctave;
}

int StringUtil::stringToInt(const juce::String& string, bool& success) noexcept
{
    success = false;

    // Not empty?
    const auto newString = string.trim();
    if (newString.isEmpty()) {
        return 0;
    }

    if (newString.containsOnly(integerNumbersSigned)) {
        // Makes sure there is only one minus (a bit hackish...).
        const auto minusIndex = newString.indexOf(minus);
        if (minusIndex >= 1) {   // Minus, if present, must be at first.
            return 0;
        }
        if ((minusIndex == 0) && (newString.lastIndexOf(minus) != minusIndex)) {
            return 0;
        }

        success = true;
        return string.getIntValue();
    }

    // Some chars are not valid.
    return 0;
}

void StringUtil::writeNtString(juce::OutputStream& outputStream, const juce::String& text) noexcept
{
    outputStream.writeText(text, false, false, nullptr);
    outputStream.writeByte(0);
}

}   // namespace arkostracker
