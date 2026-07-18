#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker
{

/** Tool top profile Z80 code. */
class ToolZ80Profiler
{
public:
    static int execute(int argc, char* argv[]);     // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)

    /** @return the init nops. Only for test units. */
    static int getInitNops() noexcept;

private:
    static int initNops;

    /**
     * Displays a line just like std::cout does, but adds a carriage-return.
     * @param text the text to display.
     */
    static void cout(const juce::String& text) noexcept;

    /**
     * Displays a line just like std::cerr does, but adds a carriage-return.
     * @param text the text to display.
     */
    static void cerr(const juce::String& text) noexcept;

    /** @return an hex number with a prefix. */
    static juce::String displayHexNumber(int number) noexcept;
};

}   // namespace arkostracker
