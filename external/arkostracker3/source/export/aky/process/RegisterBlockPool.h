#pragma once

#include <unordered_map>
#include <memory>
#include <set>
#include <vector>

#include "RegisterBlock.h"

namespace arkostracker 
{

/**
 * Pool of RegisterBlocks.
 * Also provides methods to make them unique, gets the software/hardware period as indexes (for optimizations).
 *
 * NOTE: Indexes are NOT used (too slow on the Z80 side), the code has been commented out.
 */
class RegisterBlockPool
{
public:
    /** Constructor. */
    RegisterBlockPool() noexcept;

    /**
     * Adds a RegisterBlock. Its uniqueness will be tested on insertion.
     * @param registerBlock the RegisterBlock to insert.
     */
    void addRegisterBlock(std::shared_ptr<RegisterBlock> registerBlock) noexcept;

    /**
     * Call this when all the RegisterBlocks have been inserted. They will be sorted. This should only be called once, because there is
     * no need to do it again!
     */
    void sortRegisterBlocks() noexcept;

    /**
     * Finds a stored RegisterBlock matching the given one. It might be a longer one.
     * The Blocks of the pool MUST have been sorted from largest to smallest.
     * @param registerBlockToFind the block to find.
     * @return a pointer to the stored RegisterBlock, or nullptr if it couldn't be found.
     */
    RegisterBlock* findUniqueRegisterBlock(const RegisterBlock& registerBlockToFind) const noexcept;

    /** Finds a possible RegisterBlock from its id, among the one to use. */
    RegisterBlock* findUsedRegisterBlockFromId(int registerBlockId) const noexcept;

    /**
     * @return a list of RegisterBlocks that are used in the Tracks.
     * IMPORTANT: They are sorted from largest to smallest.
     */
    std::vector<RegisterBlock*> getUsedRegisterBlocks() const noexcept;

    /** The RegisterBlocks can surely be optimized by making them loop or having them refer to data of each other. */
    void referenceOptimizeSortedEncodedBlocks() noexcept;

private:
    // All the registers blocks, unique.
    std::set<std::shared_ptr<RegisterBlock>> registerBlocks;

    bool sorted;                                                            // True if the Blocks has been sorted.
    std::vector<RegisterBlock*> sortedRegisterBlocks;                       // Blocks, sorted from largest to smallest.
};

}   // namespace arkostracker

