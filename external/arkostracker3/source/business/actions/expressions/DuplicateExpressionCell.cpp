#include "DuplicateExpressionCell.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/ExpressionChangeObserver.h"
#include "../../song/validation/LoopChange.h"

namespace arkostracker
{

DuplicateExpressionCell::DuplicateExpressionCell(SongController& pSongController, bool pIsArpeggio, Id pId, int pCellIndex) noexcept :
        ExpressionAction(pSongController, pIsArpeggio),
        id(std::move(pId)),
        cellIndex(pCellIndex),
        originalLoop()
{
}

bool DuplicateExpressionCell::perform()
{
    if (cellIndex < 0) {
        jassertfalse;           // Not supposed to happen!
        return false;
    }

    auto success = false;
    getExpressionHandler().performOnExpression(id, [&](Expression& expression) noexcept {
        const auto initialLength = expression.getLength();
        originalLoop = expression.getLoop();

        // Cannot do anything after the end.
        if (cellIndex < initialLength) {
            success = true;

            const auto newLoop = LoopChange::insert(cellIndex, originalLoop);

            const auto& newCell = expression.getValue(cellIndex);
            expression.insertValue(cellIndex, newCell);

            expression.setLoop(newLoop);
        }
    });

    // Notifies.
    if (success) {
        notifyChange();
    }

    return success;
}

bool DuplicateExpressionCell::undo()
{
    getExpressionHandler().performOnExpression(id, [&](Expression& expression) noexcept {
        expression.setLoop(originalLoop);
        expression.removeValues(cellIndex + 1, 1);
    });

    // Notifies.
    notifyChange();

    return true;
}

void DuplicateExpressionCell::notifyChange() const noexcept
{
    songController.getExpressionObservers(isArpeggio).applyOnObservers([&](ExpressionChangeObserver* observer) noexcept {
        observer->onExpressionCellChanged(id, cellIndex, true);
        observer->onExpressionChanged(id, static_cast<unsigned int>(ExpressionChangeObserver::What::loop));
    });
}

}   // namespace arkostracker
