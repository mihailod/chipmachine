#pragma once

#include <unordered_map>

#include "LinkerBlock.h"
#include "../../../utils/Id.h"

namespace arkostracker 
{

class Song;

/**
 * Container of LinkerBlocks. They are all unique. This is filled by browsing the Patterns of a Subsong.
 * Additionally, a table is built so that, for each Position, the right LinkerBlock can be addressed.
 */
class LinkerBlockContainer
{
public:
    /**
     * Constructor
     * @param song the Song.
     * @param subsongId the ID of the Subsong.
     */
    LinkerBlockContainer(const Song& song, const Id& subsongId) noexcept;

    /**
     * @return the index of the LinkerBlock from the position it is related to.
     * @param positionIndex the position. Must be valid.
     */
    size_t getLinkerBlockIndexFromPosition(int positionIndex) const noexcept;

    /**
     * @return the LinkerBlock from its index. This must be called only when the index is known, via the getLinkerBlockIndexFromPosition method.
     * @param linkerBlockIndex the index of the LinkerBlock. Must be valid!
     */
    const LinkerBlock& getLinkerBlock(int linkerBlockIndex) const noexcept;

    /** @return how many LinkerBlocks there are. */
    int getLinkerBlockCount() const noexcept;

private:
    /**
     * Builds the container. Previously stored data is cleared.
     * @param song the Song.
     * @param subsongId the ID of the Subsong.
     */
    void buildContainer(const Song& song, const Id& subsongId) noexcept;

    std::vector<LinkerBlock> linkerBlocks;      // The linker blocks, uniquely and linearly stored.
    std::unordered_map<int, size_t> positionIndexToLinkerBlockIndex;    // Links a position to the Linker Block to use.
};

}   // namespace arkostracker

