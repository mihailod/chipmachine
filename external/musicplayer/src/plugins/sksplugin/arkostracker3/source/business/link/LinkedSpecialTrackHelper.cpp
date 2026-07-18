#include "LinkedSpecialTrackHelper.h"

#include "../song/tool/browser/SpecialTrackBrowser.h"

namespace arkostracker
{

std::vector<LinkedSpecialTrackHelper::SearchResult> LinkedSpecialTrackHelper::findNamedSpecialTracks(const Song& song, const Id& subsongId, const int positionIndex,
    const bool isSpeedTrack) noexcept
{
    std::vector<SearchResult> results;

    // Gets the source pattern to know the current Track index.
    auto sourceTrackIndex = 0;
    song.performOnConstSubsong(subsongId, [&] (const Subsong& subsong) {
        const auto& pattern = subsong.getPatternRef(positionIndex);
        const auto trackIndexes = pattern.getSpecialTrackIndexAndLinkedTrackIndex(isSpeedTrack);
        if (trackIndexes.isLinked()) {
            // Link! This method shouldn't have been called.
            jassertfalse;
            return;
        }
        sourceTrackIndex = trackIndexes.getUnlinkedTrackIndex();
    });

    const auto specialTrackIndexes = SpecialTrackBrowser::getAllNamedSpecialTrackIndexes(song, subsongId, isSpeedTrack);

    // How many times used, and where?
    for (const auto specialTrackIndex : specialTrackIndexes) {
        // Dismisses the source Track.
        if (sourceTrackIndex == specialTrackIndex) {
            continue;
        }

        const auto locations = SpecialTrackBrowser::findSpecialTrackUsage(song, subsongId, isSpeedTrack, specialTrackIndex);
        if (!locations.empty()) {
            juce::String trackName;
            song.performOnConstSubsong(subsongId, [&] (const Subsong& subsong) {
                const auto& specialTrack = subsong.getSpecialTrackRefFromIndex(specialTrackIndex, isSpeedTrack);
                trackName = specialTrack.getName();
                jassert(trackName.trim().isNotEmpty());     // This was tested before!
            });

            results.emplace_back(trackName, locations);
        }
    }

    return results;
}

std::vector<LinkedSpecialTrackHelper::SearchResult> LinkedSpecialTrackHelper::findLinkedSpecialTracks(const Song& song, const Id& subsongId, const int positionIndex,
    const bool isSpeedTrack) noexcept
{
    std::vector<SearchResult> results;

    song.performOnConstSubsong(subsongId, [&] (const Subsong& subsong) {
        // Gets the source pattern to know the linked Track.
        const auto& pattern = subsong.getPatternRef(positionIndex);
        const auto specialTrackIndexes = pattern.getSpecialTrackIndexAndLinkedTrackIndex(isSpeedTrack);
        if (!specialTrackIndexes.isLinked()) {
            // No link! This method shouldn't have been called.
            jassertfalse;
            return;
        }
        const auto specialTrackIndex = specialTrackIndexes.getLinkedTrackIndex().getValue();
        const auto trackName = subsong.getSpecialTrackRefFromIndex(specialTrackIndex, isSpeedTrack).getName();

        // Where was it used?
        const auto originalTrackLocations = subsong.findSpecialTrackUse(isSpeedTrack, specialTrackIndex, LinkType::unlinkedIfLinkedNotPresent);
        // Filters our current location.
        std::vector<SpecialTrackLocation> specialTrackLocations;
        std::copy_if(originalTrackLocations.cbegin(), originalTrackLocations.cend(), std::back_inserter(specialTrackLocations), [&](const SpecialTrackLocation& item) {
            return (item.getPositionIndex() != positionIndex) || (item.isSpeedTrack() != isSpeedTrack);
        });
        results.emplace_back(trackName, specialTrackLocations);
    });

    return results;
}

std::vector<LinkedSpecialTrackHelper::SearchResult> LinkedSpecialTrackHelper::findLinkedToSpecialTracks(const Song& song, const Id& subsongId, const int positionIndex,
    const bool isSpeedTrack) noexcept
{
    std::vector<SearchResult> results;

    song.performOnConstSubsong(subsongId, [&] (const Subsong& subsong) {
        // Gets the source pattern to know the linked Track.
        const auto& pattern = subsong.getPatternRef(positionIndex);
        const auto specialTrackIndexes = pattern.getSpecialTrackIndexAndLinkedTrackIndex(isSpeedTrack);
        if (specialTrackIndexes.isLinked()) {
            // Link! This method shouldn't have been called.
            jassertfalse;
            return;
        }
        const auto trackIndex = specialTrackIndexes.getUnlinkedTrackIndex();
        const auto trackName = subsong.getSpecialTrackRefFromIndex(trackIndex, isSpeedTrack).getName();

        // Where was it used? Stores it only if at least one reference is found.
        const auto specialTrackLocations = subsong.findSpecialTrackUse(isSpeedTrack, trackIndex, LinkType::linkedOnly);
        if (!specialTrackLocations.empty()) {
            results.emplace_back(trackName, specialTrackLocations);
        }
    });

    return results;
}

}   // namespace arkostracker