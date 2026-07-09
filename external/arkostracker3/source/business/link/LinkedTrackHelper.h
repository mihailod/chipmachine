#pragma once

#include <juce_core/juce_core.h>

#include <utility>

#include "../../song/Song.h"

namespace arkostracker
{

class SongController;

/** Helps find the links Tracks. */
class LinkedTrackHelper
{
public:
    class SearchResult
    {
    public:
        SearchResult(juce::String pTrackName, const std::vector<TrackLocation>& pLocations)
            : trackName(std::move(pTrackName)),
              locations(pLocations)
        {
        }

        const juce::String& getTrackName() const
        {
            return trackName;
        }

        const std::vector<TrackLocation>& getLocations() const
        {
            return locations;
        }

        bool operator==(const SearchResult& rhs) const
        {
            return trackName == rhs.trackName &&
                locations == rhs.locations;
        }

        bool operator!=(const SearchResult& rhs) const
        {
            return !(*this == rhs);
        }

    private:
        juce::String trackName;
        std::vector<TrackLocation> locations;
    };

    /** Prevents the instantiation. */
    LinkedTrackHelper() = delete;

    /**
     * @return where are all the named Tracks are and their name. The source Track is requested in order to filter it from
     * the result. We don't want to link a Track to itself just because it has a name!
     * The source Track must be unlinked. If not, asserts, and returns an empty result.
     * @param song the Song.
     * @param subsongId the ID of the Subsong to browse.
     * @param positionIndex the position index of the Track.
     * @param channelIndex the channel index of the Track.
     */
    static std::vector<SearchResult> findNamedTracks(const Song& song, const Id& subsongId, int positionIndex, int channelIndex) noexcept;

    /**
     * @return where are all the named Tracks for the given Track. It should be linked, else nothing is returned (asserts).
     * It is possible that the item has one item but no locations, in case the pattern is not in use anymore.
     * The given Track is excluded from the result.
     * The Linked To Track is also returned, as it is "used", somehow.
     * @param song the Song.
     * @param subsongId the ID of the Subsong to browse.
     * @param positionIndex the position index of the Track.
     * @param channelIndex the channel index of the Track.
     */
    static std::vector<SearchResult> findLinkedTracks(const Song& song, const Id& subsongId, int positionIndex, int channelIndex) noexcept;

    /**
     * @return where are all the named Tracks that are linked to the given Track. The result may be empty.
     * @param song the Song.
     * @param subsongId the ID of the Subsong to browse.
     * @param positionIndex the position index of the Track.
     * @param channelIndex the channel index of the Track.
     */
    static std::vector<SearchResult> findLinkedToTracks(const Song& song, const Id& subsongId, int positionIndex, int channelIndex) noexcept;
};

}   // namespace arkostracker
