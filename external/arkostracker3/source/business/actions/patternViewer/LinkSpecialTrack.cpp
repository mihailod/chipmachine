#include "LinkSpecialTrack.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/LinkerObserver.h"

namespace arkostracker
{

LinkSpecialTrack::LinkSpecialTrack(SongController& pSongController, Id pSubsongId, bool pIsSpeedTrack, const int pSourcePositionIndex, const int pTargetPositionIndex) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        isSpeedTrack(pIsSpeedTrack),
        sourcePositionIndex(pSourcePositionIndex),
        targetPositionIndex(pTargetPositionIndex),
        originalPattern(),
        sourcePatternIndex()
{
}

bool LinkSpecialTrack::perform()
{
    songController.performOnSubsong(subsongId, [&] (Subsong& subsong) {
        // Finds the target Special Track.
        const auto targetSpecialTrackIndex = subsong.getPatternRef(targetPositionIndex).getCurrentSpecialTrackIndex(isSpeedTrack);

        // Finds the source pattern index.
        sourcePatternIndex = subsong.getPositionRef(sourcePositionIndex).getPatternIndex();

        // Gets a copy of the original Pattern. Stores it for the undo.
        originalPattern = subsong.getPatternRef(sourcePositionIndex);
        auto newPattern = originalPattern;

        const auto& originalSpeedTrackIndexes = newPattern.getSpecialTrackIndexAndLinkedTrackIndex(isSpeedTrack);

        const auto newTrackIndexes = Pattern::TrackIndexAndLinkedTrackIndex(
            originalSpeedTrackIndexes.getUnlinkedTrackIndex(),       // Only the linked Special Track changes.
            targetSpecialTrackIndex
        );
        newPattern.setSpecialTrackIndex(isSpeedTrack, newTrackIndexes);

        // Applies it as a link.
        subsong.setPattern(sourcePatternIndex, newPattern);
    });

    notifyListeners();

    return true;
}

bool LinkSpecialTrack::undo()
{
    songController.performOnSubsong(subsongId, [&] (Subsong& subsong) {
        subsong.setPattern(sourcePatternIndex, originalPattern);
    });

    notifyListeners();

    return true;
}

void LinkSpecialTrack::notifyListeners() noexcept
{
    songController.getLinkerObservers().applyOnObservers([&] (LinkerObserver* observer) noexcept {
        observer->onLinkerPositionChanged(subsongId, sourcePositionIndex, LinkerObserver::What::link);
    });
}

}   // namespace arkostracker
