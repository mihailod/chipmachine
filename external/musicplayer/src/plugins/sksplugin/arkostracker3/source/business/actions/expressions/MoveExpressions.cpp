#include "MoveExpressions.h"

#include "../../../controllers/SongController.h"

namespace arkostracker
{

MoveExpressions::MoveExpressions(SongController& pSongController, const bool pIsArpeggio, std::set<int> pIndexesToMove, const int pTargetIndex) noexcept:
        ExpressionAction(pSongController, pIsArpeggio),
        MoveItems(std::move(pIndexesToMove), pTargetIndex)
{
}

bool MoveExpressions::perform()
{
    return performAction();
}

bool MoveExpressions::undo()
{
    return undoAction();
}


// MoveItems method implementations.
// =================================================

int MoveExpressions::getItemCount() const noexcept
{
    const auto& expressionHandler = getExpressionHandler();
    return expressionHandler.getCount();
}

void MoveExpressions::setSelectedItem(const int index, const bool forceSingleSelection) noexcept
{
    setSelectedExpression(index, forceSingleSelection);
}

bool MoveExpressions::performActionOnMapper(Remapper<Expression>& pRemapper) noexcept
{
    return performOnMapper(pRemapper);
}

bool MoveExpressions::undoActionOnMapper(Remapper<Expression>& pRemapper) noexcept
{
    return undoOnMapper(pRemapper);
}

}   // namespace arkostracker
