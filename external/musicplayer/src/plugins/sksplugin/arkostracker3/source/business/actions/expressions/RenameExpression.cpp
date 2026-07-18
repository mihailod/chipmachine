#include "RenameExpression.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/ExpressionChangeObserver.h"

namespace arkostracker
{

RenameExpression::RenameExpression(SongController& pSongController, bool pIsArpeggio, Id pExpressionId, juce::String pNewName) noexcept :
        ExpressionAction(pSongController, pIsArpeggio),
        expressionId(std::move(pExpressionId)),
        newName(std::move(pNewName)),
        oldName()
{
}

bool RenameExpression::perform()
{
    // Saves the current name, to be replaced. Useful for the redo.
    auto& handler = getExpressionHandler();
    oldName = handler.getName(expressionId);
    if (oldName == newName) {           // Same name? nothing to do.
        return false;
    }

    handler.setName(expressionId, newName);

    notify();

    return true;
}

bool RenameExpression::undo()
{
    getExpressionHandler().setName(expressionId, oldName);
    notify();

    return true;
}

void RenameExpression::notify() const noexcept
{
    songController.getExpressionObservers(isArpeggio).applyOnObservers([&](ExpressionChangeObserver* observer) {
        observer->onExpressionChanged(expressionId, static_cast<unsigned int>(ExpressionChangeObserver::What::name));
    });
}

}   // namespace arkostracker
