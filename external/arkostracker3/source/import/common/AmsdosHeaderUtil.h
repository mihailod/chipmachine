#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker
{

class AmsdosHeaderUtil
{
public:
    static constexpr size_t amsdosHeaderSize = 128;

    /** Prevents instantiation. */
    AmsdosHeaderUtil() = delete;

    /** @return true if an Amsdos header is present. */
    static bool isHeaderPresent(const juce::MemoryBlock& inputMemoryBlock) noexcept;

    /**
     * Removes the possible Amsdos header.
     * @param inputMemoryBlock the input memory block.
     * @return another memory block, which may be identical if no header is found.
     */
    static juce::MemoryBlock removeHeaderIfNeeded(const juce::MemoryBlock& inputMemoryBlock) noexcept;
};

}   // namespace arkostracker
