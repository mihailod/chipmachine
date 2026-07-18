#include "SetSpecialTrackName.h"

#include "../../../controllers/SongController.h"

namespace arkostracker
{

SetSpecialTrackName::SetSpecialTrackName(SongController& pSongController, Id pSubsongId, int pPositionIndex, bool pIsSpeedTrack, const juce::String& pNewName) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        positionIndex(pPositionIndex),
        isSpeedTrack(pIsSpeedTrack),
        newName(pNewName.trim()),
        oldName()
{
}

bool SetSpecialTrackName::perform()
{
    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
        // Gets the previous data, useful for redo.
        oldName = subsong.getSpecialTrackRefFromPosition(positionIndex, isSpeedTrack).getName();
    });

    // Any change? If not, don't do anything.
    if (newName == oldName) {
        return false;
    }

    // Sets the new data.
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        subsong.setSpecialTrackName(positionIndex, isSpeedTrack, newName);
    });

    // Notifies.
    notifyListeners();

    return true;
}

bool SetSpecialTrackName::undo()
{
    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        subsong.setSpecialTrackName(positionIndex, isSpeedTrack, oldName);
    });

    // Notifies.
    notifyListeners();

    return true;
}

void SetSpecialTrackName::notifyListeners() noexcept
{
    songController.getTrackObservers().applyOnObservers([&] (TrackChangeObserver* observer) noexcept {
        observer->onTrackMetaDataChanged(subsongId);
    });
}

}   // namespace arkostracker
