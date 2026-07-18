#pragma once

#include <vector>

#include "../../ui/lookAndFeel/LookAndFeelConstants.h"
#include "../../utils/OptionalValue.h"
#include "LinkType.h"

namespace arkostracker 
{

/**
 * A Pattern. Contains the track indexes.
 * Pattern has no height. Their height is possessed by the Position! Thus, the same Pattern can have different size as they are being reused.
 */
class Pattern
{
public:
    /** Represents a track ID, superseded by the Linked trackID if present. */
    class TrackIndexAndLinkedTrackIndex
    {
    public:
        /**
         * Constructor.
         * @param pTrackIndex the track index.
         * @param pLinkedTrackIndex the possible linked track index.
         */
        explicit TrackIndexAndLinkedTrackIndex(int pTrackIndex, OptionalInt pLinkedTrackIndex = {}) noexcept;

        /**
         * Convenient builder for a list of indexes with only track indexes, no linked tracks.
         * @param trackIndexes the track indexes.
         */
        static std::vector<TrackIndexAndLinkedTrackIndex> buildFromTrackIds(const std::vector<int>& trackIndexes) noexcept;

        /** @return the normal track index, or linked one if present. */
        int getUsedTrackIndex() const noexcept;
        /** @return the unlinked track. */
        int getUnlinkedTrackIndex() const;
        /** @return the linked track index, or absent if not present. */
        const OptionalInt& getLinkedTrackIndex() const;

        /** @return true if both values are >=0 (or linked track is absent). */
        bool isValid() const noexcept;
        /** @return true if there is a linked track. */
        bool isLinked() const noexcept;

        bool operator==(const TrackIndexAndLinkedTrackIndex& rhs) const noexcept;
        bool operator!=(const TrackIndexAndLinkedTrackIndex& rhs) const noexcept;

        /** Used for "find" in maps. */
        bool operator<(const TrackIndexAndLinkedTrackIndex& rhs) const noexcept;

    private:
        int trackIndex;
        OptionalInt linkedTrackIndex;       // Absent if not linked.
    };
    /**
     * Constructor when there are NO linked tracks. This can be used by non-native importers.
     * @param trackIndexes the track indexes for each channel. It must be in accordance with how many PSG there are.
     * @param speedTrackIndex the index of the speed track.
     * @param eventTrackIndex the index of the event track.
     * @param color the color of the Pattern.
     */
    Pattern(const std::vector<int>& trackIndexes, int speedTrackIndex, int eventTrackIndex, juce::uint32 color = LookAndFeelConstants::defaultPatternColor) noexcept;

    /**
     * Constructor.
     * @param trackIdAndLinkedTrackId the track indexes (linked or not) for each channel. It must be in accordance with how many PSG there are.
     * @param speedTrackLinkOrIndex the index of the speed track.
     * @param eventsTrackLinkOrIndex the index of the event track.
     * @param color the color of the Pattern.
     */
    Pattern(std::vector<TrackIndexAndLinkedTrackIndex> trackIdAndLinkedTrackId, TrackIndexAndLinkedTrackIndex speedTrackLinkOrIndex, TrackIndexAndLinkedTrackIndex eventsTrackLinkOrIndex,
            juce::uint32 color = LookAndFeelConstants::defaultPatternColor) noexcept;

    /** Default constructor, only useful for maps. */
    Pattern() noexcept;

    /**
     * Constructor, useful to build track by track. Don't forget to also set the special tracks!
     * @param newColor the new color.
     */
    explicit Pattern(juce::uint32 newColor) noexcept;

    /**
     * @return a copy of the instance, with a new color.
     * @param newColor the new color.
     */
    Pattern withColor(juce::uint32 newColor) const noexcept;

    /**
     * @return a copy of the instance, with a new color.
     * @param newSpeedTrackIndex the new speed track index.
     */
    Pattern withSpeedTrackIndex(int newSpeedTrackIndex) const noexcept;     // FIXME Link or unlink??

    /** @return the color of the Pattern. */
    juce::uint32 getArgbColor() const noexcept;

    /**
     * @return the track index related to the given channel index. It may be the linked track if present, or the normal one.
     * @param channelIndex the channel index. 0-2 for PSG1, 3-5 for PSG2, etc. Must be valid.
     */
    int getCurrentTrackIndex(int channelIndex) const noexcept;

    /** @return the track indexes, linked or not. */
    std::vector<int> getCurrentTrackIndexes() const noexcept;

