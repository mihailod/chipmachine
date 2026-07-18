#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../utils/SelectionInList.h"

namespace arkostracker 
{

/** Abstract List View. */
class ListView : public juce::Component
{
public:
    /** Asks the view to update the list. It will fetch the data by itself from the DataProvider. */
    virtual void updateListContent() noexcept = 0;

    /**
     * Asks for the update of a Row.
     * @param rowIndex the row index.
     */
    virtual void updateRow(int rowIndex) noexcept = 0;

    /**
     * JUCE specific method when a click is performed, in order for the list to manage multiple selection well.
     * @param rowIndex the selected row.
     * @param modifierKeys the modifier keys.
     * @param isMouseUpEvent true if the mouse click is up.
     */
    virtual void selectRowsBasedOnModifierKeys(int rowIndex, juce::ModifierKeys modifierKeys, bool isMouseUpEvent) noexcept = 0;

    /**
     * Shows the selection. This does not trigger any notification. This might scroll for the selected item to be seen.
     * @param selectedRows the selected rows. May be empty.
     */
    virtual void showSelection(const juce::SparseSet<int>& selectedRows) noexcept = 0;

    /** @return the possible selected rows. */
    virtual SelectionInList getSelectedRows() const noexcept = 0;

    /**
     * Prompts the user to ask for the new name for an new item. If confirmed, ask for a rename.
     * @param index the index of the expression to rename (>0).
     */
    virtual void promptRenameItem(int index) noexcept = 0;

    /**
     * Asks the subclass to return the title when prompting to create an item.
     * @return the title.
     */
    virtual juce::String getPromptRenameTitle() const noexcept = 0;
};

}   // namespace arkostracker

