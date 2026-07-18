#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "DraggedItem.h"

namespace arkostracker 
{

class ListItemListener;

/**
 * Base class representing a visual item in a list (such as Expression, Instruments, etc.).
 * Implementation should implement mouseDown and paint.
 */
class AbstractListItem : public juce::Component,
                         public juce::DragAndDropTarget
{
public:
    /**
     * Constructor.
     * @param rowNumber the row number (>=0).
     * @param isRowSelected true if the row is selected.
     * @param itemType the type of the dragged item.
     * @param listener the listener to the item click events.
     */
    AbstractListItem(int rowNumber, bool isRowSelected, DraggedItem::ItemType draggedItemType, ListItemListener& listener) noexcept;

    /**
     * Updates the data. This is called when "recycling" the item.
     * @param rowNumber the row number (>=0).
     * @param isRowSelected true if the row is selected.
     */
    void updateData(int rowNumber, bool isRowSelected) noexcept;

    // Component method implementations.
    // =================================================
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

    // DragAndDropTarget method implementations.
    // =================================================
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDragMove(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;
    bool shouldDrawDragImageWhenOver() override;


protected:
    /**
     * Adds the possible 'drag'n'drop' highlight on the given Graphics. This can be called in the paint method, at the end.
     * @param g the Graphics object.
     * @param backgroundColor the background color to convert into a highlight color.
     */
    void showPossibleDragNDropHighlight(juce::Graphics& g, const juce::Colour& backgroundColor) const noexcept;

    /** Removes the hover bars in the UI, provoking a repaint. Useful after a d'n'd, for example. */
    void removeHoverBars();

    /** Asks the subclasses if they want to update their internal data when updateData is called. The default implementation does nothing. */
    virtual void updateDataInternal();

    /**
     * @return true if the row can be dragged.
     * @param rowNumber the row number, as a convenience.
     */
    virtual bool isDragAllowed(int rowNumber) const noexcept = 0;
    /**
     * @return true if a drop can be done here (before it).
     * @param rowNumber the row number, as a convenience.
     */
    virtual bool isDropAllowedHere(int rowNumber) const noexcept = 0;

    /**
     * Manages how a dragged item must be treated.
     * @param dragSourceDetails the drag source details.
     * @return first if true if the drag'n'drop should be managed. Second indicates what part of the item should be highlighted (top = true, bottom = false).
     */
    std::pair<bool, bool> manageDraggedItemValidity(const SourceDetails& dragSourceDetails) const noexcept;

    /** @return the background color for even items. */
    virtual juce::Colour getBackgroundColorForEvenItems(const juce::LookAndFeel& lookAndFeel) noexcept = 0;
    /** @return the background color for odd items. */
    virtual juce::Colour getBackgroundColorForOddItems(const juce::LookAndFeel& lookAndFeel) noexcept = 0;
    /** @return the background color for selected items. */
    virtual juce::Colour getBackgroundColorForSelectedItems(const juce::LookAndFeel& lookAndFeel) noexcept = 0;

    /** @return the type of the dragged item. */
    DraggedItem::ItemType getDraggedItemType() const noexcept;
    /** @return the row number. */
    int getRowNumber() const noexcept;
    /** @return true if the row is selected. */
    bool isRowSelected() const noexcept;

private:
    int rowNumber;
    bool rowSelected;
    DraggedItem::ItemType draggedItemType;          // The type of the dragged item.
    ListItemListener& listener;                     // The listener to the client events.

    bool isHoveredTop;                              // True if a drag'n'drop is hovering in the top part.
    bool isHoveredBottom;                           // True if a drag'n'drop is hovering in the bottom part.
};

}   // namespace arkostracker

