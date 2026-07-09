#pragma once


#include "DraggedItem.h"

namespace arkostracker 
{

/**
 * Represents a base "adapter". Will create the Item Views from the data it has.
 * It already implements a few useful boiler-plate methods.
 */
template<typename LIST_ITEM>
class AbstractListModel : public juce::ListBoxModel
{
public:
    /**
     * Constructor
     * @param draggedItemType the type of the dragged item.
     */
    explicit AbstractListModel(DraggedItem::ItemType pDraggedItemType) noexcept :
            draggedItemType(pDraggedItemType)
    {
    }


    // ListBoxModel method implementations.
    // =================================================
    void paintListBoxItem(int /*rowNumber*/, juce::Graphics& /*g*/, int /*width*/, int /*height*/, bool /*rowIsSelected*/) override
    {
        // Nothing to do, we'll build our own Component (see refreshComponentForRow).
    }

    juce::var getDragSourceDescription(const juce::SparseSet<int>& /*rowsToDescribe*/) override
    {
        return "ItemDragged";       // Anything will do, as long as not empty.
    }

    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override
    {
        auto* line = dynamic_cast<LIST_ITEM*>(existingComponentToUpdate);
        if (line != nullptr) {
            // Reuses existing item.
            line->updateData(rowNumber, isRowSelected);
        } else {
            // Creates a new item.
            line = createNewListItem(rowNumber, isRowSelected, draggedItemType);
            jassert(line != nullptr);
        }

        return line;
    }

protected:
    /**
     * Called when a new list item must be done.
     * @param rowNumber the row number (>=0).
     * @param isRowSelected true if the row is selected.
     * @param draggedItemType the type of the dragged item.
     * @return the list item. Must not be nullptr.
     */
    virtual LIST_ITEM* createNewListItem(int rowNumber, bool isRowSelected, DraggedItem::ItemType draggedItemType) noexcept = 0;

private:
    DraggedItem::ItemType draggedItemType;                      // The type of the dragged item.
};


}   // namespace arkostracker

