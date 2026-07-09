#pragma once

#include <memory>
#include <vector>

#include "../../../utils/task/OperationResult.h"

namespace arkostracker 
{

class RegisterBlock;
class CancelProvider;
class TaskProgressListener;

/**
 * Class to optimize the RegisterBlocks, such as when the raw data are encoded, and the common bytes must be shared.
 * This is called as a "reference" optimization.
 *
 * There is also the code for the "split" optimization, but though the idea is good, it is actually not good in practice...
 */
class RegisterBlockOptimizer
{
public:
    /** Prevents instantiation. */
    RegisterBlockOptimizer() = delete;

    /**
     * Optimizes the *sorted* RegisterBlocks from their raw data. The optimization used is the "reference" one (adding "goto" to another RegisterBlock
     * if it fits). This only encodes the right metadata to the Blocks, it doesn't actually change the data.
     * @param sortedRegisterBlocks the RegisterBlocks to optimize. They MUST be sorted from largest to smallest, they MUST have been encoded before.
     * @param cancelProvider a possible provider indicating whether the task must be cancelled or not.
     * @param progressListener a possible Listener to notify a progress.
     * @param progressStartPercent the percent at which the progress starts, if there is a progress listener.
     * @param progressEndPercent the percent at which the progress ends, if there is a progress listener.
     * @return the result of the operation.
     */
    static OperationResult referenceOptimizeSortedEncodedBlocks(std::vector<RegisterBlock*> sortedRegisterBlocks, CancelProvider* cancelProvider,
                                                                TaskProgressListener* progressListener, int progressStartPercent, int progressEndPercent) noexcept;

    /**
     * Tries to split the given RegisterBlock that contain repetitions. This method is called recursively to try to optimize
     * the second created Block, in case the first was successfully split.
     * @param registerBlock the RegisterBlock to optimize. It is not modified.
     * @return the split RegisterBlocks, or only one if it couldn't be.
     */
    static std::vector<std::unique_ptr<RegisterBlock>> findAndSplitBlocksWithRepetitions(const RegisterBlock& registerBlock);

private:
    static const int referrerWindowMinimumSize = 3;                                     // Minimum size for a reference window.

    static const int splitSequenceMinimumSize = 8;                                      // Minimum size for a relevant/"worth it" sequence for the split optimization.
    static const int splitMinimumSize = splitSequenceMinimumSize + 1;                   // Minimum size for a RegisterBlock for the split optimization.

    /**
     * @return true if the source Block can be referring to other. The size must be checked before, as well as the reference/referee status.
     * @param sourceRegisterBlock the source.
     * @param destinationRegisterBlock the destination, which may become a reference.
     * @param sourceIndex the index in the source, without taking the loop in account.
     * @param windowSize the size of the window. This is only a facility, as it is only the source size minus the source index.
     */
    static bool determineBlockReferences(RegisterBlock& sourceRegisterBlock, RegisterBlock& destinationRegisterBlock, int sourceIndex, int windowSize) noexcept;
};

}   // namespace arkostracker

