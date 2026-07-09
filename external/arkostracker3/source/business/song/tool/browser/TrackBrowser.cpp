#include "TrackBrowser.h"

#include "../../../../song/Song.h"

namespace arkostracker
{

std::vector<int> TrackBrowser::getAllUsedTrackIndexes(const Song& song, const Id& subsongId, const bool stopAtLoopEnd) noexcept
{
    auto lastPositionIndex = 0;
    song.performOnConstSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
        lastPositionIndex = stopAtLoopEnd ? subsong.getEndPosition() : subsong.getLength() - 1;
    });

    // Cumulates all the Track indexes.
    std::unordered_set<int> trackIndexesSet;    // To quickly know if unique or not.
    std::vector<int> trackIndexes;
    for (auto positionIndex = 0; positionIndex <= lastPositionIndex; ++positionIndex) {
        song.performOnConstSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
            const auto& pattern = subsong.getPatternRef(positionIndex);
            const auto patternTrackIndexes = pattern.getCurrentTrackIndexes();
            for (const auto trackIndex: patternTrackIndexes) {
                if (trackIndexesSet.find(trackIndex) == trackIndexesSet.cend()) {
                    // New Track.
                    trackIndexesSet.insert(trackIndex);
                    trackIndexes.push_back(trackIndex);
                }
            }
        });
    }

    return trackIndexes;
}

std::set<int> TrackBrowser::getAllNamedTrackIndexes(const Song& song, const Id& subsongId) noexcept
{
    // Browses all the Tracks.
    std::set<int> trackIndexes;
    song.performOnConstSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
        const auto trackCount = subsong.getTrackCount();
        for (auto trackIndex = 0; trackIndex < trackCount; ++trackIndex) {
            const auto& track = subsong.getTrackRefFromIndex(trackIndex);
            if (track.isNamed()) {
                trackIndexes.insert(trackIndex);
            }
        }
    });

    return trackIndexes;
}

std::vector<TrackLocation> TrackBrowser::findTrackUsage(const Song& song, const Id& subsongId, const int trackIndex, const LinkType linkType) noexcept
{
    std::vector<TrackLocation> locations;

    song.performOnConstSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
        locations = subsong.findTrackUse(trackIndex, linkType);
    });

    return locations;
}

}   // namespace arkostracker
