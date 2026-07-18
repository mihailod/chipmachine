#include "AkyPatterns.h"

#include "../../../utils/PsgValues.h"
#include "AkyConstants.h"
#include "RegisterBlockPool.h"

namespace arkostracker 
{

AkyPatterns::AkyPatterns(const int pPsgCount) noexcept :
    psgCount(pPsgCount),
    patterns(),
    rawTracks(),
    sortedTracks(),
    akyUsedTracks()
{
}

void AkyPatterns::addPattern(std::unique_ptr<AkyPattern> pattern) noexcept
{
    patterns.push_back(std::move(pattern));
}

void AkyPatterns::optimizePatterns() noexcept
{
    rawTracks.clear();
    sortedTracks.clear();

    // Extracts the Tracks.
    auto rawIndex = 0;
    for (const auto& pattern : patterns) {
        // Creates the raw Tracks, and store the reference in the list to be sorted.
        for (auto trackIndex = 0, trackCount = pattern->getTrackCount(); trackIndex < trackCount; ++trackIndex) {
            auto track(std::make_unique<ChannelDataWithRawOrder>(rawIndex++, pattern->getTrack(trackIndex)));
            sortedTracks.push_back(track.get());
            rawTracks.push_back(std::move(track));
        }
    }

    // Sorts the tracks from shortest to longest.
    std::sort(sortedTracks.begin(), sortedTracks.end(),
        [](const ChannelDataWithRawOrder* left, const ChannelDataWithRawOrder* right) -> bool
    {
        const auto leftSize = left->getDataSize();
        const auto rightSize = right->getDataSize();
        if (leftSize != rightSize) {
            return leftSize < rightSize;
        }
        // To get a more stable sort on Mac, like Linux. We also save a few bytes (though I don't understand why).
        return left->getRawIndex() < right->getRawIndex();
    }
    );

    // Browses the tracks, excluding the "referred to" ones (because they are referred to, so must not be modified), searching for the ones that can be
    // dismissed because equal or part of others.
    for (auto iterator = sortedTracks.begin(); iterator != sortedTracks.end(); ++iterator) {
        auto* track = *iterator;
        // Don't do anything if referred to!
        if (track->isReferringTo()) {
            continue;
        }

        // Browses the next (largest to smallest) tracks to see if the current one fits. We exclude the current track and the previous ones!
        // Note than no "rbegin" inner iterator is used, else the equality operator with the outer iterator wouldn't work.
        for (auto innerIterator = sortedTracks.end() - 1; innerIterator != iterator; --innerIterator) {
            auto* otherTrack = *innerIterator;
            if (isTrackPartOf(*track, *otherTrack)) {
                // We can "ditch" the outer track, because it is part of the inner track.
                track->setSubstitutableBy(otherTrack->getRawIndex());
                otherTrack->setReferringTo();
                // Stops for this outer track.
                break;
            }
        }
    }
}

bool AkyPatterns::isTrackPartOf(const ChannelDataWithRawOrder& firstTrack, const ChannelDataWithRawOrder& secondTrack) noexcept
{
    // According to our algorithm, the first track should be equal in size or smaller.
    const auto firstTrackSize = firstTrack.getDataSize();
    const auto secondTrackSize = secondTrack.getDataSize();

    jassert(firstTrackSize <= secondTrackSize);
    if (firstTrackSize > secondTrackSize) {
        return false;
    }

    for (auto index = 0; index < firstTrackSize; ++index) {
        // Does the line match?
        if (!firstTrack.doesLineDataMatch(index, secondTrack)) {
            // No. Stops directly.
            return false;
        }
    }

    return true;
}

void AkyPatterns::buildAkyTracks() noexcept
{
    // Clears the list of Tracks.
    akyUsedTracks.clear();
    RegisterBlock::resetIdCounter();                // Important to debug, to have always the same result.

    // Only encodes the tracks that not substitutable.
    for (const auto* rawTrack : sortedTracks) {
        const auto rawIndex = rawTrack->getRawIndex();

        if (rawTrack->isSubstitutable()) {
            continue;
        }

        auto akyTrack = std::make_unique<AkyTrack>(rawIndex);

        // Builds the RegisterBlocks.
        auto registerBlock(std::make_shared<RegisterBlock>());
        const auto size = rawTrack->getDataSize();
        for (auto index = 0; index < size; ++index) {
            const auto& channelPlayerResult = rawTrack->getData(index);
            const auto newNote = channelPlayerResult.isNewNotePlayed();
            const auto newEffect = channelPlayerResult.isEffectDeclared();

            if (newNote || newEffect) {
                // Is there a RegisterBlock already started? If yes, stores it.
                if (registerBlock->getSize() > 0) {
                    akyTrack->addRegisterBlock(registerBlock);
                    registerBlock = std::make_shared<RegisterBlock>();
                }
            }

            registerBlock->addChannelRegisters(channelPlayerResult.getChannelOutputRegisters());

            // Has the maximum duration been reached? If yes, creates a new RegisterBlock.
            if (registerBlock->getSize() >= AkyConstants::registerBlockMaximumSize) {
                akyTrack->addRegisterBlock(registerBlock);
                registerBlock = std::make_shared<RegisterBlock>();
            }
        }

        // At the last iteration, stores the possible remaining RegisterBlock.
        if (registerBlock->getSize() > 0) {
            akyTrack->addRegisterBlock(registerBlock);
        }

        // Stores the Track.
        akyUsedTracks.push_back(std::move(akyTrack));
    }
}

void AkyPatterns::findAndSplitBlocksWithRepetitions() noexcept
{
    for (const auto& akyTrack : akyUsedTracks) {
        akyTrack->findAndSplitBlocksWithRepetitions();
    }
}

void AkyPatterns::addBlocksToPool(RegisterBlockPool& registerBlockPool) noexcept
{
    for (const auto& akyTrack : akyUsedTracks) {
        akyTrack->fillPoolWithBlocks(registerBlockPool);
    }

    // Finally, sorts the Blocks of the Pool.
    registerBlockPool.sortRegisterBlocks();
}

int AkyPatterns::getPatternCount() const noexcept
{
    return static_cast<int>(patterns.size());
}

int AkyPatterns::getPatternDuration(const int position) const noexcept
{
    jassert(position < static_cast<int>(patterns.size()));
    return patterns.at(static_cast<size_t>(position))->getTrackLength();
}

int AkyPatterns::getTrackIndex(const int position, const int channelIndex) const noexcept
{
    jassert(position < static_cast<int>(patterns.size()));
    jassert(channelIndex < patterns.at(static_cast<size_t>(position))->getTrackCount());

    // Each track of each position is unique, so it's easy to know the raw index.
    const auto rawIndex = position * PsgValues::channelCountPerPsg * psgCount + channelIndex;

    // Gets the raw track related to this raw index, and gets either its own raw index, or the index of another track is can be substituted by.
    const auto& channelDataWithRawOrder = *rawTracks.at(static_cast<size_t>(rawIndex));
    return channelDataWithRawOrder.getIndexToUse();
}

int AkyPatterns::getTrackToEncodeCount() const noexcept
{
    return static_cast<int>(akyUsedTracks.size());
}

const AkyTrack& AkyPatterns::getEncodedTrack(const int index) const noexcept
{
    jassert(index < static_cast<int>(akyUsedTracks.size()));
    return *akyUsedTracks.at(static_cast<size_t>(index));
}

}   // namespace arkostracker

