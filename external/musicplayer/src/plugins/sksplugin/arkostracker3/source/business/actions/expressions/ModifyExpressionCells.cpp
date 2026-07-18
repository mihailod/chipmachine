#include "ModifyExpressionCells.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/ExpressionChangeObserver.h"

namespace arkostracker
{

ModifyExpressionCells::ModifyExpressionCells(SongController& pSongController, const bool pIsArpeggio, Id pId, const int pStartCellIndex, const int pCellCountToWorkOn,
    const bool pCorrectLoop, std::function<int(int iterationIndex, int originalValue)> pActionOnCell) noexcept :
        songController(pSongController),
        isArpeggio(pIsArpeggio),
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

bool ModifyExpressionCells::perform()
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

    auto& expressionHandler = songController.getExpressionHandler(isArpeggio);
    expressionHandler.performOnExpression(id, [&] (Expression& expression) {
        originalLoop = expression.getLoop();

        const auto initialLength = expression.getLength();
        auto currentLength = initialLength;
        modified = (startCellIndex <= initialLength);       // We can add new values at the end.
        // Don't do anything if invalid.
        if (modified) {
            auto cellOffset = 0;
            while (cellOffset < cellCountToWorkOn) {
                const auto cellIndex = startCellIndex + cellOffset;
                const auto withinBounds = cellIndex < currentLength;

                const auto readValue = expression.getValue(cellIndex);

                const auto newValue = actionOnCell(cellOffset, readValue);
                // If out of bounds, creates a new Cell.
                if (withinBounds) {
                    modifiedCells.push_back(readValue);      // For undo. Do it BEFORE setting the Cell.
                    expression.setValue(cellIndex, newValue);
                } else {
                    // Creates a new Cell.
                    expression.addValue(newValue);
                    currentLength = expression.getLength();
                    ++addedCellCount;
                }

                ++cellOffset;
            }

            // Corrects the loop, if wanted, if the last modified cell is beyond than the original end loop.
            const auto lastModifiedCellIndex = startCellIndex + cellCountToWorkOn - 1;
            if (correctLoop && (lastModifiedCellIndex > originalLoop.getEndIndex())) {
                const auto newLoop = originalLoop.withEnd(lastModifiedCellIndex);
                expression.setLoop(newLoop);
            }
        }
    });

    if (modified) {
        notify();
    }

    return modified;
}

bool ModifyExpressionCells::undo()
{
    auto& expressionHandler = songController.getExpressionHandler(isArpeggio);
    expressionHandler.performOnExpression(id, [&] (Expression& expression) {
        // Replaces the old cells.
        const auto pastLastModifiedCellIndex = startCellIndex + static_cast<int>(modifiedCells.size());
        size_t storedCellIndex = 0;
        for (auto cellIndex = startCellIndex; cellIndex < pastLastModifiedCellIndex; ++cellIndex, ++storedCellIndex) {
            const auto& storedCell = modifiedCells.at(storedCellIndex);
            expression.setValue(cellIndex, storedCell);
        }

        // Removes the added cells.
        expression.removeValues(pastLastModifiedCellIndex, addedCellCount);

        expression.setLoop(originalLoop);
    });

    notify();

    return true;
}

void ModifyExpressionCells::notify() const noexcept
{
    songController.getExpressionObservers(isArpeggio).applyOnObservers([&] (ExpressionChangeObserver* observer) noexcept {
        observer->onExpressionChanged(id,
            static_cast<unsigned int>(ExpressionChangeObserver::What::data)
            | static_cast<unsigned int>(ExpressionChangeObserver::What::loop));
    });
}

}   // namespace arkostracker
