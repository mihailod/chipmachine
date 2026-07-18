#pragma once

#include <vector>

#include <juce_core/juce_core.h>

#include "CommandLineArgumentDescriptor.h"

namespace arkostracker
{

/**
    Parses the given argc/argv into a more user-friendly interface.

    Limitations:
        - Hexadecimal numbers are not signed.
        - Parsing integer/float is only done within an option (but can be parsed via a String by the client of course).
        - 4 parameters after an option (can be easily modified).
*/
class CommandLineParser
{
public:
    enum class ParsingResult : juce::uint8
    {
        parsingOk,
        parsingHelp,
        parsingFailure,
    };

    /**
        Constructor.
        @param argc how many parameters there are, plus the program name.
        @param argv array of zero-terminated arguments.
        @param descriptors the descriptors of every argument that should be found. The client must NOT destroy them before the parsing is over!
        @param description the description (plus the parameters) to display in case there is no argument.
    */
    CommandLineParser(int argc, char* argv[], const std::vector<CommandLineArgumentDescriptor*>& descriptors, juce::String description = juce::String()) noexcept;      // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)

    /**
     * Constructor.
     * @param strings the strings of the arguments, without the program name.
     * @param descriptors the descriptors of every argument that should be found. The client must NOT destroy them before the parsing is over!
     * @param description the description (plus the parameters) to display in case there is no argument.
     */
    CommandLineParser(const juce::StringArray& strings, const std::vector<CommandLineArgumentDescriptor*>& descriptors, juce::String description = juce::String()) noexcept;

    /**
     * Parses the command line. The descriptors given to the constructor are holding the information.
     * The possible errors will be displayed.
     * @param showHelpIfNoArgs if true, if there are no parameters, the description is displayed to the standard output, as well as all the parameters.
     * @param registerHelpParameter if true, a "-h / --help" parameter is registered. Using if will, logically, shows the help.
     * @return parsingOk in case of success. parsingError is returned in case of an error. parsingHelp if no parameters were entered,
     * useful to prevent a program to continue and if the user wanted to display the help.
    */
    ParsingResult parse(bool showHelpIfNoArgs = true, bool registerHelpParameter = false) noexcept;

private:
    static constexpr auto alignmentCharacter = 35;                   // How many characters at the left to align the texts.

    static void displayStringOnXOrSkipLine(int currentX, const juce::String& text) noexcept;
    /**
        Utility method that, if the given String is not empty, considers it an error and its it to the given String collection.
        @param errorsToModify the errors collection. An entry will be added in case of error.
        @param errorString the error String. If empty, there is no error.
        @param errorFlagInOut input error flag. Must have been initialized. In case of error, it is set to true.
    */
    static void addErrorIfNotEmpty(std::vector<juce::String>& errorsToModify, const juce::String& errorString, bool& errorFlagInOut) noexcept;
    /**
        Injects the given value into the first Direct Value Descriptor that can handle it. If not, it is an error.
        @param value the value.
        @return an empty String if everything went fine, else an error description.
    */
    juce::String injectDirectValue(const juce::String& value) const noexcept;

    /**
        Parses a short option, and the possible arguments behind.
        @param optionWithTag the option, still with its tag ("-"). There is at least one letter.
        @return an empty String if everything went fine, else an error description.
    */
    juce::String parseShortOptionArgument(const juce::String& optionWithTag) noexcept;

    /**
        Parses a long option, and the possible arguments behind.
        @param optionWithTag the option, still with its tag ("--"). There is at least one letter.
        @return an empty String if everything went fine, else an error description.
    */
    juce::String parseLongOptionArgument(const juce::String& optionWithTag) noexcept;

    /**
        Parses an option, and the possible arguments behind.
        @param optionWithoutTag the option, without its tag ("-" or "--"). There is at least one letter.
        @param shortOption true if it is a short option.
        @return an empty String if everything went fine, else an error description.
    */
    juce::String parseOptionArgument(const juce::String& optionWithoutTag, bool shortOption) noexcept;

    /**
        Parses the possible arguments of the option of the descriptor. The argumentIndex must NOT be modified after the option argument has been read.
        It will NOT be increased if there are no arguments to parse. If there are arguments, it will be increased but will remain on the last one when leaving.
        This is to stay consistent with the main loop which will increase the index by itself at the end of each iteration.
        @param descriptor the descriptor of the option. Will be updated with the read parameters, if any.
        @return an empty String if everything went fine, else an error description.
    */
    juce::String parseArgumentOfOption(CommandLineArgumentDescriptor* descriptor) noexcept;

    /** Displays the help to the standard output. */
    void displayHelp() const noexcept;

    juce::String description;                                                   // The description to display if there are no arguments.

    std::vector<juce::String> rawArguments;                                     // The raw arguments to parse (without the first argument which is the program name).
    std::vector<CommandLineArgumentDescriptor*> descriptors;                    // The descriptors.

    size_t argumentIndex;                                                       // The index of the currently parsed argument.
};

}   // namespace arkostracker
