#include "ModifyPositionMarker.h"

#include <utility>

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/LinkerObserver.h"

namespace arkostracker 
{

ModifyPositionMarker::ModifyPositionMarker(SongController& pSongController, Id pSubsongId, int pPositionIndex, juce::String pNewName, juce::Colour pNewColor) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        positionIndex(pPositionIndex),
        newName(std::move(pNewName)),
        newColor(pNewColor),
        oldName(),
        oldColor()
{
}

bool ModifyPositionMarker::perform()
{
    // Gets the previous data, useful for redo.
    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        const auto& pair = subsong.getPositionMarker(positionIndex);
        oldName = pair.first;
        oldColor = juce::Colour(pair.second);

        // Sets the new data.
        subsong.setPositionMarkerData(positionIndex, newName, newColor.getARGB());
    });

    // Notifies.
    notifyListeners();

    return true;
}

bool ModifyPositionMarker::undo()
{
    // Sets the old data.
    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        subsong.setPositionMarkerData(positionIndex, oldName, oldColor.getARGB());
    });

    // Notifies.
    notifyListeners();

    return true;
}

void ModifyPositionMarker::notifyListeners() const noexcept
{
    songController.getLinkerObservers().applyOnObservers([&](LinkerObserver* observer) {
        observer->onLinkerPositionChanged(subsongId, positionIndex, LinkerObserver::What::marker);
    });
}

}   // namespace arkostracker
