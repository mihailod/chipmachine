#include "UnlinkTrack.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/LinkerObserver.h"

namespace arkostracker
{

UnlinkTrack::UnlinkTrack(SongController& pSongController, Id pSubsongId, const int pPositionIndex, const int pChannelIndex) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        positionIndex(pPositionIndex),
        channelIndex(pChannelIndex),
        originalPattern(),
        sourcePatternIndex()
{
}

bool UnlinkTrack::perform()
{
    songController.performOnSubsong(subsongId, [&] (Subsong& subsong) {
        // Finds the Pattern index.
        sourcePatternIndex = subsong.getPositionRef(positionIndex).getPatternIndex();

        // Gets a copy of the original Pattern. Stores it for the undo.
        originalPattern = subsong.getPatternRef(positionIndex);
        auto newPattern = originalPattern;

        const auto& originalTrackIndexes = newPattern.getTrackIndexAndLinkedTrackIndex(channelIndex);

        const auto newTrackIndexes = Pattern::TrackIndexAndLinkedTrackIndex(
            originalTrackIndexes.getUnlinkedTrackIndex(),
            { }       // Only the linked Track changes. Now unlinked.
        );
        newPattern.setTrackIndex(channelIndex, newTrackIndexes);

        // Applies it as a link.
        subsong.setPattern(sourcePatternIndex, newPattern);
    });

    notifyListeners();

    return true;
}

bool UnlinkTrack::undo()
{
    songController.performOnSubsong(subsongId, [&] (Subsong& subsong) {
        subsong.setPattern(sourcePatternIndex, originalPattern);
    });

    notifyListeners();

    return true;
}

void UnlinkTrack::notifyListeners() noexcept
{
    songController.getLinkerObservers().applyOnObservers([&] (LinkerObserver* observer) noexcept {
        observer->onLinkerPositionChanged(subsongId, positionIndex, LinkerObserver::What::link);
    });
}

}   // namespace arkostracker
