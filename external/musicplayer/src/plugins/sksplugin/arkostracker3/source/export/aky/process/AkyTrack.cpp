#include "AkyTrack.h"

#include "RegisterBlockOptimizer.h"
#include "RegisterBlockPool.h"

namespace arkostracker 
{

AkyTrack::AkyTrack(const int pId) noexcept :
    id(pId),
    registerBlocks()
{
}

void AkyTrack::addRegisterBlock(std::shared_ptr<RegisterBlock> registerBlock) noexcept
{
    registerBlocks.push_back(std::move(registerBlock));
}

void AkyTrack::findAndSplitBlocksWithRepetitions() noexcept
{
    // Browses through the RegisterBlocks.
    for (auto iterator = registerBlocks.begin(); iterator != registerBlocks.end();) {  // Iterator increase is managed below. // NOLINT(clion-misra-cpp2008-6-5-1)
        const RegisterBlock& registerBlock = **iterator;

        // Try to optimize.
        std::vector<std::unique_ptr<RegisterBlock>> optimizedRegisterBlocks = RegisterBlockOptimizer::findAndSplitBlocksWithRepetitions(registerBlock);
        // Did we succeed?
        if (optimizedRegisterBlocks.size() > 1U) {
            // Success. Replaces the current one with the received ones.
            // Deletes the current Block.
            iterator = registerBlocks.erase(iterator, iterator + 1);

            for (auto& optimizedRegisterBlock : optimizedRegisterBlocks) {
                // Inserts the new Blocks instead.
                std::shared_ptr sharedRegisterBlock(std::move(optimizedRegisterBlock));
                iterator = registerBlocks.insert(iterator, std::move(sharedRegisterBlock));
                ++iterator;
            }
        } else {
            ++iterator;         // Iteration increase managed here, else it conflicts with the increase just above.
        }
    }
}

void AkyTrack::fillPoolWithBlocks(RegisterBlockPool& registerBlockPool) noexcept
{
    for (const auto& registerBlock : registerBlocks) {
        registerBlockPool.addRegisterBlock(registerBlock);
    }
}

int AkyTrack::getRegisterBlockCount() const noexcept
{
    return static_cast<int>(registerBlocks.size());
}

std::pair<int, RegisterBlock*> AkyTrack::getRegisterBlockAndDuration(const int index) const noexcept
{
    jassert(index < static_cast<int>(registerBlocks.size()));
    RegisterBlock* registerBlock = registerBlocks.at(static_cast<size_t>(index)).get();
    return std::make_pair(registerBlock->getSize(), registerBlock);
}

int AkyTrack::getId() const noexcept
{
    return id;
}

}   // namespace arkostracker
