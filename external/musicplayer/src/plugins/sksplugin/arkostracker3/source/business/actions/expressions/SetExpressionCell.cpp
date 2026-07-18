#include "SetExpressionCell.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/ExpressionChangeObserver.h"

namespace arkostracker
{

SetExpressionCell::SetExpressionCell(SongController& pSongController, bool pIsArpeggio, Id pId, int pCellIndex, int pValue) noexcept :
        ExpressionAction(pSongController, pIsArpeggio),
        id(std::move(pId)),
        cellIndex(pCellIndex),
        newValue(pValue),
        originalEnd(0),
        newEnd(0),
        originalLength(),
        originalValue(-1),
        createdBarCount(0),
        refreshIndexStart(cellIndex)        // By default.
{
}

bool SetExpressionCell::perform()
{
    if (cellIndex < 0) {
        jassertfalse;           // Not supposed to happen!
        return false;
    }

    auto modified = false;
    auto mustRefreshPastIndex = false;
    getExpressionHandler().performOnExpression(id, [&](Expression& expression) noexcept {
        originalLength = expression.getLength();
        originalEnd = expression.getEnd();
        newEnd = originalEnd;           // For now.

        if (cellIndex < originalLength) {
            // Classic behavior. If the value is the same, don't do anything.
            createdBarCount = 0;
            originalValue = expression.getValue(cellIndex);          // Manages out of bounds.
            if (originalValue != newValue) {
                expression.setValue(cellIndex, newValue);
                modified = true;
            }
            refreshIndexStart = cellIndex;

            // Sets the end? Handy if the loop end is before the modified Cell.
            if (cellIndex > originalEnd) {
                newEnd = cellIndex;
                expression.setEnd(newEnd);
            }
        } else {
            // Must create new items. Spread the new value.
            createdBarCount = cellIndex - originalLength + 1;
            for (auto i = 0; i < createdBarCount; ++i) {
                expression.addValue(newValue);
            }
            refreshIndexStart = originalLength;      // Refreshes from here.

            // If the loop was at the end, changes it. Handy for the user.
            const auto lastIndex = (originalLength - 1);
            if (originalEnd == lastIndex) {
                newEnd += createdBarCount;
                expression.setEnd(newEnd);
            }
            modified = true;
            mustRefreshPastIndex = true;
        }
    });

    if (!modified) {
        return false;
    }

    // Notifies.
    notifyChange(mustRefreshPastIndex);

    return true;
}

bool SetExpressionCell::undo()
{
    auto mustRefreshPastIndex = false;

    getExpressionHandler().performOnExpression(id, [&](Expression& expression) noexcept {
        expression.setEnd(originalEnd);
        if (createdBarCount == 0) {
            // No new cells were added. Simply restores the value.
            expression.setValue(cellIndex, originalValue);
        } else {
            // The new cells must be removed. Not much to do, the new data is in the removed part.
            expression.removeValues(refreshIndexStart, createdBarCount);
            mustRefreshPastIndex = true;
        }
    });

    // Notifies.
    notifyChange(mustRefreshPastIndex);

    return true;
}

void SetExpressionCell::notifyChange(const bool mustRefreshPast) const noexcept
{
    songController.getExpressionObservers(isArpeggio).applyOnObservers([&](ExpressionChangeObserver* observer) noexcept {
        observer->onExpressionCellChanged(id, refreshIndexStart, mustRefreshPast);
        if (newEnd != originalEnd) {
            // Notifies the end change.
            observer->onExpressionChanged(id, static_cast<unsigned int>(ExpressionChangeObserver::What::loop));
        }
    });
}

}   // namespace arkostracker
