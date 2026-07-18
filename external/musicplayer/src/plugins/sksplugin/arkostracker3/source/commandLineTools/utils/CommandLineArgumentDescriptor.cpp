#include "CommandLineArgumentDescriptor.h"

#include <utility>

namespace arkostracker
{

CommandLineArgumentDescriptor::CommandLineArgumentDescriptor(juce::String pDescription, juce::String pDirectValueDescription, const bool pMandatory) noexcept :
    description(std::move(pDescription)),
    directValueDescription(std::move(pDirectValueDescription)),
    isAnOption(false),
    option(Option::buildEmptyOption()),
    directValue(),
    parameters(),
    present(false),
    mandatory(pMandatory)
{
}

CommandLineArgumentDescriptor::CommandLineArgumentDescriptor(juce::String pDescription, Option pOption, const bool pMandatory,
                                                             const std::vector<Parameter*>& pParameters) noexcept :
    description(std::move(pDescription)),
    directValueDescription(),
    isAnOption(true),
    option(std::move(pOption)),
    directValue(),
    parameters(pParameters),
    present(false),
    mandatory(pMandatory)
{
}

CommandLineArgumentDescriptor CommandLineArgumentDescriptor::buildArgumentWithDirectValue(const juce::String& description, const juce::String& directValueDescription,
                                                                                          bool mandatory) noexcept
{
    return { description, directValueDescription, mandatory };
}

CommandLineArgumentDescriptor CommandLineArgumentDescriptor::buildArgumentWithOption(const juce::String& description, const Option& option, bool mandatory) noexcept
{
    return { description, option, mandatory, std::vector<Parameter*>{ } };
}

CommandLineArgumentDescriptor CommandLineArgumentDescriptor::buildArgumentWithOption(const juce::String& description, const Option& option, const bool mandatory, Parameter& parameter1) noexcept
{
    return CommandLineArgumentDescriptor(description, option, mandatory, std::vector { &parameter1 });
}

CommandLineArgumentDescriptor CommandLineArgumentDescriptor::buildArgumentWithOption(const juce::String& description, const Option& option, const bool mandatory, Parameter& parameter1,
    Parameter& parameter2) noexcept
{
    return CommandLineArgumentDescriptor(description, option, mandatory, std::vector { &parameter1, &parameter2 });
}

CommandLineArgumentDescriptor CommandLineArgumentDescriptor::buildArgumentWithOption(const juce::String& description, const Option& option, const bool mandatory, Parameter& parameter1,
    Parameter& parameter2, Parameter& parameter3) noexcept
{
    return CommandLineArgumentDescriptor(description, option, mandatory, std::vector { &parameter1, &parameter2, &parameter3 });
}

CommandLineArgumentDescriptor CommandLineArgumentDescriptor::buildArgumentWithOption(const juce::String& description, const Option& option, const bool mandatory, Parameter& parameter1,
    Parameter& parameter2, Parameter& parameter3, Parameter& parameter4) noexcept
{
    return CommandLineArgumentDescriptor(description, option, mandatory, std::vector { &parameter1, &parameter2, &parameter3, &parameter4 });
}

bool CommandLineArgumentDescriptor::isPresent() const noexcept
{
    return present;
}

bool CommandLineArgumentDescriptor::isOption() const noexcept
{
    return isAnOption;
}

juce::String CommandLineArgumentDescriptor::getDirectValue() const noexcept
{
    return directValue;
}

void CommandLineArgumentDescriptor::setDirectValue(const juce::String& value) noexcept
{
    directValue = value;
    setAsPresent();
}

bool CommandLineArgumentDescriptor::doesMatchWithStringIfOption(const juce::String& optionWithoutTag, const bool isShortOption) const noexcept
{
    // It should be an option!
    if (!isAnOption) {
        return false;
    }

    return option.doesMatchWithStringIfOption(optionWithoutTag, isShortOption);
}

void CommandLineArgumentDescriptor::setAsPresent() noexcept
{
    jassert(!present);      // Not logical to set to present twice.
    present = true;
}

unsigned int CommandLineArgumentDescriptor::getParameterCount() const noexcept
{
    return static_cast<unsigned int>(parameters.size());
}

Parameter& CommandLineArgumentDescriptor::getParameter(const unsigned int index) const noexcept
{
    jassert(index < getParameterCount());

    return *parameters.at(index);
}

bool CommandLineArgumentDescriptor::hasMissingMandatoryParameter() const noexcept
{
    // Checks the presence.
    return mandatory && !present;
}

const juce::String& CommandLineArgumentDescriptor::getDescription() const noexcept
{
    return description;
}

const juce::String& CommandLineArgumentDescriptor::getDirectValueDescription() const noexcept
{
    return directValueDescription;
}

std::pair<juce::String, juce::String> CommandLineArgumentDescriptor::getOptionNames() const noexcept
{
    return option.getOptionNames();
}

std::vector<juce::String> CommandLineArgumentDescriptor::getParameterDescriptions() const noexcept
{
    // Gets all the non-empty parameters into a Vector.
    std::vector<juce::String> results;

    for (const auto& parameter : parameters) {
        const auto& parameterDescription = parameter->getDescription();
        if (!parameterDescription.isEmpty()) {
            results.push_back(parameterDescription);
        }
    }

    return results;
}

}   // namespace arkostracker
