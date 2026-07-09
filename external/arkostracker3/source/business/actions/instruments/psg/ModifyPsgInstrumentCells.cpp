#include "ModifyPsgInstrumentCells.h"

#include "../../../../controllers/SongController.h"

namespace arkostracker
{

ModifyPsgInstrumentCells::ModifyPsgInstrumentCells(SongController& pSongController, Id pId, const int pStartCellIndex, const int pCellCountToWorkOn,
                                                   const bool pCorrectLoop, std::function<PsgInstrumentCell(int iterationIndex,
                                                   const PsgInstrumentCell& cell)> pActionOnCell) noexcept :
        songController(pSongController),
        id(std::move(pId)),
        startCellIndex(pStartCellIndex),
        cellCountToWorkOn(pCellCountToWorkOn),
        correctLoop(pCorrectLoop),
        actionOnCell(std::move(pActionOnCell)),
        originalLoop(),
        addedCellCount(0),
        modifiedCells()
{
    jassert(cellCountToWorkOn >= 0);
}

bool ModifyPsgInstrumentCells::perform()
{
    if (startCellIndex < 0) {
        jassertfalse;           // Shouldn't happen!
        return false;
    }
    if (cellCountToWorkOn <= 0) {
        return false;
    }

    auto modified = false;
    addedCellCount = 0;
    modifiedCells.clear();

    songController.performOnInstrument(id, [&] (Instrument& instrument) noexcept {
        auto& psgPart = instrument.getPsgPart();

        originalLoop = psgPart.getMainLoop();

        const auto initialLength = psgPart.getLength();
        auto currentLength = initialLength;
        modified = (startCellIndex <= initialLength);       // We can add new values at the end.
        // Don't do anything if invalid.
        if (modified) {
            auto cellOffset = 0;
            while (cellOffset < cellCountToWorkOn) {
                const auto cellIndex = startCellIndex + cellOffset;
                const auto withinBounds = cellIndex < currentLength;

                // The getCell returns a generic Cell if out of bounds. However, this proves not user-friendly when generating cells.
                // It is better to use the previous cell as a template, so that if a first cell was SoftToHard, the generated cells are also SoftToHard.
                const auto& readCell = withinBounds && (cellIndex >= 0) ? psgPart.getCellRefConst(cellIndex) : psgPart.getCellRefConst(cellIndex - 1);

                const auto newCell = actionOnCell(cellOffset, readCell);
                // If out of bounds, creates a new Cell.
                if (withinBounds) {
                    modifiedCells.push_back(readCell);      // For undo. Do it BEFORE setting the Cell.
                    psgPart.setCell(cellIndex, newCell);
                } else {
                    // Creates a new Cell.
                    psgPart.addCell(newCell);
                    currentLength = psgPart.getLength();
                    ++addedCellCount;
                }

                ++cellOffset;
            }

            // Corrects the loop, if wanted, if the last modified cell is beyond than the original end loop.
            const auto lastModifiedCellIndex = startCellIndex + cellCountToWorkOn - 1;
            if (correctLoop && (lastModifiedCellIndex > originalLoop.getEndIndex())) {
                const auto newLoop = originalLoop.withEnd(lastModifiedCellIndex);
                psgPart.setMainLoop(newLoop);
            }
        }
    });

    if (modified) {
        notify();
    }

    return modified;
}

bool ModifyPsgInstrumentCells::undo()
{
    songController.performOnInstrument(id, [&] (Instrument& instrument) noexcept {
        auto& psgPart = instrument.getPsgPart();

        // Replaces the old cells.
        const auto pastLastModifiedCellIndex = startCellIndex + static_cast<int>(modifiedCells.size());
        size_t storedCellIndex = 0;
        for (auto cellIndex = startCellIndex; cellIndex < pastLastModifiedCellIndex; ++cellIndex, ++storedCellIndex) {
            const auto& storedCell = modifiedCells.at(storedCellIndex);
            psgPart.setCell(cellIndex, storedCell);
        }

        // Removes the added cells.
        psgPart.removeCells(pastLastModifiedCellIndex, addedCellCount);

        psgPart.setMainLoop(originalLoop);
    });

    notify();

    return true;
}

void ModifyPsgInstrumentCells::notify() const noexcept
{
    songController.getInstrumentObservers().applyOnObservers([&] (InstrumentChangeObserver* observer) noexcept {
        observer->onInstrumentChanged(id, InstrumentChangeObserver::What::other);
    });
}

}   // namespace arkostracker
