#include "DeleteExpressionCell.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/ExpressionChangeObserver.h"
#include "../../song/validation/LoopChange.h"

namespace arkostracker
{

DeleteExpressionCell::DeleteExpressionCell(SongController& pSongController, bool pIsArpeggio, Id pId, int pCellIndex) noexcept :
        ExpressionAction(pSongController, pIsArpeggio),
        id(std::move(pId)),
        cellIndex(pCellIndex),
        originalLoop(),
        originalValue(0)
{
}

bool DeleteExpressionCell::perform()
{
    if (cellIndex < 0) {
        jassertfalse;           // Not supposed to happen!
        return false;
    }

    auto success = false;
    getExpressionHandler().performOnExpression(id, [&](Expression& expression) noexcept {
        const auto initialLength = expression.getLength();
        originalLoop = expression.getLoop();

        // Cannot do anything after the end, won't remove a unique Cell.
        if ((initialLength > 1) && (cellIndex < initialLength)) {
            success = true;

            const auto newLoop = LoopChange::remove(cellIndex, originalLoop, initialLength);

            originalValue = expression.getValue(cellIndex);
            expression.removeValues(cellIndex, 1);

            expression.setLoop(newLoop);
        }
    });

    // Notifies.
    if (success) {
        notifyChange();
    }

    return success;
}

bool DeleteExpressionCell::undo()
{
    getExpressionHandler().performOnExpression(id, [&](Expression& expression) noexcept {
        expression.insertValue(cellIndex, originalValue);
        expression.setLoop(originalLoop);
    });

    // Notifies.
    notifyChange();

    return true;
}

void DeleteExpressionCell::notifyChange() const noexcept
{
    songController.getExpressionObservers(isArpeggio).applyOnObservers([&](ExpressionChangeObserver* observer) noexcept {
        observer->onExpressionCellChanged(id, cellIndex, true);
        observer->onExpressionChanged(id, static_cast<unsigned int>(ExpressionChangeObserver::What::loop));
    });
}

}   // namespace arkostracker
