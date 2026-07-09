#include "InstrumentListModel.h"

#include "InstrumentListItem.h"

namespace arkostracker 
{

InstrumentListModel::InstrumentListModel(ListItemListener& pListener, DataProvider& pDataProvider, InstrumentDataController& pInstrumentDataController) noexcept :
        ListModel(pListener, pDataProvider, DraggedItem::ItemType::instrument),
        instrumentDataController(pInstrumentDataController)
{
}


// AbstractListModel method implementations.
// =================================================

ListItem* InstrumentListModel::createNewListItem(const int rowNumber, const bool isRowSelected, DraggedItem::ItemType /*draggedItemType*/) noexcept
{
    return new InstrumentListItem(rowNumber, isRowSelected, listener, dataProvider, instrumentDataController);
}

}   // namespace arkostracker
