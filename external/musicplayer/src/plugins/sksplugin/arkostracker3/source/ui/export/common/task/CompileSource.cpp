#include "CompileSource.h"

#include "../../../../../thirdParty/rasm/rasm.h"

namespace arkostracker 
{

std::unique_ptr<juce::MemoryBlock> CompileSource::compile(const juce::MemoryBlock& sourceMemoryBlock) noexcept
{
    // Gets the data from the inner OutputStream.

    const auto dataInLength = static_cast<int>(sourceMemoryBlock.getSize());
    const auto* dataIn = static_cast<const char*>(sourceMemoryBlock.getData()); // NOLINT(clion-misra-cpp2008-5-2-8)

    // Creates a pointer of pointer. Rasm will make it point to a new buffer.
    unsigned char* dataOutPt = nullptr;

    auto lengthOut = 0;
    // Assembles the source.
    const auto result = RasmAssemble(dataIn, dataInLength, &dataOutPt, &lengthOut);

    jassert(result == 0);
    if (result != 0) {
        free(dataOutPt);        // NOLINT(*-no-malloc)
        jassertfalse;           // Assembling failed.
        return nullptr;
    }

    // Puts the binary result into the output MemoryBlock.
    const auto sizeToCopy = static_cast<size_t>(lengthOut);
    auto outputMemoryBlock = std::make_unique<juce::MemoryBlock>(sizeToCopy);       // Important to set the size first, else copyFrom won't do anything.
    outputMemoryBlock->copyFrom(dataOutPt, 0, sizeToCopy);

    free(dataOutPt);            // NOLINT(*-no-malloc)

    return (outputMemoryBlock->getSize() == 0U) ? nullptr : std::move(outputMemoryBlock);
}


}   // namespace arkostracker

