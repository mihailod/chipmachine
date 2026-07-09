#include "DeleteExpressions.h"

#include "../../../controllers/SongController.h"

namespace arkostracker
{

DeleteExpressions::DeleteExpressions(SongController& pSongController, bool pIsArpeggio, std::set<int> pIndexesToDelete) noexcept:
        ExpressionAction(pSongController, pIsArpeggio),
        DeleteItems(std::move(pIndexesToDelete))
{
}

bool DeleteExpressions::perform()
{
    return performAction();
}

bool DeleteExpressions::undo()
{
    return undoAction();
}


// DeleteItems method implementations.
// =================================================

int DeleteExpressions::getItemCount() const noexcept
{
    return getExpressionHandler().getCount();
}

void DeleteExpressions::setSelectedItem(int index, bool forceSingleSelection) noexcept
{
    setSelectedExpression(index, forceSingleSelection);
}

bool DeleteExpressions::performActionOnMapper(Remapper<Expression>& pRemapper) noexcept
{
    return performOnMapper(pRemapper);
}

bool DeleteExpressions::undoActionOnMapper(Remapper<Expression>& pRemapper) noexcept
{
    return undoOnMapper(pRemapper);
}

}   // namespace arkostracker
