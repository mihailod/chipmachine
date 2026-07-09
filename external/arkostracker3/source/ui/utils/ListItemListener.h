#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/** Listener to an item from a list. */
class ListItemListener
{
public:
    /** Destructor. */
    virtual ~ListItemListener() = default;

    /**
     * Called when an item is clicked. Warning, it is called for both DOWN and UP (in order to manage d'n'd).
     * @param rowIndex the row index.
     * @param modifierKeys to know if multiple selection.
     * @param isLeftMouseDown true if the event on the left button is DOWN. If false, UP.
     */
    virtual void onListItemClicked(int rowIndex, juce::ModifierKeys modifierKeys, bool isLeftMouseDown) = 0;

    /**
     * Called when an item is double-clicked, for the left button (down only).
     * @param rowIndex the row index.
     */
    virtual void onListItemDoubleClicked(int rowIndex) = 0;

    /**
     * Called when an item is right-clicked. Only DOWN is managed.
     * @param rowIndex the row index.
     */
    virtual void onListItemRightClicked(int rowIndex) = 0;

    /**
     * Called when a drag'n'drop is performed.
     * @param sourceRowIndex the index of the source row (>=0).
     * @param destinationRowIndex the index of the destination row (>=0).
     */
    virtual void onDragListItem(int sourceRowIndex, int destinationRowIndex) = 0;

    /** Called when Delete is pressed. */
    virtual void onDeleteKeyPressed() = 0;

    /** Called when Return is pressed. */
    virtual void onReturnKeyPressed() = 0;

    /** The selection has changed. */
    virtual void onListItemSelectionChanged() = 0;
};

}   // namespace arkostracker

