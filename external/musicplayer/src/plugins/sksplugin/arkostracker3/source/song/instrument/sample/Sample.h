#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker
{

/**
 * A sample has immutable data. They are kept in memory, even if unused, for undo/redo.
 * A sample is 8-bit unsigned (0-255).
 * A sample only contains the sample data, not the loop.
 * It can be quickly compared thanks to its internal ID.
 */
class Sample
{
public:
    /** No sample should be larger than 128k. */
    static constexpr auto maximumLength = 128 * 1024;

    /**
     * Constructor.
     * @param data the data.
     */
    explicit Sample(juce::MemoryBlock data) noexcept;

    /** @return the length. */
    int getLength() const noexcept;

    /** @return the data of the sample. */
    const juce::MemoryBlock& getData() const noexcept;

    /**
     * @return the unsigned data (0 to 255) at the given offset. If invalid, asserts and return 0.
     * @param offset the offset. Must be positive, may be invalid.
     */
    unsigned char operator[](int offset) const noexcept;

    bool operator==(const Sample& other) const noexcept;
    bool operator!=(const Sample& other) const noexcept;

private:
    juce::MemoryBlock data;                     // Should be 128k max.
    juce::Uuid uuid;                            // To perform quick comparison.
};

}   // namespace arkostracker
