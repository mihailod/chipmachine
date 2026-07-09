#include "RegisterBlock.h"

#include <algorithm>

namespace arkostracker 
{

int RegisterBlock::idCounter = 0;

RegisterBlock::RegisterBlock() noexcept :
    id(idCounter++),                    // Increases the ID counter.
    channelRegistersList(),

    loop(false),
    loopStartIndex(0),
    loopSize(0),

    lastReferredToIndex(-1),
    referredFromIndex(-1),
    referredToRegisterBlockId(-1),
    referredToRegisterBlockGotoIndex(-1),

    encodedLines(),
    pendingEncodedLine()
{
}

void RegisterBlock::resetIdCounter() noexcept
{
    idCounter = 0;
}

int RegisterBlock::getId() const noexcept
{
    return id;
}

void RegisterBlock::addChannelRegisters(const ChannelOutputRegisters& channelRegisters) noexcept
{
    channelRegistersList.push_back(channelRegisters);
}

int RegisterBlock::getSize() const noexcept
{
    return static_cast<int>(channelRegistersList.size());
}

int RegisterBlock::getSizeWithLoop() const noexcept
{
    return loop ? (loopStartIndex + loopSize) : getSize();
}


const ChannelOutputRegisters& RegisterBlock::getChannelRegisters(const int index) const noexcept
{
    jassert(index < static_cast<int>(channelRegistersList.size()));
    return channelRegistersList.at(static_cast<size_t>(index));
}

bool RegisterBlock::matches(const RegisterBlock& otherRegisterBlock) const noexcept
{
    // The input should be at least the same size.
    if (otherRegisterBlock.getSize() < getSize()) {
        return false;
    }

    // Compares the Registers. Stops whenever a mismatch is found. Compares only the necessary lines.
    auto index = 0;
    for (const auto& channelRegisters : channelRegistersList) {
        if (channelRegisters != otherRegisterBlock.getChannelRegisters(index)) {
            return false;
        }

        ++index;
    }

    // Matches!
    return true;
}

bool RegisterBlock::isLooping() const noexcept
{
    return loop;
}
int RegisterBlock::getLoopStartIndex() const noexcept
{
    return loop ? loopStartIndex : -1;
}
int RegisterBlock::getLoopSize() const noexcept
{
    return loopSize;
}

void RegisterBlock::determineLoop() noexcept
{
    // Did we try to determine a loop on an already looping RegisterBlock? Not a problem, but it is strange to do it again!
    jassert(!loop);

    loop = false;
    loopStartIndex = 0;
    loopSize = 0;

    const auto size = getSize();
    if (size < 2) {         // The size: no loop on an initial state.
        return;
    }

    auto bestSize = size;
    std::pair bestWindow(0, 0);       // Index, size.
    auto foundWindow = false;

    // Stretches the window, starting with a size of 1. The best ratio is kept.
    for (auto windowSize = 1; windowSize < (size - 2); ++windowSize) {           // Do not include the initial state.
        auto match = true;
        auto windowIndex = (size - (windowSize * 2));
        auto matchCount = 0;
        while (match && (windowIndex >= 1)) {
            match = determineLoopDoesWindowMatch(windowIndex, windowSize);
            if (match) {
                ++matchCount;
                // Match. Is the theoretical size better (=smaller)?
                const auto newSize = size - (windowSize * matchCount);
                if (newSize < bestSize) {
                    bestSize = newSize;
                    foundWindow = true;
                    bestWindow = std::pair(windowIndex, windowSize);
                }

                windowIndex -= windowSize;
            }
        }
    }

    if (foundWindow) {
        // Is the loop really worth it? No need to loop on the last item of there is only one of it!
        if (bestSize < (size - 2)) {            // A bit empirical, because the data to loop may be bigger than one byte, but we don't know that yet!
            loop = true;
            loopStartIndex = bestWindow.first;
            loopSize = bestWindow.second;
        }
    }
}

bool RegisterBlock::determineLoopDoesWindowMatch(const int windowIndex, const int windowSize) const noexcept
{
    // Compares the window to the last data of the block.
    const auto index = (getSize() - windowSize);
    for (auto shift = 0; shift < windowSize; ++shift) {
        const auto& indexChannelRegister = channelRegistersList.at(static_cast<size_t>(index) + static_cast<size_t>(shift));
        const auto& windowChannelRegister = channelRegistersList.at(static_cast<size_t>(windowIndex) + static_cast<size_t>(shift));
        if (indexChannelRegister != windowChannelRegister) {
            // They do not match! No need to go further.
            return false;
        }
    }

    return true;
}

void RegisterBlock::addEncodedLine(const EncodedLine& encodedLine) noexcept
{
    if (!pendingEncodedLine.isEmpty()) {
        // There is a pending line. Adds the given encodedLine AFTER it.
        // Mixes both so that the pending line, such as SID entry, does not mess up with the indexing of the lines.
        auto newEncodedLine = encodedLine;
        newEncodedLine.insertSequencesAtBeginning(pendingEncodedLine.getEncodedSequences());

        encodedLines.push_back(newEncodedLine);

        pendingEncodedLine = EncodedLine();
    } else {
        encodedLines.push_back(encodedLine);
    }
}

void RegisterBlock::addPendingEncodedLine(const EncodedLine& encodedLine) noexcept
{
    jassert(pendingEncodedLine.isEmpty());     // Must never happen! The previous pending line has not been used!
    pendingEncodedLine = encodedLine;
}

const std::vector<EncodedLine>& RegisterBlock::getEncodedLines() const noexcept
{
    return encodedLines;
}

const EncodedLine& RegisterBlock::getEncodedLine(const int lineIndex) const noexcept
{
    // The encoded lines must be generated first!
    jassert(static_cast<int>(encodedLines.size()) > 0);

    const auto sizeWithLoop = getSizeWithLoop();
    jassert((lineIndex >= 0) && (lineIndex < getSize()));
    jassert(!loop || (sizeWithLoop < getSize()));       // If looping, the size with loop is obviously smaller.

    // If the requested index is below the possible loop, we can safely use the EncodeLine at this index.
    if (lineIndex < sizeWithLoop) {
        return encodedLines.at(static_cast<size_t>(lineIndex));
    }

    // The index does not "exist" anymore since the loop has removed it. The returned EncodedLine must be among the looped lines.
    jassert(loopStartIndex > 0);
    const auto newIndex = ((lineIndex - loopStartIndex) % loopSize) + loopStartIndex;

    jassert(newIndex < sizeWithLoop);

    return encodedLines.at(static_cast<size_t>(newIndex));
}

bool RegisterBlock::isReferred() const noexcept
{
    return (lastReferredToIndex >= 0);
}
int RegisterBlock::getLastReferredToIndex() const noexcept
{
    return lastReferredToIndex;
}
void RegisterBlock::setLastReferredToIndex(const int index) noexcept
{
    lastReferredToIndex = std::max(lastReferredToIndex, index);
}

void RegisterBlock::setReferringTo(const int sourceIndex, const int destinationRegisterBlockId, const int index) noexcept
{
    referredFromIndex = sourceIndex;
    referredToRegisterBlockId = destinationRegisterBlockId;
    referredToRegisterBlockGotoIndex = index;
}

bool RegisterBlock::isReferringTo() const noexcept
{
    return (referredToRegisterBlockId >= 0);
}

std::tuple<int, int, int> RegisterBlock::getReferredFromAndReferredToRegisterBlockIdAndIndex() const noexcept
{
    return { referredFromIndex, referredToRegisterBlockId, referredToRegisterBlockGotoIndex };
}

int RegisterBlock::getOffsetInBytesOfLineIndex(int indexToReach) const noexcept
{
    // If a loop is encoded, the last encoded line is not a "real" line.
    const auto encodedLineCount = static_cast<int>(encodedLines.size()) - (loop ? 1 : 0);

    // The index to reach may be larger than the size, in case a loop is present.
    // In that case, it has to be corrected.
    if (indexToReach >= encodedLineCount) {
        jassert(loop);
        if (!loop) {            // Security, but abnormal case. Only with a loop can an index be too high.
            return 0;
        }

        // Corrects the index.
        indexToReach = ((indexToReach - loopStartIndex) % loopSize) + loopStartIndex;
    }
    jassert(indexToReach < encodedLineCount);

    auto offset = 0;

    // Advances as long as we didn't reach the right encoded line with the right index inside.
    auto index = 0;
    for (const auto& encodedLine : encodedLines) {
        if (indexToReach == index) {
            return offset;
        }
        offset += encodedLine.getSize();

        ++index;
    }

    // We should have found it!
    jassertfalse;
    return offset;
}

std::unique_ptr<RegisterBlock> RegisterBlock::extract(const int firstLineIndex, const int lastLineIndex) const
{
    jassert(lastLineIndex >= firstLineIndex);
    jassert((firstLineIndex >= 0) && (firstLineIndex < getSize()));
    jassert((lastLineIndex >= 0) && (lastLineIndex < getSize()));

    auto output = std::make_unique<RegisterBlock>();

    for (auto index = firstLineIndex; index <= lastLineIndex; ++index) {
        output->addChannelRegisters(channelRegistersList.at(static_cast<size_t>(index)));
    }

    return output;
}

}   // namespace arkostracker

