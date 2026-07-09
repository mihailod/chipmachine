#pragma once

#include <memory>
#include <tuple>
#include <vector>

#include "../../../player/channel/ChannelOutputRegisters.h"
#include "EncodedLine.h"

namespace arkostracker 
{

/**
 * Holds all the ChannelRegisters for one block.
 *
 * WARNING: RegisterBlocks must be created on the same thread (because the ID counter is increased via a static counter). If not, synchronize the counter.
 */
class RegisterBlock
{
public:
    /** Constructor. */
    RegisterBlock() noexcept;

    /** Resets the ID counter. Useful for debugging, to make sure the IDs starts at 0 (useful for comparisons of generated files). */
    static void resetIdCounter() noexcept;

    /** Adds a new ChannelRegisters (a copy if performed). */
    void addChannelRegisters(const ChannelOutputRegisters& channelRegisters) noexcept;

    /**
     * @return how many ChannelRegisters there are in this block. Warning, it does NOT take in account the loop that would optimize it!
     * So it includes all the lines that are lines that were originally found.
     */
    int getSize() const noexcept;

    /**
     * @return how many ChannelRegisters there are in this block. If there is a loop, it DOES take it in account, allowing to
     * encode less lines that this Block is composed of.
     */
    int getSizeWithLoop() const noexcept;

    /**
     * @return whether the given RegisterBlock matches the current one. The input RegisterBlock may be larger!
     * This is only a raw comparison: the loop is NOT compared.
     */
    bool matches(const RegisterBlock& otherRegisterBlock) const noexcept;

    /** @return the ChannelRegisters of the given index. */
    const ChannelOutputRegisters& getChannelRegisters(int index) const noexcept;

    /** Tries to find whether the RegisterBlock loops, populates the looping values. */
    void determineLoop() noexcept;
    /** @return if a loop has been determined. */
    bool isLooping() const noexcept;
    /** @return the index of the loop start if looping, else -1. */
    int getLoopStartIndex() const noexcept;
    /** @return the size of the looping window if looping, else 0. */
    int getLoopSize() const noexcept;

    /** @return the ID of this RegisterBlock. */
    int getId() const noexcept;

    /**
     * Adds and stores an object representing the encoded line. This will be useful for more optimization (raw bytes comparison) before the real encoding.
     * @param encodedLine the encoded line. A copy is performed.
     */
    void addEncodedLine(const EncodedLine& encodedLine) noexcept;

    /**
     * Stores one line to be added to the next added encoded line. Useful for SID. Once added, it is removed.
     * If this is called but a line is already pending, asserts.
     * @param encodedLine the encoded line. A copy is performed.
     */
    void addPendingEncodedLine(const EncodedLine& encodedLine) noexcept;

    /** @return all the encoded lines. */
    const std::vector<EncodedLine>& getEncodedLines() const noexcept;

    /**
     * @return the encoded line at the given index. Important: the index may refer to the same line in case the loop is used.
     * This is especially useful for comparisons.
     * @param lineIndex the lineIndex, from 0 to getSize() - 1, as the loop is not managed.
     */
    const EncodedLine& getEncodedLine(int lineIndex) const noexcept;


    /** @return true if this Block is referring to by another RegisterBlock. */
    bool isReferringTo() const noexcept;
    /**
     * Marks this RegisterBlocks as "referring to". This means this block will not be encoded in its entirety, but a "goto" will be encoded inside its raw data.
     * @param sourceIndex the index where the goto must be encoded, in this RegisterBlock.
     * @param destinationRegisterBlockId the id of the RegisterBlock to go to.
     * @param index the index that is referred to by the other RegisterBlock. >0 because the initial state can not be jumped to!
     */
    void setReferringTo(int sourceIndex, int destinationRegisterBlockId, int index) noexcept;

    /** @return the index where a possible Goto must be encoded (or -1), the id of the RegisterBlock this RegisterBlock might refer to (or -1), and the index (or -1). */
    std::tuple<int, int, int> getReferredFromAndReferredToRegisterBlockIdAndIndex() const noexcept;

    /**
     * Sets the last referred to index. For example, if another RB refers the indexes 5 to 10, this stores 10.
     * This is useful to know if, for example, a loop can be generated or not, because the data after are needed.
     * Nothing happens if a previously higher index was stored.
     */
    void setLastReferredToIndex(int index) noexcept;
    /** @return the index of the "farthest" referred to index, or -1 if there it isn't referred to. */
    int getLastReferredToIndex() const noexcept;

    /** @return the offset in bytes of the line which index is given. It is based on the raw encoded lines, but if a loop is encoded, the index is corrected. */
    int getOffsetInBytesOfLineIndex(int index) const noexcept;

    /** @return whether this RegisterBlock is referred to by another RegisterBlock. */
    bool isReferred() const noexcept;

    /** Extracts a new RegisterBlock from the current one, with a new ID, from the line given (included). */
    std::unique_ptr<RegisterBlock> extract(int firstLineIndex, int lastLineIndex) const;

private:
    static int idCounter;                                          // An ID counter that increases.

    /**
     * @return true if the given loop window is correct for this RegisterBlock.
     * @param windowIndex the index of the window.
     * @param windowSize the size of the window.
     */
    bool determineLoopDoesWindowMatch(int windowIndex, int windowSize) const noexcept;

    int id;                                                                 // An id. It does NOT relate in an order in any way.
    // All the registers used by this block.
    std::vector<ChannelOutputRegisters> channelRegistersList;

    bool loop;                                                              // True if the sound is looping.
    int loopStartIndex;                                                     // If there is a loop, the index of the loop start.
    int loopSize;                                                           // How large is the looping window, if there is a loop (>0).

    /**
        The index of the "farthest" referred to index. For example, if another RB refers the indexes 5 to 10, this stores 10.
        This is useful to know if, for example, a loop can be generated or not, because the data after are needed.
        -1 if this Block is not referred to.
    */
    int lastReferredToIndex;

    int referredFromIndex;                  // >=0 to mark the index where a "goto" must be encoded, or -1.
    int referredToRegisterBlockId;          // >=0 if a RegisterBlock is referred to.
    int referredToRegisterBlockGotoIndex;   // The index inside the RegisterBlock to refer to (>0 because no initial state can be jumped to).

    // The bytes of the encoded lines.
    std::vector<EncodedLine> encodedLines;

    EncodedLine pendingEncodedLine;         // Empty if not used.
};

/** Predicate to sort the RegisterBlocks from largest to smallest. */
class RegisterBlockBiggestToSmallestPredicate
{
public:
    bool operator()(const RegisterBlock* left, const RegisterBlock* right) const noexcept
    {
        const auto leftSize = left->getSize();
        const auto rightSize = right->getSize();
        // First, compares the size.
        if (leftSize > rightSize) {
            return true;
        }
        // If the size is equal, sort by id. This is to make the Set stable.
        if (leftSize == rightSize) {
            return (left->getId() < right->getId());
        }
        return false;
    }
};

}   // namespace arkostracker
