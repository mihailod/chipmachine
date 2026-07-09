#include "SetTrackName.h"

#include "../../../controllers/SongController.h"

namespace arkostracker
{

SetTrackName::SetTrackName(SongController& pSongController, Id pSubsongId, int pPositionIndex, int pChannelIndex, const juce::String& pNewName) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        positionIndex(pPositionIndex),
        channelIndex(pChannelIndex),
        newName(pNewName.trim()),
        oldName()
{
}

bool SetTrackName::perform()
{
    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
        // Gets the previous data, useful for redo.
        oldName = subsong.getConstTrackRefFromPosition(positionIndex, channelIndex).getName();
    });

    // Any change? If not, don't do anything.
    if (newName == oldName) {
        return false;
    }

    // Sets the new data.
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        subsong.setTrackName(positionIndex, channelIndex, newName);
    });

    // Notifies.
    notifyListeners();

    return true;
}

bool SetTrackName::undo()
{
    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        subsong.setTrackName(positionIndex, channelIndex, oldName);
    });

    // Notifies.
    notifyListeners();

    return true;
}

void SetTrackName::notifyListeners() noexcept
{
    songController.getTrackObservers().applyOnObservers([&] (TrackChangeObserver* observer) noexcept {
        observer->onTrackMetaDataChanged(subsongId);
    });
}

}   // namespace arkostracker
