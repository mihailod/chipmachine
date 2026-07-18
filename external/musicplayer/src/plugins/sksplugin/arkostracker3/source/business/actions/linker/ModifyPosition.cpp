#include "ModifyPosition.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/LinkerObserver.h"

namespace arkostracker 
{

ModifyPosition::ModifyPosition(SongController& pSongController, Id pSubsongId, const int pPositionIndex, Position pNewPosition) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        positionIndex(pPositionIndex),
        newPosition(std::move(pNewPosition)),
        oldPosition()
{
}

bool ModifyPosition::perform()
{
    const auto song = songController.getSong();
    song->performOnConstSubsong(subsongId, [&] (const Subsong& subsong) noexcept {
        // Gets the previous data, useful for redo.
        oldPosition = std::make_unique<Position>(subsong.getPosition(positionIndex));
    });

    // Any change? If not, don't do anything.
    if (newPosition == *oldPosition) {
        return false;
    }

    // Sets the new data.
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        subsong.setPosition(positionIndex, newPosition);
    });

    // Notifies.
    notifyListeners();

    return true;
}

bool ModifyPosition::undo()
{
    const auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        // Sets the old data.
        subsong.setPosition(positionIndex, *oldPosition);
    });

    // Notifies.
    notifyListeners();

    return true;
}

void ModifyPosition::notifyListeners() const noexcept
{
    songController.getLinkerObservers().applyOnObservers([&](LinkerObserver* observer) noexcept {
        observer->onLinkerPositionChanged(subsongId, positionIndex, LinkerObserver::What::all);
    });
}

}   // namespace arkostracker
