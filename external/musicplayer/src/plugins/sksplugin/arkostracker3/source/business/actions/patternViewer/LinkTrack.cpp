#include "LinkTrack.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/LinkerObserver.h"

namespace arkostracker
{

LinkTrack::LinkTrack(SongController& pSongController, Id pSubsongId, const int pSourcePositionIndex, const int pSourceChannelIndex,
    const int pTargetPositionIndex, const int pTargetChannelIndex) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        sourcePositionIndex(pSourcePositionIndex),
        sourceChannelIndex(pSourceChannelIndex),
        targetPositionIndex(pTargetPositionIndex),
        targetChannelIndex(pTargetChannelIndex),
        originalPattern(),
        sourcePatternIndex()
{
}

bool LinkTrack::perform()
{
    songController.performOnSubsong(subsongId, [&] (Subsong& subsong) {
        // Finds the target Track.
        const auto targetTrackIndex = subsong.getPatternRef(targetPositionIndex).getCurrentTrackIndex(targetChannelIndex);

        // Finds the source pattern index.
        sourcePatternIndex = subsong.getPositionRef(sourcePositionIndex).getPatternIndex();

        // Gets a copy of the original Pattern. Stores it for the undo.
        originalPattern = subsong.getPatternRef(sourcePositionIndex);
        auto newPattern = originalPattern;

        const auto& originalTrackIndexes = newPattern.getTrackIndexAndLinkedTrackIndex(sourceChannelIndex);

        const auto newTrackIndexes = Pattern::TrackIndexAndLinkedTrackIndex(
            originalTrackIndexes.getUnlinkedTrackIndex(),       // Only the linked Track changes.
            targetTrackIndex
        );
        newPattern.setTrackIndex(sourceChannelIndex, newTrackIndexes);

        // Applies it as a link.
        subsong.setPattern(sourcePatternIndex, newPattern);
    });

    notifyListeners();

    return true;
}

bool LinkTrack::undo()
{
    songController.performOnSubsong(subsongId, [&] (Subsong& subsong) {
        subsong.setPattern(sourcePatternIndex, originalPattern);
    });

    notifyListeners();

    return true;
}

void LinkTrack::notifyListeners() const noexcept
{
    songController.getLinkerObservers().applyOnObservers([&] (LinkerObserver* observer) noexcept {
        observer->onLinkerPositionChanged(subsongId, sourcePositionIndex, LinkerObserver::What::link);
    });
}

}   // namespace arkostracker
