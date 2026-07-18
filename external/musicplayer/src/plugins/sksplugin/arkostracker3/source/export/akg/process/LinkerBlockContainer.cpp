#include "LinkerBlockContainer.h"

#include "../../../song/Song.h"

namespace arkostracker 
{

LinkerBlockContainer::LinkerBlockContainer(const Song& song, const Id& subsongId) noexcept :
    linkerBlocks(),
    positionIndexToLinkerBlockIndex()
{
    buildContainer(song, subsongId);
}

void LinkerBlockContainer::buildContainer(const Song& song, const Id& subsongId) noexcept
{
    linkerBlocks.clear();
    positionIndexToLinkerBlockIndex.clear();

    // TODO The most used must be declared first! Have another positionIndexToLinkerBlockIndex be local, makes the sort.

    // Browses each Position.
    song.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
        for (auto positionIndex = 0, endPosition = subsong.getEndPosition(); positionIndex <= endPosition; ++positionIndex) {
            const auto position = subsong.getPosition(positionIndex);
            const auto& pattern = subsong.getPatternRef(positionIndex);

            AkgPattern akgPattern(position, pattern);

            LinkerBlock newLinkerBlock(akgPattern);

            // Is it unique?
            size_t linkerBlockIndexToEncode = 0;
            bool unique = true;
            for (const LinkerBlock& readLinkerBlock : linkerBlocks) {
                if (readLinkerBlock == newLinkerBlock) {
                    unique = false;     // The same has been found!
                    break;
                }
                ++linkerBlockIndexToEncode;
            }

            if (unique) {
                // The LinkerBlock is unique: let's encode it.
                linkerBlockIndexToEncode = linkerBlocks.size();
                linkerBlocks.push_back(newLinkerBlock);
            }

            // Links the position to the LinkerBlock index, whether it be new or already existing.
            positionIndexToLinkerBlockIndex.insert(std::make_pair(positionIndex, linkerBlockIndexToEncode));
        }
    });
}

size_t LinkerBlockContainer::getLinkerBlockIndexFromPosition(int positionIndex) const noexcept
{
    return positionIndexToLinkerBlockIndex.at(positionIndex);
}

const LinkerBlock& LinkerBlockContainer::getLinkerBlock(int linkerBlockIndex) const noexcept
{
    return linkerBlocks.at(static_cast<size_t>(linkerBlockIndex));
}

int LinkerBlockContainer::getLinkerBlockCount() const noexcept
{
    return static_cast<int>(linkerBlocks.size());
}


}   // namespace arkostracker

