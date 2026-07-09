#include "SongStripper.h"

#include "../../../../song/Song.h"
#include "../../../../utils/MapUtil.h"

namespace arkostracker 
{

std::unique_ptr<Song> SongStripper::split(const Song& originalSong, const SubsongsAndPsgs& subsongsAndPsgsToKeep) noexcept
{
    // First, make a copy of the original Song.
    auto song = std::make_unique<Song>(originalSong);

    // Browses the new Subsongs. If they should not be present, remove them.
    for (const auto& subsongId: song->getSubsongIds()) {
        if (subsongsAndPsgsToKeep.isSubsongPresent(subsongId)) {
            // The Subsong must be kept. Must some PSGs be removed?
            managePsg(*song, subsongId, subsongsAndPsgsToKeep);
        } else {
            song->deleteSubsong(subsongId);
        }
    }

    jassert(song->getSubsongCount() > 0);       // There MUST always be at least one Subsong! This is going to crash...

    return song;
}

void SongStripper::managePsg(Song& song, const Id& subsongId, const SubsongsAndPsgs& subsongsAndPsgsToKeep) noexcept
{
    const auto psgIndexesToKeep = subsongsAndPsgsToKeep.getPsgIndexes(subsongId);
    const auto channelIndexesToKeep = subsongsAndPsgsToKeep.getChannelIndexes(subsongId);
    jassert(!psgIndexesToKeep.empty());

    // Note: the SetSubsongPsgs cannot be used, because the behavior here is slightly different (we can remove any PSG).

    song.performOnSubsong(subsongId, [&](Subsong& subsong) {
        // First, removes the PSGs. They are deleted in reverse!
        auto psgs = subsong.getPsgs();
        for (auto psgIndex = static_cast<int>(psgs.size()) - 1; psgIndex >= 0; --psgIndex) {
            if (psgIndexesToKeep.find(psgIndex) == psgIndexesToKeep.cend()) {
                psgs.erase(psgs.begin() + psgIndex);
            }
        }
        jassert(!psgs.empty());         // There must be a PSG left! This is going to crash...
        subsong.setPsgs(psgs);

        // Then, changes the Positions.
        const auto positionCount = subsong.getLength();
        for (auto positionIndex = 0; positionIndex < positionCount; ++positionIndex) {
            // Removes the transpositions that are from a PSG that doesn't exist anymore, but also shifts the next ones. Simpler to recreate the map.
            const auto& originalPosition = subsong.getPositionRef(positionIndex);
            const auto& originalChannelToTranspositionUnordered = originalPosition.getChannelToTransposition();
            // IMPORTANT! the original map must be ordered! Else the algorithm below won't work.
            std::map originalChannelToTransposition(originalChannelToTranspositionUnordered.cbegin(), originalChannelToTranspositionUnordered.cend());

            std::unordered_map<int, int> channelToTransposition;
            auto channelIndex = 0;
            for (const auto&[readChannelIndex, transposition] : originalChannelToTransposition) {
                if (channelIndexesToKeep.find(readChannelIndex) != channelIndexesToKeep.cend()) {
                    // The channel must be kept. However, use the increasing channel index! This will take care of the holes of deletion in the source map.
                    channelToTransposition.insert(std::make_pair(channelIndex, transposition));
                    ++channelIndex;
                }
            }

            Position position(originalPosition.getPatternIndex(), originalPosition.getHeight(), originalPosition.getMarkerName(), originalPosition.getMarkerColor(),
                    channelToTransposition);
            subsong.setPosition(positionIndex, position);
        }

        // Then, changes the Patterns. Removes the channels that must be.
        const auto patternCount = subsong.getPatternCount();
        for (auto patternIndex = 0; patternIndex < patternCount; ++patternIndex) {
            // Creates a new pattern from the original one.
            const auto& originalPattern = subsong.getPatternFromIndex(patternIndex);
            const auto channelCount = originalPattern.getChannelCount();

            Pattern newPattern(originalPattern.getArgbColor());

            // Keeps only the right channels.
            std::vector<Pattern::TrackIndexAndLinkedTrackIndex> trackIndexes;
            for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
                if (channelIndexesToKeep.find(channelIndex) != channelIndexesToKeep.cend()) {
                    trackIndexes.push_back(originalPattern.getTrackIndexAndLinkedTrackIndex(channelIndex));
                }
            }

            Pattern pattern(trackIndexes, originalPattern.getSpecialTrackIndexAndLinkedTrackIndex(true),
                    originalPattern.getSpecialTrackIndexAndLinkedTrackIndex(false), originalPattern.getArgbColor());
            subsong.setPattern(patternIndex, pattern);
        }
    });
}

}   // namespace arkostracker
