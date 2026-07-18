#pragma once

#include <juce_core/juce_core.h>
#include <utility>
#include <vector>

#include "Option.h"
#include "Parameter.h"

namespace arkostracker
{

/**
    The descriptor of an argument for a command line, and what data it expects.

    Can be of the forms (none, any or both):
    - option ("-x")
    - long option ("--help")
    Or a direct value (exclusive)).

    If an option, it can be followed by parameters, which can be
    - String
    - int (possibly in hexadecimal (0x...), and signed).
    - float.
*/
class CommandLineArgumentDescriptor
{
public:
    /** Builds an argument with an option without parameters. */
    static CommandLineArgumentDescriptor buildArgumentWithOption(const juce::String& description, const Option& option, bool mandatory) noexcept;
    /** Builds an argument with an option with one parameter ("-i <input file>"). Warning, the Parameter must be kept by the client to be able to retrieve their result. */
    static CommandLineArgumentDescriptor buildArgumentWithOption(const juce::String& description, const Option& option, bool mandatory, Parameter& parameter1) noexcept;
    /** Builds an argument with an option with two parameters ("-r <old file> <new file>"). Warning, the Parameters must be kept by the client to be able to retrieve their result. */
    static CommandLineArgumentDescriptor buildArgumentWithOption(const juce::String& description, const Option& option, bool mandatory, Parameter& parameter1, Parameter& parameter2) noexcept;
    /** Builds an argument with an option with three parameters ("-t <file1> <file2> <file3>"). Warning, the Parameters must be kept by the client to be able to retrieve their result. */
    static CommandLineArgumentDescriptor buildArgumentWithOption(const juce::String& description, const Option& option, bool mandatory, Parameter& parameter1, Parameter& parameter2, Parameter& parameter3) noexcept;
    /** Builds an argument with an option with four parameters ("-t <file1> <file2> <file3> <file4>"). Warning, the Parameters must be kept by the client to be able to retrieve their result. */
    static CommandLineArgumentDescriptor buildArgumentWithOption(const juce::String& description, const Option& option, bool mandatory, Parameter& parameter1,
        Parameter& parameter2, Parameter& parameter3, Parameter& parameter4) noexcept;
    /** Builds an argument that will get a direct value ("myFile.bin"). */
    static CommandLineArgumentDescriptor buildArgumentWithDirectValue(const juce::String& description, const juce::String& directValueDescription, bool mandatory) noexcept;

    /** Indicates whether the argument is present. */
    bool isPresent() const noexcept;
    /** Indicates whether this argument is an option (true if yes, false if direct value). */
    bool isOption() const noexcept;

    /** Returns the direct value. Empty if not present, or not direct value (but an option). */
    juce::String getDirectValue() const noexcept;

    /** Sets a direct value. Makes no sense to call it for an option. This also sets this descriptor to "present". */
    void setDirectValue(const juce::String& value) noexcept;

    /**
        Indicates whether the short or long option matches the given option.
        @param optionWithoutTag the option, without the tag ("-" or "--").
        @param isShortOption true if the short option, false if short.
        @return true if matches. If the descriptor is not an option, always returns false.
    */
    bool doesMatchWithStringIfOption(const juce::String& optionWithoutTag, bool isShortOption) const noexcept;

    /** Marks the descriptor as "present". */
    void setAsPresent() noexcept;

    /** Returns the count of the arguments, if any. */
    unsigned int getParameterCount() const noexcept;

    /** Returns a Parameter. Its index must be valid. */
    Parameter& getParameter(unsigned int index) const noexcept;

    /** Indicates whether one mandatory parameter is missing. */
    bool hasMissingMandatoryParameter() const noexcept;

    /** Returns the description of the argument. */
    const juce::String& getDescription() const noexcept;

    /** Returns the description of the direct value, if any. */
    const juce::String& getDirectValueDescription() const noexcept;

    /** Returns the short and long name of this Option. They may be empty, especially if this Descriptor is not an option! */
    std::pair<juce::String, juce::String> getOptionNames() const noexcept;

    /** Returns the possible parameter descriptions of this Option. The returned list may be empty. */
    std::vector<juce::String> getParameterDescriptions() const noexcept;

private:
    /**
        Private constructor, with no parameters, useful for a direct value.
        @param description the description ("<input file"> for example).
        @param directValueDescription the value description ("The file to process." for example).
        @param mandatory true if mandatory.
    */
    CommandLineArgumentDescriptor(juce::String description, juce::String directValueDescription, bool mandatory) noexcept;

    /**
        Private constructor, for an option.
        @param description the description ("This option A is very useful." for example).
        @param option the option. Fake if a direct value is used.
        @param parameters the possible parameters that must be after. Only valid for options.
        @param mandatory true if mandatory.
    */
    CommandLineArgumentDescriptor(juce::String description, Option option, bool mandatory, const std::vector<Parameter*>& parameters) noexcept;

    juce::String description;                                   // The description of the parameter.
    juce::String directValueDescription;                        // The description of the direct value, if relevant.

    bool isAnOption;                                            // True if an option, false if direct value.

    Option option;                                              // The option. May be unused.
    juce::String directValue;                                   // If not an option, the value entered in the command line.

    std::vector<Parameter*> parameters;                         // The possible parameters. Only for options.

    bool present;                                               // True if the argument is present (known after parsing).
    bool mandatory;                                             // True if mandatory.
};

}   // namespace arkostracker
