#include "RegisterBlockPool.h"

#include <juce_core/juce_core.h>

namespace arkostracker 
{

RegisterBlockPool::RegisterBlockPool() noexcept :
    registerBlocks(),
    sorted(false),
    sortedRegisterBlocks()
{
}

void RegisterBlockPool::addRegisterBlock(std::shared_ptr<RegisterBlock> registerBlock) noexcept
{
    // Is there a RegisterBlock that already matches this one?
    for (const auto& browsedRegisterBlock : registerBlocks) {
        if (registerBlock->matches(*browsedRegisterBlock)) {
            // A matching RegisterBlock already exists. No need to add this one.
            return;
        }
    }

    // Stores the RegisterBlock, as it is unique.
    registerBlocks.insert(std::move(registerBlock));
}

void RegisterBlockPool::sortRegisterBlocks() noexcept
{
    jassert(!sorted);

    sortedRegisterBlocks.clear();

    // Fills the vector with the unique RegisterBlocks from the Set.
    for (auto& registerBlock : registerBlocks) {
        sortedRegisterBlocks.push_back(registerBlock.get());
    }

    // Sorts the vector, from the largest to smallest.
    const RegisterBlockBiggestToSmallestPredicate registerBlockBiggestToSmallestPredicate;
    std::sort(sortedRegisterBlocks.begin(), sortedRegisterBlocks.end(), registerBlockBiggestToSmallestPredicate);

    sorted = true;
}

RegisterBlock* RegisterBlockPool::findUniqueRegisterBlock(const RegisterBlock& registerBlockToFind) const noexcept
{
    // The Blocks of the pool MUST have been sorted from largest to smallest.
    jassert(sorted);

    for (auto* readRegisterBlock : sortedRegisterBlocks) {
        if (registerBlockToFind.matches(*readRegisterBlock)) {
            return readRegisterBlock;
        }
    }

    // Not found!
    jassertfalse;
    return nullptr;
}

RegisterBlock* RegisterBlockPool::findUsedRegisterBlockFromId(const int registerBlockId) const noexcept
{
    for (auto* readRegisterBlock : sortedRegisterBlocks) {
        if (registerBlockId == readRegisterBlock->getId()) {
            return readRegisterBlock;
        }
    }

    // Not found!
    return nullptr;
}

std::vector<RegisterBlock*> RegisterBlockPool::getUsedRegisterBlocks() const noexcept
{
    return sortedRegisterBlocks;
}

void RegisterBlockPool::referenceOptimizeSortedEncodedBlocks() noexcept
{
    // The Blocks of the pool MUST have been sorted from largest to smallest.
    jassert(sorted);

    // Finds the loop for each Block.
    for (auto* registerBlock : sortedRegisterBlocks) {
        registerBlock->determineLoop();
    }
}

}   // namespace arkostracker

