#include "UnlinkSpecialTrack.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/LinkerObserver.h"

namespace arkostracker
{

UnlinkSpecialTrack::UnlinkSpecialTrack(SongController& pSongController, Id pSubsongId, const bool pIsSpeedTrack, const int pPositionIndex) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        isSpeedTrack(pIsSpeedTrack),
        positionIndex(pPositionIndex),
        originalPattern(),
        sourcePatternIndex()
{
}

bool UnlinkSpecialTrack::perform()
{
    songController.performOnSubsong(subsongId, [&] (Subsong& subsong) {
        // Finds the Pattern index.
        sourcePatternIndex = subsong.getPositionRef(positionIndex).getPatternIndex();

        // Gets a copy of the original Pattern. Stores it for the undo.
        originalPattern = subsong.getPatternRef(positionIndex);
        auto newPattern = originalPattern;

        const auto& originalSpecialTrackIndexes = newPattern.getSpecialTrackIndexAndLinkedTrackIndex(isSpeedTrack);

        const auto newSpecialTrackIndexes = Pattern::TrackIndexAndLinkedTrackIndex(
            originalSpecialTrackIndexes.getUnlinkedTrackIndex(),
            { }       // Only the linked Track changes. Now unlinked.
        );
        newPattern.setSpecialTrackIndex(isSpeedTrack, newSpecialTrackIndexes);

        // Applies it as a link.
        subsong.setPattern(sourcePatternIndex, newPattern);
    });

    notifyListeners();

    return true;
}

bool UnlinkSpecialTrack::undo()
{
    songController.performOnSubsong(subsongId, [&] (Subsong& subsong) {
        subsong.setPattern(sourcePatternIndex, originalPattern);
    });

    notifyListeners();

    return true;
}

void UnlinkSpecialTrack::notifyListeners() noexcept
{
    songController.getLinkerObservers().applyOnObservers([&] (LinkerObserver* observer) noexcept {
        observer->onLinkerPositionChanged(subsongId, positionIndex, LinkerObserver::What::link);
    });
}

}   // namespace arkostracker
