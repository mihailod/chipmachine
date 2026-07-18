#include "SetExpressionCellMetadata.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/ExpressionChangeObserver.h"

namespace arkostracker
{

SetExpressionCellMetadata::SetExpressionCellMetadata(SongController& pSongController, bool pIsArpeggio, Id pExpressionId, OptionalInt pNewLoopStart, OptionalInt pNewEnd,
                                                     OptionalInt pNewSpeed, OptionalInt pNewShift) noexcept :
        ExpressionAction(pSongController, pIsArpeggio),
        expressionId(std::move(pExpressionId)),
        loopStart(pNewLoopStart),
        end(pNewEnd),
        speed(pNewSpeed),
        shift(pNewShift),
        oldLoop(0, 0, true),
        oldSpeed(0),
        oldShift(0)
{
    if (pNewLoopStart.isPresent() && pNewEnd.isPresent()) {
        jassert(pNewLoopStart.getValue() <= pNewEnd.getValue());
    }
    jassert(pNewLoopStart.isPresent() || pNewEnd.isPresent() || pNewSpeed.isPresent() || pNewShift.isPresent());    // No new data??
}

bool SetExpressionCellMetadata::perform()
{
    songController.getExpressionHandler(isArpeggio).performOnExpression(expressionId, [&](Expression& expression) noexcept {
        // Saves the old data, for Undo.
        oldLoop = expression.getLoop();
        Loop newLoop = oldLoop;
        oldSpeed = expression.getSpeed();
        oldShift = expression.getShift();

        // Sets the new data, if present.
        if (loopStart.isPresent() || end.isPresent()) {     // Used to prevent an assertion when changing one, but not the other.
            newLoop = newLoop.withPossibleStartAndEnd(loopStart, end);
        }
        if (speed.isPresent()) {
            expression.setSpeed(speed.getValue());
        }
        if (shift.isPresent()) {
            expression.setShift(shift.getValue());
        }
        expression.setLoop(newLoop);
    });

    notify();

    return true;
}

bool SetExpressionCellMetadata::undo()
{
    songController.getExpressionHandler(isArpeggio).performOnExpression(expressionId, [&](Expression& expression) noexcept {
        expression.setLoop(oldLoop);
        expression.setSpeed(oldSpeed);
        expression.setShift(oldShift);
    });

    notify();

    return true;
}

void SetExpressionCellMetadata::notify() const noexcept
{
    songController.getExpressionObservers(isArpeggio).applyOnObservers([&](ExpressionChangeObserver* observer) noexcept {
        observer->onExpressionChanged(expressionId,
            static_cast<unsigned int>(ExpressionChangeObserver::What::speed)                // We don't bother checking exactly what has changed...
            | static_cast<unsigned int>(ExpressionChangeObserver::What::shift)
            | static_cast<unsigned int>(ExpressionChangeObserver::What::loop)
        );
    });
}

}   // namespace arkostracker
