#include "DuplicatePsgInstrumentCell.h"

#include "../../../../controllers/SongController.h"
#include "../../../song/validation/LoopChange.h"

namespace arkostracker
{

DuplicatePsgInstrumentCell::DuplicatePsgInstrumentCell(SongController& pSongController, Id pId, const int pCellIndex) noexcept :
        songController(pSongController),
        id(std::move(pId)),
        cellIndex(pCellIndex),
        originalLoop(),
        originalAutoSpreadLoops()
{
}

bool DuplicatePsgInstrumentCell::perform()
{
    if (cellIndex < 0) {
        jassertfalse;           // Shouldn't happen!
        return false;
    }

    auto success = false;
    songController.performOnInstrument(id, [&](Instrument& instrument) noexcept {
        auto& psgPart = instrument.getPsgPart();
        const auto initialLength = psgPart.getLength();
        originalLoop = psgPart.getMainLoop();

        // Cannot do anything after the end.
        if (cellIndex < initialLength) {
            success = true;

            const auto& newCell = psgPart.getCellRefConst(cellIndex);
            psgPart.insertCell(cellIndex, newCell);

            const auto newLoop = LoopChange::insert(cellIndex, originalLoop);
            psgPart.setMainLoop(newLoop);

            // Auto-spread loops. Shifts them if necessary.
            auto modifiedAutoSpreadLoops = psgPart.getAutoSpreadAllLoops();
            originalAutoSpreadLoops = modifiedAutoSpreadLoops;
            for (auto& entry : modifiedAutoSpreadLoops) {
                const auto& psgSection = entry.first;
                const auto autoSpreadLoop = entry.second;

                const auto newAutoSpreadLoop = LoopChange::insert(cellIndex, autoSpreadLoop);
                modifiedAutoSpreadLoops.at(psgSection) = newAutoSpreadLoop;

                psgPart.setAutoSpreadLoops(modifiedAutoSpreadLoops);
            }
        }
    });

    if (success) {
        notify();
    }

    return success;
}

bool DuplicatePsgInstrumentCell::undo()
{
    songController.performOnInstrument(id, [&](Instrument& instrument) noexcept {
        auto& psgPart = instrument.getPsgPart();
        psgPart.setMainLoop(originalLoop);
        psgPart.setAutoSpreadLoops(originalAutoSpreadLoops);
        psgPart.removeCells(cellIndex + 1, 1);
    });

    notify();

    return true;
}

void DuplicatePsgInstrumentCell::notify() const noexcept
{
    songController.getInstrumentObservers().applyOnObservers([&](InstrumentChangeObserver* observer) noexcept {
        // If adding new cells, all of them must be refreshed.
        observer->onPsgInstrumentCellChanged(id, cellIndex, true);
        observer->onInstrumentChanged(id, InstrumentChangeObserver::What::other);
    });
}

}   // namespace arkostracker
