#pragma once



#include "../../../lists/view/item/ListItem.h"
#include "../../../lists/view/item/ListModel.h"

namespace arkostracker 
{

class ListItemListener;
class DataProvider;
class InstrumentDataController;

/** Represents the "adapter" for Instruments. Will create the Item Views from the data it has. */
class InstrumentListModel final : public ListModel<ListItem>
{
public:
    /**
     * Constructor.
     * @param listener to be notified on item click.
     * @param dataProvider indicates what to display.
     * @param instrumentDataController provides specialized data about the instrument, and can be sent events about it.
     */
    InstrumentListModel(ListItemListener& listener, DataProvider& dataProvider, InstrumentDataController& instrumentDataController) noexcept;

protected:
    // ListModel method implementations.
    // =================================================
    ListItem* createNewListItem(int rowNumber, bool isRowSelected, DraggedItem::ItemType draggedItemType) noexcept override;

private:
    InstrumentDataController& instrumentDataController;
};

}   // namespace arkostracker

