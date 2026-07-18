#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** A simple circular buffer. */
template<typename DATA, unsigned int SIZE>
class CircularBuffer
{
public:
    /** Constructor. */
    CircularBuffer() noexcept :
            mask(SIZE - 1),
            buffer(SIZE, true),
            nextPosition(0)
    {
        static_assert((SIZE != 0) && ((SIZE & (SIZE - 1)) == 0), "SIZE must be a power of 2!");
    }

    /**
     * Adds a new data.
     * @param data the data to write.
     */
    void addData(const DATA& data) noexcept
    {
        const size_t position = nextPosition;
        buffer[position] = data;

        // Moves forward, cycling if necessary.
        nextPosition = correctPosition(position + 1);
    }

    /** @return the next position in the buffer. */
    size_t getOffsetInBuffer() const noexcept
    {
        return nextPosition;
    }

    /**
     * Reads a data.
     * @param position the position to read. May be out of bounds, even negative!
     * @return the data.
     */
    DATA readData(size_t position) const noexcept
    {
        return buffer[correctPosition(position)];
    }

    /**
     * Fills a given array from the data of this buffer. The cycling is managed automatically.
     * The output buffer must be correctly sized!
     * As a security, if wanting to copy more than the input buffer holds, the size is corrected.
     * @param position the start position. May be out of bounds, it will be corrected.
     * @param dataToFill the data to fill. It must be large enough to contain the data.
     * @param sizeToCopy how many items to copy.
     */
    void fillFrom(size_t position, DATA* dataToFill, size_t sizeToCopy) const noexcept
    {
        // Sanity check.
        if (sizeToCopy > SIZE) {
            jassertfalse;                           // Should never happen! Trying to copy more than the input buffer has!
            sizeToCopy = SIZE;
        }
        position = correctPosition(position);       // Important!

        // Can this be copied in one pass?
        if (position + sizeToCopy <= SIZE) {
            // One pass!
            memcpy(dataToFill, buffer.getData() + position, sizeof(DATA) * sizeToCopy);
            return;
        }
        // Two passes. First pass: from start position to end of input buffer.
        auto firstPassSize = SIZE - position;
        memcpy(dataToFill, buffer.getData() + position, sizeof(DATA) * firstPassSize);

        // Second pass. From 0 to what remains.
        auto secondPassSize = sizeToCopy - firstPassSize;
        memcpy(dataToFill + firstPassSize, buffer.getData() + 0, sizeof(DATA) * secondPassSize);
    }


private:
    size_t mask;                                // Used to make the AND loop.
    juce::HeapBlock<DATA> buffer;
    std::atomic<size_t> nextPosition;           // The next position where to write in the buffer.

    /** @return a corrected position. */
    size_t correctPosition(size_t position) const noexcept {
        return (position & mask);
    }
};

}   // namespace arkostracker
