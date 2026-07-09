#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "RegisterBlock.h"

namespace arkostracker 
{

class RegisterBlockPool;

/** Holds the RegisterBlocks for a Track, to be encoded as AKY. */
class AkyTrack
{
public:
    /**
     * Constructor.
     * @param id an id.
     */
    explicit AkyTrack(int id) noexcept;

    /** Adds a RegisterBlock to the Track. */
    void addRegisterBlock(std::shared_ptr<RegisterBlock> registerBlock) noexcept;

    /**
     * Some Tracks may contain Blocks that contain repetitions: this takes room and can not be optimized by a loop.
     * But they can be split, to allow the loop to do a better job later.
     */
    void findAndSplitBlocksWithRepetitions() noexcept;

    /** Fills the given Pool with the RegisterBlocks of this Track. */
    void fillPoolWithBlocks(RegisterBlockPool& registerBlockPool) noexcept;

    /** @return how many RegisterBlock this Track is composed of. */
    int getRegisterBlockCount() const noexcept;

    /** @return the id of the AkyTrack. */
    int getId() const noexcept;

    /** @return the duration (in frames) and the Register Block from the given index. */
    std::pair<int, RegisterBlock*> getRegisterBlockAndDuration(int index) const noexcept;

private:
    int id;                                                             // The ID of the Track.
    std::vector<std::shared_ptr<RegisterBlock>> registerBlocks;         // The Register Blocks this Track is composed of.
};

}   // namespace arkostracker

