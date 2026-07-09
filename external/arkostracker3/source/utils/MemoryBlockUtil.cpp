#include "MemoryBlockUtil.h"

#include "OptionalValue.h"

namespace arkostracker 
{

juce::MemoryBlock MemoryBlockUtil::fromInputStream(juce::InputStream& inputStream, const juce::int64 bytesToRead) noexcept
{
    const auto success = inputStream.setPosition(0);
    jassert(success); (void)success;           // Reset failed?

    juce::MemoryBlock memoryBlock;
    inputStream.readIntoMemoryBlock(memoryBlock,
#if JUCE_WINDOWS && ! JUCE_MINGW
        static_cast<juce::pointer_sized_int>(bytesToRead)       // Windows specific, ssize_t isn't POSIX :(.
#else
        bytesToRead
#endif
    );

    return memoryBlock;
}

std::unique_ptr<juce::MemoryBlock> MemoryBlockUtil::fromFile(const juce::File& inputFile) noexcept
{
    auto outputMemoryBlock = std::make_unique<juce::MemoryBlock>();

    juce::FileInputStream inputStream(inputFile);
    const auto readBytes = inputStream.readIntoMemoryBlock(*outputMemoryBlock);
    if (readBytes <= 0) {
        return nullptr;
    }

    return outputMemoryBlock;
}

juce::String MemoryBlockUtil::extractString(const juce::MemoryBlock& memoryBlock, const juce::int64 offset, const juce::int64 size, bool& success, const bool trim) noexcept
{
    jassert(offset >= 0);
    // Empty String?
    if ((offset == 0) && (size == 0)) {
        success = true;
        return { };
    }

    jassert(size > 0);

    // Checks we don't ask for too much data.
    if (!isMemoryBlockLongEnough(memoryBlock, offset, size)) {
        success = false;
        return { };
    }

    // Builds an array, to avoid manipulating deletion by ourselves...
    juce::Array<char> array;
    array.resize(static_cast<int>(size));

    auto* arrayPtr = array.getRawDataPointer();
    memoryBlock.copyTo(arrayPtr, static_cast<int>(offset), static_cast<size_t>(size));
    // Is it a valid UTF-8/ASCII String?
    if (!juce::CharPointer_ASCII::isValidString(arrayPtr, static_cast<int>(size))) {       // If not valid, do not try to convert to String (would crash).
        success = false;
        return { };
    }

    const juce::String str(arrayPtr, static_cast<size_t>(size));

    success = true;

    return trim ? str.trim() : str;
}

std::vector<juce::String> MemoryBlockUtil::extractStrings(const juce::MemoryBlock& memoryBlock) noexcept
{
    std::vector<juce::String> readLines;

    // Reads and stores the lines as long as we can read the MemoryBlock.
    juce::MemoryInputStream memoryInputStream(memoryBlock, false);
    while (!memoryInputStream.isExhausted()) {
        const auto readString = memoryInputStream.readNextLine();
        readLines.push_back(readString);
    }

    return readLines;
}

unsigned char MemoryBlockUtil::extractUnsignedChar(const juce::MemoryBlock& memoryBlock, const juce::int64 offset, bool& success) noexcept
{
    jassert(offset >= 0);

    // Checks we don't ask for too much data.
    if (!isMemoryBlockLongEnough(memoryBlock, offset, 1)) {
        success = false;
        return 0U;
    }

    success = true;

    return static_cast<unsigned char>(memoryBlock[offset]);
}

int MemoryBlockUtil::extractSignedWord(const juce::MemoryBlock& memoryBlock, const juce::int64 offset, bool& success) noexcept
{
    jassert(offset >= 0);

    // Checks we don't ask for too much data.
    if (!isMemoryBlockLongEnough(memoryBlock, offset, 2)) {
        success = false;
        return 0;
    }

    success = true;

    const auto lsb = static_cast<unsigned char>(memoryBlock[offset]);

    const auto nb = lsb + (static_cast<unsigned int>(memoryBlock[offset + 1]) << 8U);
    return static_cast<int16_t>(nb);        // To manage correctly the possible sign.
}

int MemoryBlockUtil::extractUnsignedWord(const juce::MemoryBlock& memoryBlock, const juce::int64 offset, bool& success, const bool bigEndian) noexcept
{
    jassert(offset >= 0);

    // Checks we don't ask too much data.
    if (!isMemoryBlockLongEnough(memoryBlock, offset, 2)) {
        success = false;
        return 0;
    }

    success = true;

    const auto nb1 = static_cast<unsigned char>(memoryBlock[offset]);
    const auto nb2 = static_cast<unsigned char>(memoryBlock[offset + 1]);

    if (bigEndian) {
        return (static_cast<int>(nb1) << 8) + nb2;
    }
    return (static_cast<int>(nb2) << 8) + nb1;
}

unsigned int MemoryBlockUtil::extractUnsigned32Bits(const juce::MemoryBlock& memoryBlock, const juce::int64 offset, bool& success, const bool bigEndian) noexcept
{
    auto msSuccess = false;
    auto lsSuccess = false;
    const auto msWord = static_cast<unsigned int>(extractUnsignedWord(memoryBlock, offset, msSuccess, bigEndian));
    const auto lsWord = static_cast<unsigned int>(extractUnsignedWord(memoryBlock, offset + 2, lsSuccess, bigEndian));

    // No success? Returns 0.
    success = msSuccess && lsSuccess;
    if (!success) {
        return 0U;
    }

    return bigEndian
           ? (msWord << 16U) + lsWord
           : (lsWord << 16U) + msWord;
}

int MemoryBlockUtil::extractSignedByte(const juce::MemoryBlock& memoryBlock, const juce::int64 offset, bool& success) noexcept
{
    jassert(offset >= 0);

    // Checks we don't ask for too much data.
    if (!isMemoryBlockLongEnough(memoryBlock, offset, 1)) {
        success = false;
        return 0;
    }

    success = true;

    return memoryBlock[offset];
}

bool MemoryBlockUtil::isMemoryBlockLongEnough(const juce::MemoryBlock& memoryBlock, const juce::int64 offset, const juce::int64 size) noexcept
{
    jassert(offset >= 0);
    jassert(size > 0);
    return ((offset + size) <= static_cast<juce::int64>(memoryBlock.getSize()));
}

juce::MemoryBlock MemoryBlockUtil::fromString(const juce::String& string) noexcept
{
    return { string.toUTF8(), static_cast<size_t>(string.length()) };
}

juce::MemoryBlock MemoryBlockUtil::fromStrings(const std::vector<juce::String>& strings) noexcept
{
    juce::MemoryBlock memoryBlock;

    for (const auto& string : strings) {
        auto stringWithRc = string + juce::NewLine::getDefault();
        memoryBlock.append(stringWithRc.toUTF8(), static_cast<size_t>(stringWithRc.length()));
    }

    return memoryBlock;
}

void MemoryBlockUtil::appendString(juce::MemoryBlock& memoryBlock, const juce::String& string) noexcept
{
    const auto stringMemoryBlock = fromString(string);
    memoryBlock.append(stringMemoryBlock.getData(), stringMemoryBlock.getSize());
}

bool MemoryBlockUtil::compare(const juce::MemoryBlock& memoryBlock1, const juce::MemoryBlock& memoryBlock2, const std::unordered_set<int>& errorOffsets,
                              const int tolerance) noexcept
{
    const auto size1 = memoryBlock1.getSize();
    const auto size2 = memoryBlock2.getSize();

    auto success = (size1 == size2);
    if (!success) {
        DBG("Different sizes! " + juce::String(size1) + " vs " + juce::String(size2));
        jassertfalse;
    }

    // Compares each byte.
    for (size_t i = 0U; i < std::min(size1, size2); ++i) {
        const auto val1 = memoryBlock1[i];
        const auto val2 = memoryBlock2[i];
        if (val1 != val2) {
            // Is the error tolerable?
            if (errorOffsets.find(static_cast<int>(i)) != errorOffsets.cend()) {
                if (std::abs(val1 - val2) <= tolerance) {
                    continue;
                }
            }

            DBG("Byte different at index: " + juce::String(i) + ", #" + juce::String::toHexString(val1) + " vs #" + juce::String::toHexString(val2));
            //jassertfalse;       // Useful for debugging.
            success = false;
        }
    }

    return success;
}

juce::MemoryBlock MemoryBlockUtil::fromVector(const std::vector<unsigned char>& inputChars) noexcept
{
    juce::MemoryBlock result;

    std::array<unsigned char, 1U> array; // NOLINT(*-pro-type-member-init,*-member-init)

    // Not optimized at all, who cares?
    for (const auto c : inputChars) {
        array[0U] = c;
        result.append(&array, 1U);
    }

    return result;
}

bool MemoryBlockUtil::replaceBytes(juce::MemoryBlock& memoryBlock, const size_t startOffset, const std::vector<int>& bytesToSearch, const std::vector<int>& bytesToReplaceWith) noexcept
{
    if (bytesToSearch.empty()) {
        return true;
    }
    if (bytesToSearch.size() != bytesToReplaceWith.size()) {
        jassertfalse;       // Wrong parameters!
        return false;
    }

    const auto size = memoryBlock.getSize();
    if (startOffset >= size) {
        jassertfalse;       // Offset to superior to the size!
        return false;
    }

    const auto sequenceLength = bytesToSearch.size();
    OptionalValue<size_t> foundIndex;
    for (auto i = static_cast<unsigned int>(startOffset), lastI = static_cast<unsigned int>(size - sequenceLength); foundIndex.isAbsent() && (i < lastI); ++i) {
        // Same subsequence here?
        auto found = true;
        for (auto i2 = 0U; i2 < sequenceLength; ++i2) {
            if (memoryBlock[i + i2] != static_cast<char>(bytesToSearch[i2])) {
                found = false;
                break;
            }
        }
        if (found) {
            foundIndex = i;
        }
    }

    if (foundIndex.isPresent()) {
        // Same sequence! Replaces.
        for (auto i2 = 0U; i2 < sequenceLength; ++i2) {
            memoryBlock[foundIndex.getValue() + i2] = static_cast<char>(bytesToReplaceWith[i2]);
        }
        return true;
    }

    return foundIndex.isPresent();
}

}   // namespace arkostracker
