#pragma once

#include <utility>

#include <juce_core/juce_core.h>

namespace arkostracker
{
/**
 * An option in the command line.
 * Can be short ("-h"), long ("--help") or both.
 */
class Option
{
public:
    /** @return an empty Option. Useful if there is no Option, actually! */
    static Option buildEmptyOption() noexcept;

    /**
     * @return an option, short format.
     * @param option the option, without the "-".
     */
    static Option buildShortOption(const juce::String& option) noexcept;

    /**
     * @return an option, long format.
     * @param option the short option, without the "--".
     */
    static Option buildLongOption(const juce::String& option) noexcept;

    /**
     * @return an option, with both short and long format
     * @param shortOption the short option, without the "-".
     * @param longOption the long option, without the "--".
     */
    static Option buildOption(const juce::String& shortOption, const juce::String& longOption) noexcept;

    /**
     * Indicates whether the given short or long option matches the given option.
     * @param optionWithoutTag the option, without the tag ("-" or "--").
     * @param isShortOption true if the short option, false if short.
     * @return true if matches.
     */
    bool doesMatchWithStringIfOption(const juce::String& optionWithoutTag, bool isShortOption) const noexcept;

    /** @return the short and long name of this Option. They may be empty. */
    std::pair<juce::String, juce::String> getOptionNames() const noexcept;

private:
    /**
     * Private constructor.
     * @param shortOption the short option, without the "-". Empty if not used, but must be valid if used.
     * @param longOption the long option, without the "--". Empty if not used, but must be valid if used.
     */
    Option(const juce::String& shortOption, const juce::String& longOption) noexcept;

    juce::String shortOption;                                             // If not empty, the option to put after a "-".
    juce::String longOption;                                              // If not empty, the option to put after a "--".
};

}   // namespace arkostracker
