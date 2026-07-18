#include "AddExpressions.h"

#include "../../../controllers/MainController.h"

namespace arkostracker
{

AddExpressions::AddExpressions(SongController& pSongController, bool pIsArpeggio, int pIndexWhereToInsert, int pOriginalSelectedIndexForUndo,
                               std::vector<std::unique_ptr<Expression>> pExpressions) noexcept:
        ExpressionAction(pSongController, pIsArpeggio),
        AddItems(pIndexWhereToInsert, std::move(pExpressions), pOriginalSelectedIndexForUndo)
{
    jassert(pIndexWhereToInsert > 0);
}

bool AddExpressions::perform()
{
    return performAction();
}

bool AddExpressions::undo()
{
    return undoAction();
}


// DeleteItems method implementations.
// =================================================

int AddExpressions::getItemCount() const noexcept
{
    return getExpressionHandler().getCount();
}

void AddExpressions::setSelectedItem(int index, bool forceSingleSelection) noexcept
{
    setSelectedExpression(index, forceSingleSelection);
}

bool AddExpressions::performActionOnMapper(Remapper<Expression>& pRemapper) noexcept
{
    return performOnMapper(pRemapper);
}

bool AddExpressions::undoActionOnMapper(Remapper<Expression>& pRemapper) noexcept
{
    return undoOnMapper(pRemapper);
}

}   // namespace arkostracker
