#pragma once



#include "../../../components/lists/AbstractListModel.h"
#include "../../controller/DataProvider.h"
#include "../../../utils/ListItemListener.h"

namespace arkostracker 
{

/** Generic abstract class, represents the "adapter". Will create the Item Views from the data it has. Only the createNewListItem method must be overridden. */
template<typename ITEM>
class ListModel : public AbstractListModel<ITEM>
{
public:
    /**
     * Constructor.
     * @param pListener to be notified on item click.
     * @param pDataProvider indicates what to display.
     * @param pDraggedItemType the type of the dragged item.
     */
    ListModel(ListItemListener& pListener, DataProvider& pDataProvider, DraggedItem::ItemType pDraggedItemType) noexcept :
            AbstractListModel<ITEM>(pDraggedItemType),
            listener(pListener),
            dataProvider(pDataProvider)
    {
    }


    // AbstractListModel method implementations.
    // =================================================

    int getNumRows() override
    {
        return dataProvider.getItemCount();
    }

    void deleteKeyPressed(int /*lastRowSelected*/) override
    {
        listener.onDeleteKeyPressed();
    }

    void returnKeyPressed(int /*lastRowSelected*/) override
    {
        listener.onReturnKeyPressed();
    }

    void selectedRowsChanged(int /*lastRowSelected*/) override
    {
        listener.onListItemSelectionChanged();
    }

protected:
    ListItemListener& listener;                     // To be called on item click.
    DataProvider& dataProvider;                     // Provides the data to display.
};

}   // namespace arkostracker
