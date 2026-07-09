#include "LinkedTrackHelper.h"

#include "../song/tool/browser/TrackBrowser.h"

namespace arkostracker
{

std::vector<LinkedTrackHelper::SearchResult> LinkedTrackHelper::findNamedTracks(const Song& song, const Id& subsongId, const int positionIndex, const int channelIndex) noexcept
{
    std::vector<SearchResult> results;

    // Gets the source pattern to know the current Track index.
    auto sourceTrackIndex = 0;
    song.performOnConstSubsong(subsongId, [&] (const Subsong& subsong) {
        const auto& pattern = subsong.getPatternRef(positionIndex);
        const auto trackIndexes = pattern.getTrackIndexAndLinkedTrackIndex(channelIndex);
        if (trackIndexes.isLinked()) {
            // Link! This method shouldn't have been called.
            jassertfalse;
            return;
        }
        sourceTrackIndex = trackIndexes.getUnlinkedTrackIndex();
    });

    const auto trackIndexes = TrackBrowser::getAllNamedTrackIndexes(song, subsongId);

    // How many times used, and where?
    for (const auto trackIndex : trackIndexes) {
        // Dismisses the source Track.
        if (sourceTrackIndex == trackIndex) {
            continue;
        }

        const auto locations = TrackBrowser::findTrackUsage(song, subsongId, trackIndex);
        if (!locations.empty()) {
            juce::String trackName;
            song.performOnConstSubsong(subsongId, [&] (const Subsong& subsong) {
                const auto& track = subsong.getTrackRefFromIndex(trackIndex);
                trackName = track.getName();
                jassert(trackName.trim().isNotEmpty());     // This was tested before!
            });

            results.emplace_back(trackName, locations);
        }
    }

    return results;
}

std::vector<LinkedTrackHelper::SearchResult> LinkedTrackHelper::findLinkedTracks(const Song& song, const Id& subsongId, const int positionIndex, const int channelIndex) noexcept
{
    std::vector<SearchResult> results;

    song.performOnConstSubsong(subsongId, [&] (const Subsong& subsong) {
        // Gets the source pattern to know the linked Track.
        const auto& pattern = subsong.getPatternRef(positionIndex);
        const auto trackIndexes = pattern.getTrackIndexAndLinkedTrackIndex(channelIndex);
        if (!trackIndexes.isLinked()) {
            // No link! This method shouldn't have been called.
            jassertfalse;
            return;
        }
        const auto trackIndex = trackIndexes.getLinkedTrackIndex().getValue();
        const auto trackName = subsong.getTrackRefFromIndex(trackIndex).getName();

        // Where was it used?
        const auto originalTrackLocations = subsong.findTrackUse(trackIndex, LinkType::unlinkedIfLinkedNotPresent);
        // Filters our current location.
        std::vector<TrackLocation> trackLocations;
        std::copy_if(originalTrackLocations.cbegin(), originalTrackLocations.cend(), std::back_inserter(trackLocations), [&](const TrackLocation& item) {
            return (item.getPositionIndex() != positionIndex) || (item.getChannelIndex() != channelIndex);
        });
        results.emplace_back(trackName, trackLocations);
    });

    return results;
}

std::vector<LinkedTrackHelper::SearchResult> LinkedTrackHelper::findLinkedToTracks(const Song& song, const Id& subsongId, const int positionIndex, const int channelIndex) noexcept
{
    std::vector<SearchResult> results;

    song.performOnConstSubsong(subsongId, [&] (const Subsong& subsong) {
        // Gets the source pattern to know the linked Track.
        const auto& pattern = subsong.getPatternRef(positionIndex);
        const auto trackIndexes = pattern.getTrackIndexAndLinkedTrackIndex(channelIndex);
        if (trackIndexes.isLinked()) {
            // Link! This method shouldn't have been called.
            jassertfalse;
            return;
        }
        const auto trackIndex = trackIndexes.getUnlinkedTrackIndex();
        const auto trackName = subsong.getTrackRefFromIndex(trackIndex).getName();

        // Where was it used? Stores it only if at least one reference is found.
        const auto trackLocations = subsong.findTrackUse(trackIndex, LinkType::linkedOnly);
        if (!trackLocations.empty()) {
            results.emplace_back(trackName, trackLocations);
        }
    });

    return results;
}

}   // namespace arkostracker