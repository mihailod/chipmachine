#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker
{

class RasmUtil
{
public:
    /** Prevents instantiation. */
    RasmUtil() = delete;

    /**
     * Adds a variable declaration at the beginning of the source (example: "<variable> = <value>").
     * @param source the MemoryBlock to add the data to.
     * @param variable the name of the variable.
     * @param value the value after the equal.
     */
    static void addVariableDeclaration(juce::MemoryBlock& source, const juce::String& variable, const juce::String& value = "1") noexcept;

    /**
     * @return another MemoryBlock with the "include" commented. They can be either "include "" or "include"", not case-sensitive.
     * @param source the source MemoryBlock.
     * @return the new MemoryBlock.
     */
    static juce::MemoryBlock commentIncludes(const juce::MemoryBlock& source) noexcept;
};

}       // namespace arkostracker
