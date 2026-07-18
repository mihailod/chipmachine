#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker
{

/** Possible types of a parameter. */
enum class ParameterType
{
    string,
    integer,
    floatNumber
};

/**
    A parameter after an option in a command line.

    It can be a String, or an integer (possibly in hexadecimal (0x...)).
*/
class Parameter
{
public:
    /**
     * Constructor.
     * @param parameterType the type of the parameter.
     * @param description the description ("<input file>" for example).
     */
    explicit Parameter(ParameterType parameterType, juce::String description = juce::String()) noexcept;

    /** Returns the description ("<input file>" for example). */
    const juce::String& getDescription() const noexcept;

    /**
        Sets the value. Also sets the "present" flag to true.
        @param value the value.
    */
    void setValue(const juce::String& value) noexcept;

    /** Returns the parameter as a String. It is equivalent to the read parameter. */
    juce::String getValueAsString() const noexcept;
    /**
        Attempts to returns the parameter as an integer. Warning, the isParameterAnInteger method should be called first to make sure it is valid.
        An integer, or 0 if invalid.
    */
    int getValueAsInteger() const noexcept;
    /**
        Attempts to returns the parameter as an integer.
        @param successOut true if everything went fine.
        @return the integer, or any value if it failed.
    */
    int getValueAsIntegerWithFailureTest(bool& successOut) const noexcept;

    /**
        Attempts to returns the parameter as float. Warning, the isParameterAFloat method should be called first to make sure it is valid.
        A float, or 0 if invalid.
    */
    float getValueAsFloat() const noexcept;

    /** Indicates whether the parameter is an integer (may be written in hexadecimal). */
    bool isParameterAnInteger() const noexcept;

    /** Indicates whether the parameter is a float. */
    bool isParameterAFloat() const noexcept;

    /** Indicates whether the value matches the parameter type. */
    bool doesMatchParameterType() const noexcept;

    /** Indicates whether the value has been set or not. */
    bool isPresent() const noexcept;

private:
    static const juce::String numbersOnly;                    // Characters for integers only.
    static const juce::String hexadecimalOnly;                // Characters for hexadecimals (without the 0x).
    static const juce::String floatOnly;                      // Characters for floats only.
    static const juce::String hexadecimalTagLowerCase;        // The "0x" tag for hexadecimal.
    static const juce::String hexadecimalTagUpperCase;        // The "0X" tag for hexadecimal.

    static const char floatDot = '.';                   // The dot used in floats.

    /**
        Attempts to convert an hexadecimal String to an integer. It may be signed. It should start with (-)0x or (-)0X.
        @param string the String.
        @param successOut true if the String was converted. The initial value does not matter.
        @return the integer, or 0 if it failed.
    */
    static int hexadecimalStringToInt(const juce::String& string, bool& successOut) noexcept;

    juce::String description;                           // The description ("<input file>" for example).
    juce::String value;                                 // The value, if the parameter is a String.
    ParameterType parameterType;                        // The parameter type.
    bool present;                                       // True if the value has been set.
};

}   // namespace arkostracker
