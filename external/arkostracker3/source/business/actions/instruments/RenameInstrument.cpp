#include "RenameInstrument.h"

#include <utility>

#include "../../../controllers/SongController.h"

namespace arkostracker 
{

RenameInstrument::RenameInstrument(SongController& pSongController, Id pId, juce::String pNewName) noexcept :
        InstrumentAction(pSongController),
        id(std::move(pId)),
        newName(std::move(pNewName)),
        oldName()
{
}

bool RenameInstrument::perform()
{
    // Saves the current name, to be replaced. Useful for the redo.
    songController.performOnConstInstrument(id, [&](const Instrument& instrument) noexcept {
        oldName = instrument.getName();
    });

    if (oldName == newName) {           // Same name? nothing to do.
        return false;
    }

    // Renames.
    songController.performOnInstrument(id, [&](Instrument& instrument) noexcept {
        instrument.setName(newName);
    });

    notify();

    return true;
}

bool RenameInstrument::undo()
{
    songController.performOnInstrument(id, [&](Instrument& instrument) noexcept {
        instrument.setName(oldName);
    });

    notify();

    return true;
}

void RenameInstrument::notify() noexcept
{
    songController.getInstrumentObservers().applyOnObservers([&](InstrumentChangeObserver* observer) noexcept {
        observer->onInstrumentChanged(id, InstrumentChangeObserver::What::name);
    });
}


}   // namespace arkostracker

