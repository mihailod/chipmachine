#include "RegisterBlockOptimizer.h"

#include "../../../utils/task/CancelProvider.h"
#include "../../../utils/task/TaskProgressListener.h"
#include "EncodedLine.h"
#include "RegisterBlock.h"

namespace arkostracker 
{

OperationResult RegisterBlockOptimizer::referenceOptimizeSortedEncodedBlocks(std::vector<RegisterBlock*> sortedRegisterBlocks, CancelProvider* cancelProvider,
    TaskProgressListener* progressListener, const int progressStartPercent, const int progressEndPercent) noexcept
{
    jassert(progressStartPercent <= progressEndPercent);
    jassert((progressStartPercent >= 0) && (progressStartPercent <= 100));
    jassert((progressEndPercent >= 0) && (progressEndPercent <= 100));

    const auto registerBlockCount = sortedRegisterBlocks.size();
    const auto progressRatio = (static_cast<double>(progressEndPercent) - static_cast<double>(progressStartPercent)) / static_cast<double>(registerBlockCount);

    auto currentProgress = static_cast<double>(progressStartPercent);

    // Browses from the smallest to the largest, trying to make these RegisterBlocks match with larger ones.
    // Excludes the largest RegisterBlock. Note that we don't use a reverse iterator, to allow comparison with the inner iterator.
    for (auto sourceRegisterBlocksIterator = sortedRegisterBlocks.end() - 1; sourceRegisterBlocksIterator != sortedRegisterBlocks.begin(); --sourceRegisterBlocksIterator) {
        // Has the task been cancelled?
        if ((cancelProvider != nullptr) && (cancelProvider->isCanceled())) {
            return OperationResult::cancelled;
        }

        // Notifies the progress (simpler to do it at the beginning, even if not so accurate!).
        if (progressListener != nullptr) {
            const auto newProgress = currentProgress + progressRatio;
            if (static_cast<int>(newProgress) != static_cast<int>(currentProgress)) {
                // A significant change has been made.
                progressListener->onTaskProgressed(static_cast<int>(newProgress), 100);
            }
            currentProgress = newProgress;
        }

        auto& sourceRegisterBlock = **sourceRegisterBlocksIterator;

        // If the size is inferior to 4, there is no need to bother optimizing, we won't gain anything.
        const auto sourceSize = sourceRegisterBlock.getSize();
        if (sourceSize < (referrerWindowMinimumSize + 1)) {     // +1 because the initial state isn't counted.
            continue;
        }

        // Starts with a window size that is as big as the source, minus the initial state.
        auto found = false;
        for (auto windowSize = sourceSize - 1; ((windowSize >= referrerWindowMinimumSize) && !found); --windowSize) {

            // Browses from largest to smallest till the outer iterator is reached.
            for (auto destinationRegisterBlocksIterator = sortedRegisterBlocks.begin();
                (!found && (destinationRegisterBlocksIterator != sourceRegisterBlocksIterator));
                ++destinationRegisterBlocksIterator) {

                auto& destinationRegisterBlock = **destinationRegisterBlocksIterator;

                // Stops if the destination is now too small. Since they are ordered, the next ones will be too small too (not very useful since the inner iterator can't go beyond the outer...).
                if ((destinationRegisterBlock.getSize() - 1) < windowSize) {            // Minus the initial state.
                    break;
                }

                const auto sourceIndex = sourceSize - windowSize;            // Always tries to mutualize the "end" of the Block.
                found = determineBlockReferences(sourceRegisterBlock, destinationRegisterBlock, sourceIndex, windowSize);
            }
        }
    }

    return OperationResult::success;
}

bool RegisterBlockOptimizer::determineBlockReferences(RegisterBlock& sourceRegisterBlock, RegisterBlock& destinationRegisterBlock, const int sourceIndex, const int windowSize) noexcept
{
    // Two blocks match if all the lines of the source match the lines of the destination.
    // HOWEVER, it is very important to make sure that ALL the lines from the source match. This means the original size, not the looped one.
    // Since we work with raw bytes, this is a bit more cumbersome.

    const auto destinationSize = destinationRegisterBlock.getSize();
    jassert(sourceRegisterBlock.getSize() <= destinationSize);

    jassert((sourceIndex + windowSize) <= sourceRegisterBlock.getSize());

    // Compares the source with a sliding window of the destination, as long as possible.
    for (auto destinationIndex = 1; ((destinationIndex + windowSize) <= destinationSize); ++destinationIndex) {  // The initial state is skipped.
        auto slidingIndexInDestination = destinationIndex;
        auto iterationCount = 0;
        // Compares each line within the *raw* data.
        for (auto slidingIndexInSource = sourceIndex; iterationCount < windowSize; ++slidingIndexInSource, ++slidingIndexInDestination) {
            const auto& sourceEncodedLine = sourceRegisterBlock.getEncodedLine(slidingIndexInSource);
            const auto& destinationEncodedLine = destinationRegisterBlock.getEncodedLine(slidingIndexInDestination);
            if (sourceEncodedLine == destinationEncodedLine) {
                ++iterationCount;
            } else {
                break;
            }
        }

        // Did all the lines match?
        if (iterationCount == windowSize) {
            // Match! Stores the data in both the source and destination RegisterBlocks.
            // But is it safe to do so?
            // We must avoid the source to "cut" its data if another Block refers to it! Fortunately, we have the information to avoid that.
            auto safe = true;
            if (sourceRegisterBlock.isReferred()) {
                // Another Block refers the source! Maybe dangerous.
                const auto referredIndex = sourceRegisterBlock.getLastReferredToIndex();
                jassert(referredIndex > 0);
                // Safe if the Goto is encoded after the index the other Block is jumping to.
                // Note the strict superior: this avoid linking a goto to another one.
                safe = (sourceIndex > referredIndex);
            }

            if (safe) {
                sourceRegisterBlock.setReferringTo(sourceIndex, destinationRegisterBlock.getId(), destinationIndex);
                destinationRegisterBlock.setLastReferredToIndex(destinationIndex);
                return true;
            }
        }
    }

    return false;
}

std::vector<std::unique_ptr<RegisterBlock>> RegisterBlockOptimizer::findAndSplitBlocksWithRepetitions(const RegisterBlock& registerBlock)
{
    std::vector<std::unique_ptr<RegisterBlock>> result;

    const auto size = registerBlock.getSize();
    // Since a certain-sized window must be reached (because the optimization costs a bit), a too small Block is rejected.
    if (size < splitMinimumSize) {
        // Makes a copy and returns it.
        auto uniqueBlock = std::make_unique<RegisterBlock>(registerBlock);
        result.push_back(std::move(uniqueBlock));
        return result;
    }

    auto bestGainInLines = 0;
    auto bestGainSplitLine = 0;

    // Tries with a small window and tries to increase it.
    for (auto windowSize = 1; windowSize < (size - 1); ++windowSize) {       // The initial state is skipped.

        // Moves the window from the beginning (minus the initial state) to the end, with a windowSize step.
        for (auto sourceIndex = 1; (sourceIndex + windowSize) <= size; sourceIndex += windowSize) {

            auto mustContinue = true;
            auto tempGain = 0;
            //int tempSplitLine = -1;

            // Moves a destination window from just after the source index, to as much as we can.
            auto lastReachedSuccessfulIndex = -1;
            for (auto destinationIndex = (sourceIndex + windowSize); (mustContinue && ((destinationIndex + windowSize) <= size)); destinationIndex += windowSize) {

                // Uses an offset to compare every line. It stops when one window has reached the limit.
                for (auto offset = 0; (mustContinue && (offset < windowSize) && ((sourceIndex + offset) < size) && ((destinationIndex + offset) < size)); ++offset) {
                    // Does the line match?
                    const auto sourceChannelRegisters = registerBlock.getChannelRegisters(sourceIndex);
                    const auto destinationChannelRegisters = registerBlock.getChannelRegisters(destinationIndex);
                    mustContinue = (sourceChannelRegisters == destinationChannelRegisters);

                    if (mustContinue) {
                        tempGain += 1;
                        lastReachedSuccessfulIndex = destinationIndex;
                    }
                }
            }

            // If the end is reached and the last occurrence is a success, exclude this pass: we don't want to replace the "loop" feature.
            if (!mustContinue && (lastReachedSuccessfulIndex < (size - 1))) {
                if (tempGain > bestGainInLines) {
                    bestGainInLines = tempGain;
                    jassert(lastReachedSuccessfulIndex >= 0);
                    bestGainSplitLine = lastReachedSuccessfulIndex + 1;
                }
            }
        }
    }

    // Have we found somewhere to split?
    if (bestGainInLines >= splitSequenceMinimumSize) {
        jassert(bestGainSplitLine > 0);
        jassert(bestGainSplitLine < size);

        // Creates two RegisterBlocks and stores them.
        auto firstRegisterBlock = registerBlock.extract(0, bestGainSplitLine - 1);
        const auto secondRegisterBlock = registerBlock.extract(bestGainSplitLine, size - 1);
        result.push_back(std::move(firstRegisterBlock));

        // But only the first one! The second will be split again, recursively.
        auto newResult = findAndSplitBlocksWithRepetitions(*secondRegisterBlock);
        jassert(!newResult.empty());
        // Puts them back into the result.
        for (auto& newRegisterBlock : newResult) {
            result.push_back(std::move(newRegisterBlock));
        }
    } else {
        // Nothing found. Makes a copy of the original RegisterBlock and returns it.
        auto uniqueBlock = std::make_unique<RegisterBlock>(registerBlock);
        result.push_back(std::move(uniqueBlock));
        return result;
    }

    return result;
}

}   // namespace arkostracker

