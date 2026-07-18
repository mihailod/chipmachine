#include "SubsongActionHelper.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/MainController.h"
#include "../../../controllers/AudioController.h"
#include "../../../controllers/PlayerController.h"
#include "../../../controllers/observers/LinkerObserver.h"

namespace arkostracker 
{

void SubsongActionHelper::notifyAndShowSubsong(SongController& songController, const Id& subsongIdToShow) noexcept
{
    songController.getMainController().getAudioController().stopPlayer();

    const auto newLocation = Location(subsongIdToShow, 0);
    const auto newLocationPlusOnePosition = newLocation.withPosition(1);

    songController.setCurrentLocation(newLocation, false);
    songController.getMainController().getPlayerController().setCurrentlyPlayedLocation(newLocation);

    // A bit hackish, but since the player is stopped, the observers such as the MainToolBarController are not noticed of the change
    // of Subsong (the SongPlayer was sending the events), so are still checking on the wrong Subsong, possibly deleted (assertions raised).
    // So we notify the observers by ourselves.
    songController.getMainController().getPlayerController().getSongPlayerObservers().applyOnObservers([&](SongPlayerObserver* observer) noexcept {
        SongPlayerObserver::Locations newLocations(newLocation, newLocation, newLocationPlusOnePosition, newLocation);
        observer->onPlayerNewLocations(newLocations);
    });

    // Done AFTER the observers above, else the linker can crash if the position doesn't exist anymore... This is fragile.
    songController.getLinkerObservers().applyOnObservers([&](LinkerObserver* observer) noexcept {
        observer->onLinkerPositionsInvalidated(subsongIdToShow, { });
    });
}

}   // namespace arkostracker
