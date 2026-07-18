#include "SpecialTrackBrowser.h"

#include "../../../../song/Song.h"
#include "../../../../utils/SetUtil.h"

namespace arkostracker
{

std::vector<int> SpecialTrackBrowser::getAllUsedSpecialTrackIndexes(const Song& song, const Id& subsongId, const bool isSpeedTrack, const bool stopAtLoopEnd) noexcept
{
    auto lastPositionIndex = 0;
    song.performOnConstSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
        lastPositionIndex = stopAtLoopEnd ? subsong.getEndPosition() : subsong.getLength() - 1;
    });

    // Cumulates all the Special Track indexes.
    std::set<int> specialTrackIndexesSet;
    for (auto positionIndex = 0; positionIndex <= lastPositionIndex; ++positionIndex) {
        song.performOnConstSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
            const auto& pattern = subsong.getPatternRef(positionIndex);
            const auto specialTrackIndex = pattern.getCurrentSpecialTrackIndex(isSpeedTrack);
            specialTrackIndexesSet.insert(specialTrackIndex);
        });
    }

    return SetUtil::setToVector(specialTrackIndexesSet);
}

std::set<int> SpecialTrackBrowser::getAllNamedSpecialTrackIndexes(const Song& song, const Id& subsongId, const bool isSpeedTrack) noexcept
{
    // Browses all the Special Tracks.
    std::set<int> specialTrackIndexes;
    song.performOnConstSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
        const auto specialTrackCount = subsong.getSpecialTrackCount(isSpeedTrack);
        for (auto specialTrackIndex = 0; specialTrackIndex < specialTrackCount; ++specialTrackIndex) {
            const auto& specialTrack = subsong.getSpecialTrackRefFromIndex(specialTrackIndex, isSpeedTrack);
            if (specialTrack.isNamed()) {
                specialTrackIndexes.insert(specialTrackIndex);
            }
        }
    });

    return specialTrackIndexes;
}

std::vector<SpecialTrackLocation> SpecialTrackBrowser::findSpecialTrackUsage(const Song& song, const Id& subsongId, const bool isSpeedTrack, const int specialTrackIndex,
    const LinkType linkType) noexcept
{
    std::vector<SpecialTrackLocation> locations;

    song.performOnConstSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
        locations = subsong.findSpecialTrackUse(isSpeedTrack, specialTrackIndex, linkType);
    });

    return locations;
}

}   // namespace arkostracker
