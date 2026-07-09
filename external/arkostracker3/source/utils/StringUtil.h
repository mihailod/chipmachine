#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** Utility class for Strings. */
class StringUtil
{
public:
    static const juce::String boolTrue;                                     // "true".
    static const juce::String boolFalse;                                    // "false".
    static const juce::String boolYes;                                      // "Yes".
    static const juce::String boolNo;                                       // "No".
    static const juce::String integerNumbers;                               // Integer number restriction.
    static const juce::String integerNumbersSigned;
    static const juce::String decimalNumbers;                               // Decimal number restriction.

    /** Prevents instantiation. */
    StringUtil() = delete;

    /**
     * Converts a string to a boolean. Only "true" (case-insensitive and trimmed) value is considered "true".
     * @param string the string ("true", "false", etc.).
     * @return true if the string matches "true" (or a case-insensitive and trimmed variation).
     */
    static bool stringToBool(const juce::String& string) noexcept;

    /** @return "true" or "false" according to the given boolean. */
    static juce::String boolToString(bool b) noexcept;

    /** @return "Yes" or "No" according to the given boolean. */
    static juce::String boolToYesOrNo(bool b) noexcept;

    /**
     * Extracts a file extension. Simple but effective enough: ".aks", "aks", "*.aks", "hello.aks" all return "aks" or ".aks" if the dot is wanted.
     * @param inputString the input String.
     * @param addDot true to add a dot if there is none, in the output.
     * @return the file extension. Empty if there is none in the input.
     */
    static juce::String extractExtension(const juce::String& inputString, bool addDot) noexcept;

    /** Splits the given String into several ones, when the given separator is found. */
    static std::vector<juce::String> split(const juce::String& input, const juce::String& separator) noexcept;

    /**
     * Converts a String (such as C#4 to a note number, or C#A).
     * @param string the String. May be lower or upper case.
     * @return the note number, or -1 if it couldn't be converted.
     */
    static int stringToNoteNumber(const juce::String& string) noexcept;

    /**
     * Converts a String to an integer.
     * @param string the String to parse.
     * @param success true if success.
     */
    static int stringToInt(const juce::String& string, bool& success) noexcept;

    /**
     * Writes a 0-terminated String to the given Output Stream.
     * @param outputStream the stream.
     * @param text the text.
     */
    static void writeNtString(juce::OutputStream& outputStream, const juce::String& text) noexcept;

private:
    static const juce::String minus;
};

}   // namespace arkostracker

