#pragma once

#include "../../../lists/view/item/ListItem.h"

namespace arkostracker 
{

/** A list item for Expressions. */
class ExpressionListItem : public ListItem
{
public:
    /**
     * Constructor.
     * @param rowNumber the row number (>=0).
     * @param isRowSelected true if the row is selected.
     * @param draggedItemType the type of the dragged item.
     * @param listener to be notified on item click.
     * @param dataProvider indicates what to display.
     */
    ExpressionListItem(int rowNumber, bool isRowSelected, DraggedItem::ItemType draggedItemType, ListItemListener& listener, DataProvider& dataProvider) noexcept;

protected:
    bool isDragAllowed(int rowNumber) const noexcept override;
    bool isDropAllowedHere(int rowNumber) const noexcept override;
    juce::Colour getBackgroundColorForEvenItems(const juce::LookAndFeel& lookAndFeel) noexcept override;
    juce::Colour getBackgroundColorForOddItems(const juce::LookAndFeel& lookAndFeel) noexcept override;
    juce::Colour getBackgroundColorForSelectedItems(const juce::LookAndFeel& lookAndFeel) noexcept override;
};


}   // namespace arkostracker

