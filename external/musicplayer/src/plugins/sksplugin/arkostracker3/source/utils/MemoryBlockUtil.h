#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** Utility class about Memory Block. */
class MemoryBlockUtil
{
public:
    /** Prevents instantiation. */
    MemoryBlockUtil() = delete;

    /**
     * @return a MemoryBlock from the given InputStream. It is reset at first.
     * @param inputStream the Input Stream to read from.
     * @param bytesToRead the bytes to read, or -1 to read everything.
     */
    static juce::MemoryBlock fromInputStream(juce::InputStream& inputStream, juce::int64 bytesToRead = -1) noexcept;

    /**
     * @return a MemoryBlock from a File, or nullptr if failed (file not found, etc.).
     * @param inputFile the File to open.
     */
    static std::unique_ptr<juce::MemoryBlock> fromFile(const juce::File& inputFile) noexcept;

    /**
     * Extracts a String from the given MemoryBlock.
     * @param memoryBlock the data to read.
     * @param offset where to read the String, in bytes.
     * @param size the size, in bytes.
     * @param success modified to indicate if everything went fine or not.
     * @param trim true to trim the result String.
     * @return the String. Empty if no success.
     */
    static juce::String extractString(const juce::MemoryBlock& memoryBlock, juce::int64 offset, juce::int64 size, bool& success, bool trim = true) noexcept;

    /**
     * Extracts Strings from the given MemoryBlock. The return carriages are not added to the read lines.
     * @param memoryBlock the data to read.
     * @return the lines.
     */
    static std::vector<juce::String> extractStrings(const juce::MemoryBlock& memoryBlock) noexcept;

    /**
     * Extracts an unsigned char (0-255) from the given MemoryBlock.
     * @param memoryBlock the data to read.
     * @param offset where to read the String, in bytes.
     * @param success modified to indicate if everything went fine or not.
     * @return the value. 0 if error.
     */
    static unsigned char extractUnsignedChar(const juce::MemoryBlock& memoryBlock, juce::int64 offset, bool& success) noexcept;

    /**
     * Extracts a signed word (16 bits, little endian) from the given MemoryBlock.
     * @param memoryBlock the data to read.
     * @param offset where to read the word, in bytes.
     * @param success modified to indicate if everything went fine or not.
     * @return the value. 0 if error.
     */
    static int extractSignedWord(const juce::MemoryBlock& memoryBlock, juce::int64 offset, bool& success) noexcept;

    /**
     * Extracts an unsigned word (16 bits) from the given MemoryBlock.
     * @param memoryBlock the data to read.
     * @param offset where to read the word, in bytes.
     * @param success modified to indicates if everything went fine or not.
     * @param bigEndian true to read a big-endian word, false for little-endian.
     * @return the value. 0 if error.
     */
    static int extractUnsignedWord(const juce::MemoryBlock& memoryBlock, juce::int64 offset, bool& success, bool bigEndian = false) noexcept;

    /**
     * Extracts an unsigned int (32 bits) from the given MemoryBlock.
     * @param memoryBlock the data to read.
     * @param offset where to read the word, in bytes.
     * @param success modified to indicates if everything went fine or not.
     * @param bigEndian true to read a big-endian word, false for little-endian.
     * @return the value. 0 if error.
     */
    static unsigned int extractUnsigned32Bits(const juce::MemoryBlock& memoryBlock, juce::int64 offset, bool& success, bool bigEndian = false) noexcept;

    /**
     * Extracts a signed byte (8 bits) from the given MemoryBlock.
     * @param memoryBlock the data to read.
     * @param offset where to read the byte, in bytes.
     * @param success modified to indicate if everything went fine or not.
     * @return the value. 0 if error.
     */
    static int extractSignedByte(const juce::MemoryBlock& memoryBlock, juce::int64 offset, bool& success) noexcept;

    /**
     * Indicates whether the given MemoryBlock holds the offset plus the size that are given.
     * @param memoryBlock the data to read.
     * @param offset where to read the String, in bytes (>=0).
     * @param size the size, in bytes (>0).
     * @return true if the MemoryBlock can contain the offset plus the size.
     */
    static bool isMemoryBlockLongEnough(const juce::MemoryBlock& memoryBlock, juce::int64 offset, juce::int64 size) noexcept;

    /**
     * Creates a MemoryBlock from the given String. No return carriage is added.
     * @param string the String.
     * @return the MemoryBlock.
     */
    static juce::MemoryBlock fromString(const juce::String& string) noexcept;

    /**
     * Creates a MemoryBlock from the given Strings. Return carriages don't need to be added.
     * @param strings the Strings.
     * @return the MemoryBlock.
     */
    static juce::MemoryBlock fromStrings(const std::vector<juce::String>& strings) noexcept;

    /**
     * Appends a String to the given MemoryBlock.
     * @param memoryBlock the target MemoryBlock.
     * @param string the String to append.
     */
    static void appendString(juce::MemoryBlock& memoryBlock, const juce::String& string) noexcept;

    /**
     * Compares the two given MemoryBlocks. This is useful on debug to know exactly on which index the
     * MemoryBlocks differs, else one should simply use the equal method of the MemoryBlocks.
     * @param memoryBlock1 the first MemoryBlock.
     * @param memoryBlock2 the second MemoryBlock.
     * @param errorOffsets possible offsets where an difference is.
     * @param tolerance on those error offsets (only!), indicates the tolerance.
     * @return true if they are equal.
     */
    static bool compare(const juce::MemoryBlock& memoryBlock1, const juce::MemoryBlock& memoryBlock2,
        const std::unordered_set<int>& errorOffsets = { }, int tolerance = 0) noexcept;

    /**
    * Generates a MemoryBlock from a vector of chars. Not optimized, only for test units.
    * @param inputChars the vector to read.
    * @return the generated MemoryBlock.
    */
    static juce::MemoryBlock fromVector(const std::vector<unsigned char>& inputChars) noexcept;

    /**
     * Replaces bytes with others. The two bytes array must be the same length, else returns false!
     * @return true if everything went fine.
     * @param memoryBlock the memory block to modify.
     * @param startOffset where to start.
     * @param bytesToSearch the bytes to search. If empty, returns true.
     * @param bytesToReplaceWith the bytes to replace with. Must be the same length as the bytes to search!
     */
    static bool replaceBytes(juce::MemoryBlock& memoryBlock, size_t startOffset, const std::vector<int>& bytesToSearch, const std::vector<int>& bytesToReplaceWith) noexcept;
};

}   // namespace arkostracker
