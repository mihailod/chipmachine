#include "AmsdosHeaderUtil.h"

namespace arkostracker
{

bool AmsdosHeaderUtil::isHeaderPresent(const juce::MemoryBlock& inputMemoryBlock) noexcept
{
    if (inputMemoryBlock.getSize() < amsdosHeaderSize) {
        return false;
    }

    auto checkSum = 0;
    auto foundNonZero = false;

    constexpr auto checkSumOffset = 67;
    constexpr size_t firstCheckSumOffset = 0;
    constexpr size_t lastCheckSumOffset = checkSumOffset - 1;
    constexpr size_t encodedSizeOffset = 24;

    // The size should be encoded. We don't manage empty files anyway...
    const auto size = inputMemoryBlock[encodedSizeOffset] + inputMemoryBlock[encodedSizeOffset + 1] * 256;
    if (size == 0) {
        return false;
    }

    for (auto offset = firstCheckSumOffset; offset <= lastCheckSumOffset; ++offset) {
        const auto value = static_cast<unsigned char>(inputMemoryBlock[offset]);
        if (value != 0) {
            foundNonZero = true;
        }
        checkSum += value;
    }

    const auto expectedChecksum = static_cast<unsigned char>(inputMemoryBlock[checkSumOffset]) +
        static_cast<unsigned char>(inputMemoryBlock[checkSumOffset + 1]) * 256;
    if (checkSum != expectedChecksum) {
        return false;
    }

    // It *seems* the header is valid. But check for corner cases.
    if (!foundNonZero && expectedChecksum == 0) {
        // Everything is 0. Fishy!
        return false;
    }

    return true;
}

juce::MemoryBlock AmsdosHeaderUtil::removeHeaderIfNeeded(const juce::MemoryBlock& inputMemoryBlock) noexcept
{
    if (!isHeaderPresent(inputMemoryBlock)) {
        return inputMemoryBlock;
    }

    // Removes the header.
    return juce::MemoryBlock(
        static_cast<const char*>(inputMemoryBlock.getData()) + amsdosHeaderSize,
        inputMemoryBlock.getSize() - amsdosHeaderSize);
}

}   // namespace arkostracker
