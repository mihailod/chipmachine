#pragma once

#include "../../../lists/view/item/ListItem.h"
#include "../../../lists/view/item/ListModel.h"

namespace arkostracker 
{

class ListItemListener;
class DataProvider;

/** Represents the "adapter". Will create the Item Views from the data it has. */
class ExpressionListModel : public ListModel<ListItem>
{
public:
    /**
     * Constructor.
     * @param listener to be notified on item click.
     * @param dataProvider indicates what to display.
     * @param draggedItemType the type of the dragged item.
     */
    ExpressionListModel(ListItemListener& listener, DataProvider& dataProvider, DraggedItem::ItemType draggedItemType) noexcept;

protected:
    // ListModel method implementations.
    // =================================================
    ListItem* createNewListItem(int rowNumber, bool isRowSelected, DraggedItem::ItemType draggedItemType) noexcept override;
};

}   // namespace arkostracker
