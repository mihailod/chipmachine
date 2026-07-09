#include "InsertOrRemoveCellAt.h"

#include <utility>

#include "../../../../controllers/SongController.h"

namespace arkostracker 
{

InsertOrRemoveCellAt::InsertOrRemoveCellAt(SongController& pSongController, Location pLocation, const CursorLocation& pCursorLocation) noexcept :
        songController(pSongController),
        location(std::move(pLocation)),
        cursorLocation(pCursorLocation),
        removedCell(),
        removedSpecialCell(),
        trackIndexToPerformOn(-1)
{
}

bool InsertOrRemoveCellAt::perform()
{
    switch (cursorLocation.getTrackType()) {
        default:
        case TrackType::line:
            jassertfalse;               // Shouldn't happen.
            return false;
        case TrackType::normal:
            performOnNormalTrackType();
            break;
        case TrackType::speed:
            performOnSpecialTrackType(true);
            break;
        case TrackType::event:
            performOnSpecialTrackType(false);
            break;
    }

    notifyListeners();

    return true;
}

bool InsertOrRemoveCellAt::undo()
{
    switch (cursorLocation.getTrackType()) {
        default:
        case TrackType::line:
            jassertfalse;               // Shouldn't happen.
            return false;
        case TrackType::normal:
            undoOnNormalTrackType();
            break;
        case TrackType::speed:
            undoOnSpecialTrackType(true);
            break;
        case TrackType::event:
            undoOnSpecialTrackType(false);
            break;
    }
    notifyListeners();

    return true;
}

void InsertOrRemoveCellAt::performOnNormalTrackType() noexcept
{
    const auto subsongId = location.getSubsongId();
    songController.performOnSubsong(subsongId, [&](Subsong& subsong) noexcept {
        // Finds the Track Index.
        const auto& pattern = subsong.getPatternRef(location.getPosition());
        const auto trackIndex = pattern.getCurrentTrackIndex(cursorLocation.getChannelIndex());
        trackIndexToPerformOn = trackIndex;     // So that we don't have to find it again.

        removedCell = performActionOnTrack(subsong, trackIndex, location.getLine());
    });
}

void InsertOrRemoveCellAt::performOnSpecialTrackType(const bool isSpeedTrack) noexcept
{
    const auto subsongId = location.getSubsongId();
    songController.performOnSubsong(subsongId, [&](Subsong& subsong) noexcept {
        // Finds the SpecialTrack Index.
        const auto& pattern = subsong.getPatternRef(location.getPosition());
        const auto specialTrackIndex = pattern.getCurrentSpecialTrackIndex(isSpeedTrack);
        trackIndexToPerformOn = specialTrackIndex;     // So that we don't have to find it again.

        removedSpecialCell = performActionOnSpecialTrack(subsong, isSpeedTrack, specialTrackIndex, location.getLine());
    });
}

void InsertOrRemoveCellAt::undoOnNormalTrackType() noexcept
{
    jassert(trackIndexToPerformOn >= 0);            // Not defined!?

    const auto subsongId = location.getSubsongId();
    songController.performOnSubsong(subsongId, [&](Subsong& subsong) noexcept {
        undoActionOnNormalTrack(subsong, trackIndexToPerformOn, location.getLine(), removedCell);
    });
}

void InsertOrRemoveCellAt::undoOnSpecialTrackType(const bool isSpeedTrack) noexcept
{
    jassert(trackIndexToPerformOn >= 0);            // Not defined!?

    const auto subsongId = location.getSubsongId();
    songController.performOnSubsong(subsongId, [&](Subsong& subsong) noexcept {
        undoActionOnSpecialTrack(subsong, isSpeedTrack, trackIndexToPerformOn, location.getLine(), removedSpecialCell);
    });
}

void InsertOrRemoveCellAt::notifyListeners() noexcept
{
    songController.getTrackObservers().applyOnObservers([&](TrackChangeObserver* observer) noexcept {
        observer->onTrackDataChanged(location.getSubsongId());
    });
}


}   // namespace arkostracker

