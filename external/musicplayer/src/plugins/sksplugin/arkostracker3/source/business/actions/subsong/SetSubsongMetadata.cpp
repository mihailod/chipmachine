#include "SetSubsongMetadata.h"

#include <utility>

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/SubsongMetadataObserver.h"

namespace arkostracker 
{

SetSubsongMetadata::SetSubsongMetadata(SongController& pSongController, Id pSubsongId, Properties pMetadata) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        newMetadata(std::move(pMetadata)),
        oldMetadata()
{
}

bool SetSubsongMetadata::perform()
{
    auto changed = false;
    songController.performOnSubsong(subsongId, [&](Subsong& subsong) noexcept {
        // Gets the previous metadata for Undo.
        oldMetadata = std::make_unique<Properties>(subsong.getMetadata());

        // Sets the new ones, if different.
        if (newMetadata != *oldMetadata) {
            newMetadata.setMetadataToSubsong(subsong);

            changed = true;
        }
    });

    if (changed) {
        notify();
    }

    return changed;
}

bool SetSubsongMetadata::undo()
{
    songController.performOnSubsong(subsongId, [&](Subsong& subsong) noexcept {
        oldMetadata->setMetadataToSubsong(subsong);
    });

    notify();

    return true;
}

void SetSubsongMetadata::notify() const noexcept
{
    songController.getSubsongMetadataObservers().applyOnObservers([&](SubsongMetadataObserver* observer) noexcept {
        observer->onSubsongMetadataChanged(subsongId, SubsongMetadataObserver::What::subsongMetadata);
    });
}

}   // namespace arkostracker
