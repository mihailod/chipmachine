#include "SetPsgInstrumentMetadata.h"

#include "../../../../controllers/SongController.h"

namespace arkostracker
{

SetPsgInstrumentMetadata::SetPsgInstrumentMetadata(SongController& pSongController, Id pId, const OptionalInt pNewLoopStart, const OptionalInt pNewEnd, const OptionalBool pNewIsLoop,
                                                   const OptionalBool pNewIsRetrig, const OptionalInt pNewSpeed,
                                                   std::unordered_map<PsgSection, Loop> pNewModifiedSectionToAutoSpreadLoop) noexcept :
        songController(pSongController),
        id(std::move(pId)),
        loopStart(pNewLoopStart),
        end(pNewEnd),
        loop(pNewIsLoop),
        retrig(pNewIsRetrig),
        speed(pNewSpeed),
        modifiedSectionToAutoSpreadLoop(std::move(pNewModifiedSectionToAutoSpreadLoop)),
        oldLoop(0, 0, false),
        oldRetrig(false),
        oldSpeed(0),
        oldSectionToAutoSpreadLoop()
{
    if (pNewLoopStart.isPresent() && pNewEnd.isPresent()) {
        jassert(loopStart.getValue() <= end.getValue());
    }
    jassert(pNewLoopStart.isPresent() || pNewEnd.isPresent() || pNewIsLoop.isPresent() || pNewIsRetrig.isPresent() || pNewSpeed.isPresent()
            || !modifiedSectionToAutoSpreadLoop.empty());    // No new data??
}

bool SetPsgInstrumentMetadata::perform()
{
    songController.performOnInstrument(id, [&](Instrument& instrument) noexcept {
        auto& psgPart = instrument.getPsgPart();

        // Saves the old data, for Undo.
        oldLoop = psgPart.getMainLoop();
        auto newLoop = oldLoop;
        oldRetrig = psgPart.isInstrumentRetrig();
        oldSpeed = psgPart.getSpeed();
        oldSectionToAutoSpreadLoop = psgPart.getAutoSpreadAllLoops();

        // Sets the new data, if present.
        if (loopStart.isPresent() || end.isPresent()) {     // Used to prevent an assertion when changing one, but not the other.
            newLoop = newLoop.withPossibleStartAndEnd(loopStart, end);
        }
        if (loop.isPresent()) {
            newLoop = newLoop.withLooping(loop.getValue());
        }
        if (retrig.isPresent()) {
            psgPart.setInstrumentRetrig(retrig.getValue());
        }
        if (speed.isPresent()) {
            psgPart.setSpeed(speed.getValue());
        }
        psgPart.setMainLoop(newLoop);
        psgPart.setAutoSpreadLoops(modifiedSectionToAutoSpreadLoop);
    });

    notify();

    return true;
}

bool SetPsgInstrumentMetadata::undo()
{
    // Writes back all the stored data.
    songController.performOnInstrument(id, [&](Instrument& instrument) noexcept {
        auto& psgPart = instrument.getPsgPart();
        psgPart.setMainLoop(oldLoop);

        psgPart.setInstrumentRetrig(oldRetrig);
        psgPart.setSpeed(oldSpeed);
        psgPart.setAutoSpreadLoops(oldSectionToAutoSpreadLoop);
    });

    notify();

    return true;
}

void SetPsgInstrumentMetadata::notify() const noexcept
{
    songController.getInstrumentObservers().applyOnObservers([&](InstrumentChangeObserver* observer) noexcept {
        observer->onInstrumentChanged(id, InstrumentChangeObserver::What::other);
    });
}

}   // namespace arkostracker
