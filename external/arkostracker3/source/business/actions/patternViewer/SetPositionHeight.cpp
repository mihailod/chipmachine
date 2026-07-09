#include "SetPositionHeight.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/LinkerObserver.h"

namespace arkostracker 
{

SetPositionHeight::SetPositionHeight(SongController& pSongController, Location pLocation, int pNewHeight) noexcept :
        songController(pSongController),
        location(std::move(pLocation)),
        newHeight(pNewHeight),
        oldPosition()
{
}

bool SetPositionHeight::perform()
{
    const auto positionIndex = location.getPosition();

    const auto song = songController.getSong();
    song->performOnSubsong(location.getSubsongId(), [&] (Subsong& subsong) noexcept {
        // Gets the previous data, useful for redo. Directly gets the whole position, simpler for the undo.
        oldPosition = std::make_unique<Position>(subsong.getPosition(positionIndex));

        // Sets the new data in a copy of the Position.
        auto newPosition = *oldPosition;
        newPosition.setHeight(newHeight);

        subsong.setPosition(positionIndex, newPosition);
    });

    // Notifies.
    notifyListeners();

    return true;
}

bool SetPositionHeight::undo()
{
    const auto positionIndex = location.getPosition();

    auto song = songController.getSong();
    song->performOnSubsong(location.getSubsongId(), [&] (Subsong& subsong) noexcept {
        // Sets the old data.
        subsong.setPosition(positionIndex, *oldPosition);
    });

    // Notifies.
    notifyListeners();

    return true;
}

void SetPositionHeight::notifyListeners() noexcept
{
    songController.getLinkerObservers().applyOnObservers([&](LinkerObserver* observer) noexcept {
        observer->onLinkerPositionChanged(location.getSubsongId(), location.getPosition(), LinkerObserver::What::positionHeight);
    });
}


}   // namespace arkostracker

