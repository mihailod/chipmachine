#include "ExpressionListModel.h"

#include "ExpressionListItem.h"

namespace arkostracker 
{

ExpressionListModel::ExpressionListModel(ListItemListener& pListener, DataProvider& pDataProvider, DraggedItem::ItemType pDraggedItemType) noexcept :
        ListModel(pListener, pDataProvider, pDraggedItemType)
{
}


// AbstractListModel method implementations.
// =================================================

ListItem* ExpressionListModel::createNewListItem(int pRowNumber, bool pIsRowSelected, DraggedItem::ItemType pDraggedItemType) noexcept
{
    return new ExpressionListItem(pRowNumber, pIsRowSelected, pDraggedItemType, listener, dataProvider);
}

}   // namespace arkostracker
