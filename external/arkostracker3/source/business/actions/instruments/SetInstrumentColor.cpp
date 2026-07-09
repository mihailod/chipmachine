#include "SetInstrumentColor.h"

#include <utility>

#include "../../../controllers/SongController.h"

namespace arkostracker 
{

SetInstrumentColor::SetInstrumentColor(SongController& pSongController, Id pId, const juce::Colour pNewColor) noexcept :
        InstrumentAction(pSongController),
        id(std::move(pId)),
        newColor(pNewColor),
        oldColor()
{
}

bool SetInstrumentColor::perform()
{
    // Saves the current name, to be replaced. Useful for the redo.
    songController.performOnConstInstrument(id, [&](const Instrument& instrument) noexcept {
        oldColor = juce::Colour(instrument.getArgbColor());
    });

    if (oldColor == newColor) {           // Same name? nothing to do.
        return false;
    }

    // Changes the color.
    songController.performOnInstrument(id, [&](Instrument& instrument) noexcept {
        instrument.setColor(newColor.getARGB());
    });

    notify();

    return true;
}

bool SetInstrumentColor::undo()
{
    songController.performOnInstrument(id, [&](Instrument& instrument) noexcept {
        instrument.setColor(oldColor.getARGB());
    });

    notify();

    return true;
}

void SetInstrumentColor::notify() noexcept
{
    songController.getInstrumentObservers().applyOnObservers([&](InstrumentChangeObserver* observer) noexcept {
        observer->onInstrumentChanged(id, InstrumentChangeObserver::What::color);
    });
}

}   // namespace arkostracker
