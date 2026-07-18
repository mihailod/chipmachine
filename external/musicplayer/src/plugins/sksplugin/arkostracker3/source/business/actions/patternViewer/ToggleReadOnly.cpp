#include "ToggleReadOnly.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/TrackChangeObserver.h"

namespace arkostracker
{

ToggleReadOnly::ToggleReadOnly(SongController& pSongController, Id pSubsongId, const std::set<int>& pTrackIndexes, OptionalInt pSpeedTrackIndex,
    OptionalInt pEventTrackIndex) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        originalTrackIndexes(pTrackIndexes),
        originalSpeedTrackIndex(pSpeedTrackIndex),
        originalEventTrackIndex(pEventTrackIndex)
{
}

bool ToggleReadOnly::perform()
{
    // Anything to do?
    if (originalTrackIndexes.empty() && originalSpeedTrackIndex.isAbsent() && originalEventTrackIndex.isAbsent()) {
        jassertfalse;       // The UI should prevent this.
        return false;
    }

    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        for (const auto trackIndex : originalTrackIndexes) {
            subsong.getTrackRefFromIndex(trackIndex).toggleReadOnly();
        }
        // The same for the possible Special Tracks.
        if (originalSpeedTrackIndex.isPresent()) {
            subsong.getSpecialTrackRefFromIndex(originalSpeedTrackIndex.getValue(), true).toggleReadOnly();
        }
        if (originalEventTrackIndex.isPresent()) {
            subsong.getSpecialTrackRefFromIndex(originalEventTrackIndex.getValue(), false).toggleReadOnly();
        }
    });

    // Notifies.
    notifyListeners();

    return true;
}

bool ToggleReadOnly::undo()
{
    // Since it is a toggle, performing the action again will revert the result.
    return perform();
}

void ToggleReadOnly::notifyListeners() const noexcept
{
    songController.getTrackObservers().applyOnObservers([&](TrackChangeObserver* observer) noexcept {
        observer->onTrackDataChanged(subsongId);
    });
}

}   // namespace arkostracker
