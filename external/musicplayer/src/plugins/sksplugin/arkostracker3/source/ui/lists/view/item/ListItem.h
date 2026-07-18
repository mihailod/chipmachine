#pragma once

#include "../../../components/lists/AbstractListItem.h"

namespace arkostracker 
{

class DataProvider;
class ListItemListener;

/** Represents a visual item in an item in a list. Abstract. */
class ListItem : public AbstractListItem
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
    ListItem(int rowNumber, bool isRowSelected, DraggedItem::ItemType draggedItemType, ListItemListener& listener, DataProvider& dataProvider) noexcept;

    // Component method implementations.
    // =================================================
    void paint(juce::Graphics& g) override;

protected:
    /**
     * Asks to display the text. Subclasses may want to customize this.
     * @param g the Graphics.
     * @param rowNumber the row number (>=0).
     * @param width the total width of the cell.
     * @param height the total height of the cell.
     * @param suggestedTextX the X of the text, if the suggested width is used.
     * @param textY the Y of the text.
     * @param maximumTextWidth the maximum the text should use.
     * @param textHeight the height of the text.
     */
    virtual void drawText(juce::Graphics& g, int rowNumber, int width, int height, int suggestedTextX, int textY,
                          int maximumTextWidth, int textHeight) noexcept;

    /**
     * Allows subclasses to draw after the text has been displayed.
     * @param graphics the Graphics.
     * @param rowNumber the row number (>=0).
     * @param width the total width of the cell.
     * @param height the total height of the cell.
     * @param textY the Y of the text.
     * @param textHeight the height of the text.
     */
    virtual void drawOnGraphics(juce::Graphics& graphics, int rowNumber, int width, int height, int textY, int textHeight) noexcept;

    DataProvider& dataProvider;                     // Provides the data to display.
};

}   // namespace arkostracker
