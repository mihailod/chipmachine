#pragma once

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "AkyPattern.h"
#include "AkyTrack.h"

namespace arkostracker 
{

class ChannelPlayerResults;
class RegisterBlockPool;

/**
    Hold the AKY Patterns (raw registers with metadata).
*/
class AkyPatterns
{
public:
    /** Constructor. */
    explicit AkyPatterns(int psgCount) noexcept;

    /** Adds a Pattern. */
    void addPattern(std::unique_ptr<AkyPattern> pattern) noexcept;

    /** Reorders the tracks so that common tracks are put together. This is a necessary step once all the Patterns are added. */
    void optimizePatterns() noexcept;

    /** Once, the Patterns are optimized and we know which tracks to encode, the AkyTracks can be built. */
    void buildAkyTracks() noexcept;

    /**
        Some Tracks may contain Blocks that contain repetitions: this takes room and can not be optimized by a loop.
        But they can be split, to allow the loop to do a better job later.
    */
    void findAndSplitBlocksWithRepetitions() noexcept;

    /** Adds the Blocks from the AkyTracks to the RegisterBlockPool. They are copied. */
    void addBlocksToPool(RegisterBlockPool& registerBlockPool) noexcept;

    /** Returns how many Patterns there are in the song. */
    int getPatternCount() const noexcept;

    /**
     * @return the duration, in frames, of a position.
     * @param position the position (>= 0).
     */
    int getPatternDuration(int position) const noexcept;

    /**
     * @return the track index for a position and a channel index. It does not match the Track index from the Song.
     * It can be an index that was not on the original song!
     * @param position the position (>= 0).
     * @param channelIndex the channel index (>= 0, <3).
     */
    int getTrackIndex(int position, int channelIndex) const noexcept;

    /** @return how many tracks have to be encoded. */
    int getTrackToEncodeCount() const noexcept;

    /** @return the encoded track from the given index. It is NOT a raw index. Just a simple index. */
    const AkyTrack& getEncodedTrack(int index) const noexcept;

private:
    class ChannelDataWithRawOrder {
    public:
        /**
          * Constructor.
          * @param pRawIndex the raw index when first inserted.
          * @param pChannelData the data.
          * @param pIsReferringTo true if another track refers to this track (the other track can be dismissed).
          * @param pSubstitutableBy if >=0, the raw index to use instead of this one.
         */
        ChannelDataWithRawOrder(const int pRawIndex, ChannelData& pChannelData, const bool pIsReferringTo = false, const int pSubstitutableBy = -1) :
            rawIndex(pRawIndex),
            channelData(pChannelData),
            referredTo(pIsReferringTo),
            substitutableBy(pSubstitutableBy)
        {
        }
        /** @return the size of the data. */
        int getDataSize() const noexcept
        {
            return channelData.getSize();
        }
        /** @return true if the data is referred to by another data, so it should probably not be tempered with! */
        bool isReferringTo() const noexcept
        {
            return referredTo;
        }
        /** @return the raw index. */
        int getRawIndex() const noexcept
        {
            return rawIndex;
        }
        /** Sets the track as "referred to". */
        void setReferringTo() noexcept
        {
            referredTo = true;
        }
        /** @return true if the track can be substitutable by another track. This means this track can be dismissed. */
        bool isSubstitutable() const noexcept
        {
            return (substitutableBy >= 0);
        }
        /** Sets the track as substitutable by another track (which can be longer), which raw index is given. This means this track can be dismissed. */
        void setSubstitutableBy(const int substitutableByRawIndex) noexcept
        {
            substitutableBy = substitutableByRawIndex;
        }
        /** @return the track index to use. Either this one, or another one if this track can be dismissed. */
        int getIndexToUse() const noexcept
        {
            return isSubstitutable() ? substitutableBy : rawIndex;
        }

        /**
            Indicates whether two data lines matches.
            @param index the index for both the data.
            @param otherChannelData the other channel data.
            @return true if they match.
        */
        bool doesLineDataMatch(const int index, const ChannelDataWithRawOrder& otherChannelData) const noexcept
        {
            jassert(index < channelData.getSize());
            jassert(index < otherChannelData.getDataSize());        // Tolerated, but shouldn't happen from our algorithm.

            if (index >= otherChannelData.getDataSize()) {
                return false;
            }
            return (channelData.getData(index).isPsgDataEqual(otherChannelData.channelData.getData(index)));
        }
        /**
         * @return the data according to the given index.
         * @param index the index. Must be valid.
         */
        const ChannelPlayerResults& getData(const int index) const noexcept
        {
            return channelData.getData(index);
        }
        /** @return true if this track doesn't have to be encoded. */
        bool canBeDismissed() const noexcept
        {
            return (!referredTo && (substitutableBy >= 0));
        }

    private:
        int rawIndex;                           // The raw index when first inserted.
        ChannelData& channelData;               // The data.
        bool referredTo;                        // True if another track refers to this track (the other track can be dismissed).
        int substitutableBy;                    // If >=0, the raw index to use instead of this one.
    };

    /**
     * @return whether the first track is a part of the second. The first track should be already smaller or of the same size.
     * @param firstTrack the first track (the smaller one).
     * @param secondTrack the second track (the largest one).
     */
    static bool isTrackPartOf(const ChannelDataWithRawOrder& firstTrack, const ChannelDataWithRawOrder& secondTrack) noexcept;

    int psgCount;                                                               // How many PSG there are in the Song.

    std::vector<std::unique_ptr<AkyPattern>> patterns;                          // The patterns.

    std::vector<std::unique_ptr<ChannelDataWithRawOrder>> rawTracks;            // The Tracks, in the raw order.
    std::vector<ChannelDataWithRawOrder*> sortedTracks;                         // The sorted Tracks, from smallest to largest.

    // The used AKY tracks to build, in the raw order. Only used Tracks are encoded.
    std::vector<std::unique_ptr<AkyTrack>> akyUsedTracks;
};

}   // namespace arkostracker