    /**
     * @return the track id and linked track id (which may not be present) for the given channel index.
     * @param channelIndex the channel index. 0-2 for PSG1, 3-5 for PSG2, etc. Must be valid.
     */
    const TrackIndexAndLinkedTrackIndex& getTrackIndexAndLinkedTrackIndex(int channelIndex) const noexcept;

    /** @return the track ids and linked track ids (which may not be present). */
    const std::vector<TrackIndexAndLinkedTrackIndex>& getTrackIndexesAndLinkedTrackIndexes() const noexcept;

    /**
     * Adds track ids. This should only be called when using the empty Pattern constructor.
     * @param trackIndex the track index.
     * @param linkedTrackIndex the linked track index, if it is present.
     */
    void addTrackIndex(int trackIndex, OptionalInt linkedTrackIndex) noexcept;

    /**
     * Adds track ids. This should only be called when using the empty Pattern constructor.
     * @param trackIndexAndLinkedTrackIndex the indexes, unlinked and possible linked.
     */
    void addTrackIndex(const TrackIndexAndLinkedTrackIndex& trackIndexAndLinkedTrackIndex) noexcept;

    /**
     * Sets track ids.
     * @param channelIndex the channel index. It must already exist.
     * @param trackIndexAndLinkedTrackIndex the indexes, unlinked and possible linked.
     */
    void setTrackIndex(int channelIndex, const TrackIndexAndLinkedTrackIndex& trackIndexAndLinkedTrackIndex) noexcept;

    /**
     * Sets the track ids for speed or event.
     * @param speedTrack true if speed track, false if event track.
     * @param trackIndex the track index.
     * @param linkedTrackIndex the linked track index, if it is present.
     */
    void setSpecialTrackIndex(bool speedTrack, int trackIndex, OptionalInt linkedTrackIndex) noexcept;

    /**
     * Sets the track ids for speed or event.
     * @param speedTrack true if speed track, false if event track.
     * @param trackIndexAndLinkedTrackIndex the unlink/link track indexes.
     */
    void setSpecialTrackIndex(bool speedTrack, const TrackIndexAndLinkedTrackIndex& trackIndexAndLinkedTrackIndex) noexcept;

    /**
     * @return the index of the Special Track, which may be the linked one or not.
     * @param speedTrack true if speed track, false if event track.
     */
    int getCurrentSpecialTrackIndex(bool speedTrack) const noexcept;

    /**
     * @return the index of the Special Track.
     * @param speedTrack true if speed track, false if event track.
     */
    TrackIndexAndLinkedTrackIndex getSpecialTrackIndexAndLinkedTrackIndex(bool speedTrack) const noexcept;

    /** @return how many channels there are in this Pattern. */
    int getChannelCount() const noexcept;

    /**
     * @return the channel indexes where the given Track index is used, with the given link type.
     * @param trackIndexToFind the track index.
     * @param linkType the link type to look for.
     */
    std::vector<int> getWhereTrackUsed(int trackIndexToFind, LinkType linkType = LinkType::unlinkedIfLinkedNotPresent) const noexcept;

    /**
     * @return true if the given Special Track index is present, with the given link type.
     * @param speedTrack true if speed track, false if event track.
     * @param specialTrackIndexToFind the special track index to look for.
     * @param linkType the link type to look for.
     */
    bool isSpecialTrackUsed(bool speedTrack, int specialTrackIndexToFind, LinkType linkType = LinkType::unlinkedIfLinkedNotPresent) const noexcept;

    bool operator==(const Pattern& rhs) const noexcept;
    bool operator!=(const Pattern& rhs) const noexcept;

    /** Used for "find" in maps. */
    bool operator<(const Pattern& rhs) const noexcept;

    /** @return true if the current Pattern is equal to the other, not taking the color in account. */
    bool areEqualMusically(const Pattern& other) const noexcept;

private:
    juce::uint32 color;                     // The color of the Pattern.

    std::vector<TrackIndexAndLinkedTrackIndex> trackLinksOrIndexes;         // The track indexes for each channel. It must be in accordance with how many PSG there are.
    TrackIndexAndLinkedTrackIndex speedTrackLinkOrIndex;                    // The index of the speed track.
    TrackIndexAndLinkedTrackIndex eventTrackLinkOrIndex;                    // the index of the event track.
};

}   // namespace arkostracker
