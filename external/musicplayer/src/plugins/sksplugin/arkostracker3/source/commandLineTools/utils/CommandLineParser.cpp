#include "CommandLineParser.h"

#include <utility>

#include "CommandLineConstants.h"

namespace arkostracker
{

CommandLineParser::CommandLineParser(const int argc, char* argv[], const std::vector<CommandLineArgumentDescriptor*>& pDescriptors, juce::String pDescription) noexcept :     // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)
    description(std::move(pDescription)),
    rawArguments(),
    descriptors(pDescriptors),
    argumentIndex()
{
    // Converts all these crappy arrays into Strings.
    for (auto i = 1; i < argc; ++i) {
        const auto& argument(argv[i]); // NOLINT(*-pro-bounds-pointer-arithmetic)
        rawArguments.emplace_back(argument);
    }
}

CommandLineParser::CommandLineParser(const juce::StringArray& pStrings,
                                     const std::vector<CommandLineArgumentDescriptor*>& pDescriptors,
                                     juce::String pDescription) noexcept :
    description(std::move(pDescription)),
    rawArguments(),
    descriptors(pDescriptors),
    argumentIndex()
{
    for (const auto& str : pStrings) {
        rawArguments.push_back(str);
    }
}

CommandLineParser::ParsingResult CommandLineParser::parse(const bool showHelpIfNoArgs, const bool registerHelpParameter) noexcept
{
    std::vector<juce::String> errors;

    // No argument? Displays the help, if wanted, and stops.
    const auto rawArgumentsCount = rawArguments.size();
    if ((rawArgumentsCount == 0U) && showHelpIfNoArgs) {
        displayHelp();
        return CommandLineParser::ParsingResult::parsingHelp;
    }

    // If one argument and it is -h or --help, if wanted, shows the help and stops.
    if (registerHelpParameter && (rawArgumentsCount == 1U)) {
        auto& argument = rawArguments.at(0U);
        if ((argument == "-h") || (argument == "--help")) {
            displayHelp();
            return CommandLineParser::ParsingResult::parsingHelp;
        }
    }

    // Parses each argument.
    argumentIndex = 0U;
    auto error = false;
    while (!error && (argumentIndex < rawArgumentsCount)) {
        auto& argument = rawArguments.at(argumentIndex);

        // If the argument starts with "-" or "--", looks for the matching option. Makes sure it has more than one character after the tag.
        // First starts with the "--", important!
        if ((argument.startsWith(CommandLineConstants::stringLongOptionTag) && (argument.length() > CommandLineConstants::stringLongOptionTag.length()))) {
            const auto errorString = parseLongOptionArgument(argument);
            addErrorIfNotEmpty(errors, errorString, error);
        } else if ((argument.startsWith(CommandLineConstants::stringShortOptionTag) && (argument.length() > CommandLineConstants::stringShortOptionTag.length()))) {
            const auto errorString = parseShortOptionArgument(argument);
            addErrorIfNotEmpty(errors, errorString, error);
        } else {
            // A direct value. Injects it the first descriptor that accept it.
            const auto errorString = injectDirectValue(argument);
            addErrorIfNotEmpty(errors, errorString, error);
        }

        ++argumentIndex;
    }

    // Checks if all the arguments that are mandatory are present.
    for (const auto& descriptor : descriptors) {
        if (descriptor->hasMissingMandatoryParameter()) {
            addErrorIfNotEmpty(errors, translate("Mandatory parameter is missing: " + descriptor->getDescription()), error);
        }
    }

    // Displays the first error only (it is the most relevant)... Shows the Help first.
    if (!errors.empty()) {
        displayHelp();
        std::cout << juce::translate("Error: ") << errors.at(0U) << "\n";
    }

    return errors.empty() ? CommandLineParser::ParsingResult::parsingOk : CommandLineParser::ParsingResult::parsingFailure;
}

void CommandLineParser::addErrorIfNotEmpty(std::vector<juce::String>& errorsToModify, const juce::String& errorString, bool& errorFlagInOut) noexcept
{
    // If the error String is empty, there is no error.
    const auto error = !errorString.isEmpty();
    if (error) {
        // Error! Adds it to the Strings.
        errorsToModify.push_back(errorString);
    }
    errorFlagInOut = errorFlagInOut || error;
}

juce::String CommandLineParser::injectDirectValue(const juce::String& value) const noexcept
{
    // The matching Descriptor is a Direct Value, and is not present yet.
    for (const auto& descriptor : descriptors) {
        if (!descriptor->isPresent() && !descriptor->isOption()) {
            // Matches.
            descriptor->setDirectValue(value);
            return { };
        }
    }

    return juce::translate("Too many arguments.");
}

juce::String CommandLineParser::parseShortOptionArgument(const juce::String& optionWithTag) noexcept
{
    // Skips the short tag.
    return parseOptionArgument(optionWithTag.substring(CommandLineConstants::stringShortOptionTag.length()), true);
}

juce::String CommandLineParser::parseLongOptionArgument(const juce::String& optionWithTag) noexcept
{
    // Skips the long tag.
    return parseOptionArgument(optionWithTag.substring(CommandLineConstants::stringLongOptionTag.length()), false);
}

juce::String CommandLineParser::parseOptionArgument(const juce::String& optionWithoutTag, const bool shortOption) noexcept
{
    // Parses the options, looking for one that matches.
    for (const auto& descriptor : descriptors) {
        if (descriptor->isOption() && descriptor->doesMatchWithStringIfOption(optionWithoutTag, shortOption)) {
            if (descriptor->isPresent()) {
                // Abnormal: the option is already present!
                return translate("The option " + optionWithoutTag +" has been declared twice.");
            }
            // This option matches. Marks if as "present".
            descriptor->setAsPresent();

            // Any arguments in the option? Checks that they are here.
            auto errorString = parseArgumentOfOption(descriptor);
            // If error, stops.
            if (!errorString.isEmpty()) {
                return errorString;
            }
            return { };
        }
    }

    // Not found.
    return translate("Unknown option: " + optionWithoutTag);
}

juce::String CommandLineParser::parseArgumentOfOption(CommandLineArgumentDescriptor* descriptor) noexcept
{
    jassert(descriptor->isOption());

    // Nothing to do if there are no parameters.
    const auto parameterCount = descriptor->getParameterCount();
    if (parameterCount == 0U) {
        return { };
    }

    // Skips the option, go to the first parameter.
    ++argumentIndex;

    for (auto index = 0U; index < parameterCount; ++index) {
        // Checks the limit!
        if (argumentIndex >= rawArguments.size()) {
            return juce::translate("Not enough parameters given.");
        }
        auto& parameter = descriptor->getParameter(index);
        // Sets the value of the parameter.
        const auto& rawArgument(rawArguments.at(argumentIndex));
        parameter.setValue(rawArgument);
        // Is the parameter valid?
        if (!parameter.doesMatchParameterType()) {
            return translate("The parameter " + rawArgument + " does not fit!");
        }

        // Moves to the next argument, unless we are on the last one (the increment is performed by the caller).
        if (index < (parameterCount - 1U)) {
            ++argumentIndex;
        }
    }

    return { };
}

void CommandLineParser::displayHelp() const noexcept
{
    // Displays the description, if any.
    if (!description.isEmpty()) {
        std::cout << description << "\n\n";
    }

    // First, displays the Direct Value parameters.
    for (const auto& descriptor : descriptors) {
        if (!descriptor->isOption()) {
            // If not option, displays the argument description.
            const auto& argumentDescription = descriptor->getDescription();
            const auto& directValueDescription = descriptor->getDirectValueDescription();
            std::cout << argumentDescription;

            const auto currentX = argumentDescription.length();
            // Aligns the rest, maybe needed to go to the next line.
            displayStringOnXOrSkipLine(currentX, (directValueDescription + "\n"));
            //<< "\t\t\t" << directValueDescription << "\n";
        }
    }

    // Then, the options.
    for (const auto& descriptor : descriptors) {
        if (descriptor->isOption()) {
            // If option, displays the tags.
            juce::String result;
            auto pair = descriptor->getOptionNames();
            // Short tag?
            if (!pair.first.isEmpty()) {
                result = CommandLineConstants::stringShortOptionTag + pair.first;
            }
            // Long tag?
            if (!pair.second.isEmpty()) {
                if (!result.isEmpty()) {
                    result += ", ";
                }
                result += CommandLineConstants::stringLongOptionTag + pair.second;
            }

            // Shows the possible parameters.
            const auto parameterDescriptions = descriptor->getParameterDescriptions();
            for (const auto& parameterDescription : parameterDescriptions) {
                result += " " + parameterDescription;       // The separator is also used to separate from the tags before, handy.
            }

            // If the line is already long, enters a new line.
            std::cout << result;            // Displays what we have.
            displayStringOnXOrSkipLine(result.length(), descriptor->getDescription() + "\n");
        }
    }
}

void CommandLineParser::displayStringOnXOrSkipLine(int currentX, const juce::String& text) noexcept
{
    // Too late for this line? If yes, skips it.
    if (currentX > (alignmentCharacter - 2)) {              // - 2 to have a space.
        std::cout << '\n';
        currentX = 0;
    }

    // Displays the descriptor at the "alignmentCharacter".
    for (; currentX < alignmentCharacter; ++currentX) {
        std::cout << ' ';
    }
    std::cout << text;
}

}   // namespace arkostracker
