#include "DeletePsgInstrumentCell.h"

#include "../../../../controllers/SongController.h"
#include "../../../song/validation/LoopChange.h"

namespace arkostracker
{

DeletePsgInstrumentCell::DeletePsgInstrumentCell(SongController& pSongController, Id pId, const int pCellIndex) noexcept :
        songController(pSongController),
        id(std::move(pId)),
        cellIndex(pCellIndex),
        originalLoop(),
        originalAutoSpreadLoops(),
        originalCell()
{
}

bool DeletePsgInstrumentCell::perform()
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

        // Cannot do anything after the end, won't remove a unique Cell.
        if ((initialLength > 1) && (cellIndex < initialLength)) {
            success = true;

            originalCell = psgPart.getCellRefConst(cellIndex);
            psgPart.removeCells(cellIndex, 1);

            const auto newLoop = LoopChange::remove(cellIndex, originalLoop, initialLength);
            psgPart.setMainLoop(newLoop);

            // Auto-spread loops. Shifts them if necessary.
            auto modifiedAutoSpreadLoops = psgPart.getAutoSpreadAllLoops();
            originalAutoSpreadLoops = modifiedAutoSpreadLoops;
            for (auto& entry : modifiedAutoSpreadLoops) {
                const auto& psgSection = entry.first;
                const auto autoSpreadLoop = entry.second;

                const auto newAutoSpreadLoop = LoopChange::remove(cellIndex, autoSpreadLoop, initialLength);
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

bool DeletePsgInstrumentCell::undo()
{
    songController.performOnInstrument(id, [&](Instrument& instrument) noexcept {
        auto& psgPart = instrument.getPsgPart();
        psgPart.insertCell(cellIndex, originalCell);
        psgPart.setMainLoop(originalLoop);
        psgPart.setAutoSpreadLoops(originalAutoSpreadLoops);
    });

    notify();

    return true;
}

void DeletePsgInstrumentCell::notify() const noexcept
{
    songController.getInstrumentObservers().applyOnObservers([&](InstrumentChangeObserver* observer) noexcept {
        // If removing new cells, all of them must be refreshed.
        observer->onPsgInstrumentCellChanged(id, cellIndex, true);
        observer->onInstrumentChanged(id, InstrumentChangeObserver::What::other);
    });
}

}   // namespace arkostracker
