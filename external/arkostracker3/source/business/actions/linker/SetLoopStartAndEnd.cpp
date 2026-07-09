#include "SetLoopStartAndEnd.h"

#include <utility>

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/SubsongMetadataObserver.h"

namespace arkostracker 
{

SetLoopStartAndEnd::SetLoopStartAndEnd(SongController& pSongController, Id pSubsongId, int pLoopStartPosition, int pEndPosition) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        loopStartPosition(pLoopStartPosition),
        endPosition(pEndPosition),

        oldLoopStartPosition(0),
        oldEndPosition(0)
{
}

bool SetLoopStartAndEnd::perform()
{
    // Gets the previous data of both markers, useful for redo.
    auto performed = false;
    auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        oldLoopStartPosition = subsong.getLoopStartPosition();
        oldEndPosition = subsong.getEndPosition();

        // Any difference? Then do something.
        if ((oldLoopStartPosition != loopStartPosition) || (oldEndPosition != endPosition)) {
            performed = true;

            subsong.setLoopAndEndStartPosition(loopStartPosition, endPosition);
        }
    });

    // Notifies.
    if (performed) {
        notifyListeners();
    }

    return performed;
}

bool SetLoopStartAndEnd::undo()
{
    auto song = songController.getSong();
    song->performOnSubsong(subsongId, [&] (Subsong& subsong) noexcept {
        subsong.setLoopAndEndStartPosition(oldLoopStartPosition, oldEndPosition);
    });

    // Notifies.
    notifyListeners();

    return true;
}

void SetLoopStartAndEnd::notifyListeners() const noexcept
{
    songController.getSubsongMetadataObservers().applyOnObservers([&](SubsongMetadataObserver* observer) noexcept {
        observer->onSubsongMetadataChanged(subsongId, SubsongMetadataObserver::What::loop);
    });
}


}   // namespace arkostracker

